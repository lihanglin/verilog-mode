#include "verilog_ts_common.hpp"

namespace verilog_ts {

namespace {

bool is_word_char(char c) {
  unsigned char uc = static_cast<unsigned char>(c);
  return std::isalnum(uc) || c == '_' || c == '$' || c == '`';
}

std::string_view read_token_at(const BufferState &bs, uint32_t byte,
                               uint32_t *end_out = nullptr) {
  uint32_t n = static_cast<uint32_t>(bs.text.size());
  if (byte >= n) {
    if (end_out) *end_out = n;
    return {};
  }
  if (bs.text[byte] == '\\') {
    uint32_t j = byte + 1;
    while (j < n && bs.text[j] != ' ' && bs.text[j] != '\t' &&
           bs.text[j] != '\n' && bs.text[j] != '\r' && bs.text[j] != '\f') {
      ++j;
    }
    if (end_out) *end_out = j;
    return bs.slice(byte, j);
  }
  uint32_t j = byte;
  while (j < n && is_word_char(bs.text[j])) ++j;
  if (end_out) *end_out = j;
  return bs.slice(byte, j);
}

// Extract the previous non-comment word token before `byte` on the same statement/line
std::string_view prev_word_before(const BufferState &bs, uint32_t byte,
                                  uint32_t *word_start_out = nullptr) {
  uint32_t p = backward_syntactic_ws_bytes(bs, byte, 0, false);
  if (p == 0) return {};
  uint32_t end = p;
  while (p > 0 && is_word_char(bs.text[p - 1])) --p;
  if (word_start_out) *word_start_out = p;
  return bs.slice(p, end);
}

bool is_preceded_by_typedef_or_extern_or_dpi(const BufferState &bs,
                                             uint32_t kwd_byte,
                                             uint32_t *prefix_start_out = nullptr) {
  uint32_t ls = bs.line_start_byte(kwd_byte);
  uint32_t p = ls;
  uint32_t first_tok_start = kwd_byte;
  bool saw_prefix = false;
  while (p < kwd_byte) {
    p = forward_syntactic_ws_bytes(bs, p, kwd_byte, false);
    if (p >= kwd_byte) break;
    uint32_t tend = p;
    std::string_view tok = read_token_at(bs, p, &tend);
    if (tok.empty()) {
      ++p;
      continue;
    }
    if (tok == "typedef" || tok == "extern" || tok == "pure" ||
        tok == "import" || tok == "export" || tok == "context") {
      return true;
    }
    if ((tok == "virtual" || tok == "interface" || tok == "protected" ||
         tok == "local" || tok == "static") &&
        !saw_prefix) {
      first_tok_start = p;
      saw_prefix = true;
    }
    p = tend;
  }
  if (prefix_start_out) *prefix_start_out = first_tok_start;
  return false;
}

std::optional<std::string_view> closer_for_defun_opener(std::string_view opener) {
  if (opener == "module" || opener == "macromodule") return "endmodule";
  if (opener == "connectmodule") return "endconnectmodule";
  if (opener == "interface") return "endinterface";
  if (opener == "program") return "endprogram";
  if (opener == "package") return "endpackage";
  if (opener == "class") return "endclass";
  if (opener == "primitive") return "endprimitive";
  if (opener == "checker") return "endchecker";
  if (opener == "config") return "endconfig";
  if (opener == "function") return "endfunction";
  if (opener == "task") return "endtask";
  return std::nullopt;
}

bool is_module_like_opener(std::string_view opener) {
  return opener == "module" || opener == "macromodule" ||
         opener == "connectmodule" || opener == "interface" ||
         opener == "program" || opener == "package" ||
         opener == "class" || opener == "primitive" ||
         opener == "config" || opener == "checker";
}

// Parse defun header starting right after opener keyword to extract name and header_end_byte
void extract_defun_name_and_header(const BufferState &bs, uint32_t after_kwd,
                                   uint32_t limit_byte, std::string_view opener,
                                   std::string &name_out,
                                   uint32_t &header_end_out) {
  uint32_t n = std::min<uint32_t>(limit_byte,
                                  static_cast<uint32_t>(bs.text.size()));
  uint32_t cur = forward_syntactic_ws_bytes(bs, after_kwd, n, true);

  // Skip optional lifetime (automatic | static)
  uint32_t tend = cur;
  std::string_view tok = read_token_at(bs, cur, &tend);
  if (tok == "automatic" || tok == "static") {
    cur = forward_syntactic_ws_bytes(bs, tend, n, true);
    tok = read_token_at(bs, cur, &tend);
  }

  if (opener == "function" || opener == "task") {
    // Function may have return type or [msb:lsb] before function name; scan to identifier before '(' or ';'
    uint32_t last_id_start = cur;
    uint32_t last_id_end = tend;
    uint32_t scan = cur;
    int bracket_depth = 0;
    while (scan < n) {
      scan = forward_syntactic_ws_bytes(bs, scan, n, true);
      if (scan >= n) break;
      char c = bs.text[scan];
      if (c == '[') {
        ++bracket_depth;
        ++scan;
        continue;
      }
      if (c == ']') {
        if (bracket_depth > 0) --bracket_depth;
        ++scan;
        continue;
      }
      if (bracket_depth == 0 && (c == '(' || c == ';')) {
        header_end_out = scan + 1;
        break;
      }
      uint32_t id_end = scan;
      std::string_view id = read_token_at(bs, scan, &id_end);
      if (!id.empty()) {
        if (bracket_depth == 0) {
          last_id_start = scan;
          last_id_end = id_end;
        }
        scan = id_end;
      } else {
        ++scan;
      }
    }
    name_out = std::string(bs.slice(last_id_start, last_id_end));
    if (header_end_out == 0) header_end_out = last_id_end;
    return;
  }

  // Module / interface / program / package / class / checker / primitive / config
  name_out = std::string(tok);
  uint32_t scan = tend;
  while (scan < n) {
    scan = forward_syntactic_ws_bytes(bs, scan, n, true);
    if (scan >= n) break;
    if (bs.text[scan] == '(' || bs.text[scan] == ';') {
      header_end_out = scan + 1;
      break;
    }
    ++scan;
  }
  if (header_end_out == 0) header_end_out = tend;
}

} // namespace

std::vector<DefunInfo> find_all_defuns(const BufferState &bs,
                                       bool modules_only) {
  std::vector<DefunInfo> results;
  uint32_t n = static_cast<uint32_t>(bs.text.size());
  uint32_t i = 0;

  while (i < n) {
    i = forward_syntactic_ws_bytes(bs, i, n, false);
    if (i >= n) break;

    if (bs.is_inside_protected_byte(i) ||
        bs.is_inside_comment_or_string_byte(i, false)) {
      ++i;
      continue;
    }

    if (!is_word_char(bs.text[i])) {
      ++i;
      continue;
    }

    uint32_t tend = i;
    std::string_view tok = read_token_at(bs, i, &tend);
    if (tok.empty()) {
      ++i;
      continue;
    }

    // Check if tok is a defun opener keyword
    auto closer_opt = closer_for_defun_opener(tok);
    if (!closer_opt.has_value()) {
      i = tend;
      continue;
    }

    if (modules_only && !is_module_like_opener(tok)) {
      i = tend;
      continue;
    }

    // Ignore `interface` if it is `interface class` (handled when `class` is reached)
    // or `virtual interface` (variable declaration)
    if (tok == "interface") {
      uint32_t next_ws = forward_syntactic_ws_bytes(bs, tend, n, false);
      uint32_t next_end = next_ws;
      std::string_view next_tok = read_token_at(bs, next_ws, &next_end);
      if (next_tok == "class") {
        i = tend;
        continue;
      }
      std::string_view prev_tok = prev_word_before(bs, i);
      if (prev_tok == "virtual") {
        i = tend;
        continue;
      }
    }

    uint32_t start_byte = i;
    if (is_preceded_by_typedef_or_extern_or_dpi(bs, i, &start_byte)) {
      i = tend;
      continue;
    }

    std::string_view closer = *closer_opt;
    // Scan forward to matching closer (supporting nested class..endclass)
    uint32_t scan = tend;
    int depth = 1;
    uint32_t end_byte = n;
    bool found_closer = false;
    while (scan < n) {
      scan = forward_syntactic_ws_bytes(bs, scan, n, false);
      if (scan >= n) break;
      if (bs.is_inside_protected_byte(scan) ||
          bs.is_inside_comment_or_string_byte(scan, false)) {
        ++scan;
        continue;
      }
      if (!is_word_char(bs.text[scan])) {
        ++scan;
        continue;
      }
      uint32_t wend = scan;
      std::string_view w = read_token_at(bs, scan, &wend);
      if (w == tok && tok == "class") {
        if (!is_preceded_by_typedef_or_extern_or_dpi(bs, scan)) {
          ++depth;
        }
      } else if (w == closer) {
        --depth;
        if (depth == 0) {
          end_byte = wend;
          found_closer = true;
          break;
        }
      }
      scan = wend;
    }

    DefunInfo info;
    info.type = std::string(tok);
    info.start_byte = start_byte;
    info.kwd_byte = i;
    info.header_end_byte = 0;
    info.end_byte = end_byte;
    extract_defun_name_and_header(bs, tend, end_byte, tok, info.name,
                                  info.header_end_byte);
    results.push_back(std::move(info));

    (void)found_closer;
    i = tend;
  }
  return results;
}

std::optional<DefunInfo> find_enclosing_defun(const BufferState &bs,
                                              uint32_t byte, bool module_only) {
  std::vector<DefunInfo> defuns = find_all_defuns(bs, module_only);
  const DefunInfo *best = nullptr;
  for (const auto &d : defuns) {
    if (d.start_byte <= byte && byte <= d.end_byte) {
      if (!best || (d.end_byte - d.start_byte) <= (best->end_byte - best->start_byte)) {
        best = &d;
      }
    }
  }
  if (best) return *best;
  // Fallback: last defun starting before `byte` (matches verilog-beg-of-defun behavior when after endmodule)
  for (auto it = defuns.rbegin(); it != defuns.rend(); ++it) {
    if (it->start_byte <= byte) {
      return *it;
    }
  }
  return std::nullopt;
}

namespace {

enum class TokenRole {
  NONE,
  OPENER,
  MIDDLE,
  CLOSER
};

struct BlockPairSpec {
  std::string_view category;
  bool nestable;
};

// Classify a token for block matching (begin..end, case..endcase, fork..join*, `ifdef..`endif, UVM/OVM/VMM, etc.)
TokenRole classify_block_token(const BufferState &bs, uint32_t tok_start,
                               std::string_view tok,
                               std::string_view *category_out) {
  if (tok == "begin") {
    *category_out = "begin";
    return TokenRole::OPENER;
  }
  if (tok == "end") {
    *category_out = "begin";
    return TokenRole::CLOSER;
  }
  if (tok == "case" || tok == "casex" || tok == "casez" || tok == "randcase") {
    // Check that case is not followed immediately by ':'
    uint32_t tend = tok_start + static_cast<uint32_t>(tok.size());
    if (tend < bs.text.size() && bs.text[tend] == ':') return TokenRole::NONE;
    *category_out = "case";
    return TokenRole::OPENER;
  }
  if (tok == "endcase") {
    *category_out = "case";
    return TokenRole::CLOSER;
  }
  if (tok == "fork") {
    std::string_view prev = prev_word_before(bs, tok_start);
    if (prev == "disable" || prev == "wait") return TokenRole::NONE;
    *category_out = "fork";
    return TokenRole::OPENER;
  }
  if (tok == "join" || tok == "join_any" || tok == "join_none") {
    *category_out = "fork";
    return TokenRole::CLOSER;
  }
  if (tok == "class") {
    if (is_preceded_by_typedef_or_extern_or_dpi(bs, tok_start))
      return TokenRole::NONE;
    *category_out = "class";
    return TokenRole::OPENER;
  }
  if (tok == "endclass") {
    *category_out = "class";
    return TokenRole::CLOSER;
  }
  if (tok == "function") {
    if (is_preceded_by_typedef_or_extern_or_dpi(bs, tok_start))
      return TokenRole::NONE;
    *category_out = "function";
    return TokenRole::OPENER;
  }
  if (tok == "endfunction") {
    *category_out = "function";
    return TokenRole::CLOSER;
  }
  if (tok == "task") {
    if (is_preceded_by_typedef_or_extern_or_dpi(bs, tok_start))
      return TokenRole::NONE;
    *category_out = "task";
    return TokenRole::OPENER;
  }
  if (tok == "endtask") {
    *category_out = "task";
    return TokenRole::CLOSER;
  }
  if (tok == "generate") {
    *category_out = "generate";
    return TokenRole::OPENER;
  }
  if (tok == "endgenerate") {
    *category_out = "generate";
    return TokenRole::CLOSER;
  }
  if (tok == "covergroup") {
    *category_out = "covergroup";
    return TokenRole::OPENER;
  }
  if (tok == "endgroup") {
    *category_out = "covergroup";
    return TokenRole::CLOSER;
  }
  if (tok == "property") {
    std::string_view prev = prev_word_before(bs, tok_start);
    if (prev == "assert" || prev == "assume" || prev == "cover" ||
        prev == "restrict" || prev == "expect") {
      return TokenRole::NONE;
    }
    *category_out = "property";
    return TokenRole::OPENER;
  }
  if (tok == "endproperty") {
    *category_out = "property";
    return TokenRole::CLOSER;
  }
  if (tok == "sequence" || tok == "randsequence") {
    std::string_view prev = prev_word_before(bs, tok_start);
    if (prev == "assert" || prev == "assume" || prev == "cover" ||
        prev == "restrict" || prev == "expect") {
      return TokenRole::NONE;
    }
    *category_out = "sequence";
    return TokenRole::OPENER;
  }
  if (tok == "endsequence") {
    *category_out = "sequence";
    return TokenRole::CLOSER;
  }
  if (tok == "clocking") {
    std::string_view prev = prev_word_before(bs, tok_start);
    if (prev == "default") return TokenRole::NONE;
    *category_out = "clocking";
    return TokenRole::OPENER;
  }
  if (tok == "endclocking") {
    *category_out = "clocking";
    return TokenRole::CLOSER;
  }
  if (tok == "specify") {
    *category_out = "specify";
    return TokenRole::OPENER;
  }
  if (tok == "endspecify") {
    *category_out = "specify";
    return TokenRole::CLOSER;
  }
  if (tok == "table") {
    *category_out = "table";
    return TokenRole::OPENER;
  }
  if (tok == "endtable") {
    *category_out = "table";
    return TokenRole::CLOSER;
  }
  if (tok == "module" || tok == "macromodule") {
    *category_out = "module";
    return TokenRole::OPENER;
  }
  if (tok == "endmodule") {
    *category_out = "module";
    return TokenRole::CLOSER;
  }
  if (tok == "connectmodule") {
    *category_out = "connectmodule";
    return TokenRole::OPENER;
  }
  if (tok == "endconnectmodule") {
    *category_out = "connectmodule";
    return TokenRole::CLOSER;
  }
  if (tok == "interface") {
    uint32_t next_ws = forward_syntactic_ws_bytes(
        bs, tok_start + static_cast<uint32_t>(tok.size()),
        static_cast<uint32_t>(bs.text.size()), false);
    std::string_view next_tok = read_token_at(bs, next_ws);
    if (next_tok == "class") return TokenRole::NONE;
    if (prev_word_before(bs, tok_start) == "virtual") return TokenRole::NONE;
    *category_out = "interface";
    return TokenRole::OPENER;
  }
  if (tok == "endinterface") {
    *category_out = "interface";
    return TokenRole::CLOSER;
  }
  if (tok == "package") {
    *category_out = "package";
    return TokenRole::OPENER;
  }
  if (tok == "endpackage") {
    *category_out = "package";
    return TokenRole::CLOSER;
  }
  if (tok == "program") {
    *category_out = "program";
    return TokenRole::OPENER;
  }
  if (tok == "endprogram") {
    *category_out = "program";
    return TokenRole::CLOSER;
  }
  if (tok == "primitive") {
    *category_out = "primitive";
    return TokenRole::OPENER;
  }
  if (tok == "endprimitive") {
    *category_out = "primitive";
    return TokenRole::CLOSER;
  }
  if (tok == "checker") {
    *category_out = "checker";
    return TokenRole::OPENER;
  }
  if (tok == "endchecker") {
    *category_out = "checker";
    return TokenRole::CLOSER;
  }
  if (tok == "config") {
    *category_out = "config";
    return TokenRole::OPENER;
  }
  if (tok == "endconfig") {
    *category_out = "config";
    return TokenRole::CLOSER;
  }
  if (tok == "`ifdef" || tok == "`ifndef") {
    *category_out = "ifdef";
    return TokenRole::OPENER;
  }
  if (tok == "`else" || tok == "`elsif") {
    *category_out = "ifdef";
    return TokenRole::MIDDLE;
  }
  if (tok == "`endif") {
    *category_out = "ifdef";
    return TokenRole::CLOSER;
  }
  if (tok.rfind("`uvm_", 0) == 0 || tok.rfind("`ovm_", 0) == 0 ||
      tok.rfind("`vmm_", 0) == 0) {
    if (tok.size() >= 6 && tok.substr(tok.size() - 6) == "_begin") {
      *category_out = "uvm";
      return TokenRole::OPENER;
    }
    if (tok.size() >= 4 && tok.substr(tok.size() - 4) == "_end") {
      *category_out = "uvm";
      return TokenRole::CLOSER;
    }
  }
  return TokenRole::NONE;
}

} // namespace

std::optional<uint32_t> leap_to_head_byte(const BufferState &bs,
                                          uint32_t byte) {
  uint32_t n = static_cast<uint32_t>(bs.text.size());
  if (byte >= n) return std::nullopt;
  uint32_t start = byte;
  if (!is_word_char(bs.text[start])) {
    while (start > 0 && is_word_char(bs.text[start - 1])) --start;
  }
  while (start > 0 && is_word_char(bs.text[start - 1])) --start;

  uint32_t tend = start;
  std::string_view tok = read_token_at(bs, start, &tend);
  if (tok.empty()) return std::nullopt;

  std::string_view target_cat;
  TokenRole role = classify_block_token(bs, start, tok, &target_cat);
  if (role != TokenRole::CLOSER && role != TokenRole::MIDDLE) {
    return std::nullopt;
  }

  // Scan backward through tokens outside comments/strings
  bool non_nesting = (target_cat == "function" || target_cat == "task" ||
                      target_cat == "package" || target_cat == "program");
  int depth = 1;
  std::string_view sub_cat;
  int sub_depth = 0;
  uint32_t cur = start;
  while (cur > 0) {
    cur = backward_syntactic_ws_bytes(bs, cur, 0, false);
    if (cur == 0) break;
    if (bs.is_inside_comment_or_string_byte(cur - 1, false) ||
        bs.is_inside_protected_byte(cur - 1)) {
      --cur;
      continue;
    }
    if (!is_word_char(bs.text[cur - 1])) {
      --cur;
      continue;
    }
    uint32_t wstart = cur - 1;
    while (wstart > 0 && is_word_char(bs.text[wstart - 1])) --wstart;
    std::string_view w = bs.slice(wstart, cur);
    std::string_view cat;
    TokenRole r = classify_block_token(bs, wstart, w, &cat);
    if (!sub_cat.empty()) {
      if (cat == sub_cat) {
        if (r == TokenRole::CLOSER) {
          ++sub_depth;
        } else if (r == TokenRole::OPENER) {
          --sub_depth;
          if (sub_depth == 0) {
            sub_cat = {};
          }
        }
      }
      cur = wstart;
      continue;
    }
    if (target_cat == "begin" && r == TokenRole::CLOSER &&
        (cat == "case" || cat == "fork")) {
      sub_cat = cat;
      sub_depth = 1;
      cur = wstart;
      continue;
    }
    if (cat == target_cat) {
      if (r == TokenRole::CLOSER && !non_nesting) {
        ++depth;
      } else if (r == TokenRole::OPENER) {
        --depth;
        if (depth == 0 || non_nesting) {
          // If case had unique/priority prefix, or class had virtual/interface prefix, adjust if needed
          if (cat == "case") {
            uint32_t pstart = wstart;
            std::string_view prev = prev_word_before(bs, wstart, &pstart);
            if (prev == "unique" || prev == "unique0" || prev == "priority") {
              return pstart;
            }
          } else if (cat == "class") {
            uint32_t pstart = wstart;
            std::string_view prev = prev_word_before(bs, wstart, &pstart);
            if (prev == "virtual" || prev == "interface") {
              return pstart;
            }
          } else if (cat == "function" || cat == "task") {
            uint32_t pstart = wstart;
            is_preceded_by_typedef_or_extern_or_dpi(bs, wstart, &pstart);
            return pstart;
          }
          return wstart;
        }
      } else if (r == TokenRole::MIDDLE && role == TokenRole::MIDDLE &&
                 w == "`elsif" && depth == 1) {
        return wstart;
      }
    }
    cur = wstart;
  }
  return std::nullopt;
}

std::optional<uint32_t> forward_sexp_byte(const BufferState &bs,
                                          uint32_t byte) {
  uint32_t n = static_cast<uint32_t>(bs.text.size());
  if (byte >= n) return std::nullopt;

  // Check if at delimiter ( [ {
  char c = bs.text[byte];
  if (c == '(' || c == '[' || c == '{') {
    char close_c = (c == '(') ? ')' : (c == '[') ? ']' : '}';
    int depth = 1;
    uint32_t i = byte + 1;
    while (i < n) {
      if (const CommentStringSpan *sp = bs.find_span_at_byte(i, false)) {
        i = sp->end_byte;
        continue;
      }
      if (bs.text[i] == c) {
        ++depth;
      } else if (bs.text[i] == close_c) {
        --depth;
        if (depth == 0) return i + 1;
      }
      ++i;
    }
    return std::nullopt;
  }

  uint32_t start = byte;
  if (!is_word_char(bs.text[start])) {
    while (start > 0 && is_word_char(bs.text[start - 1])) --start;
  } else {
    while (start > 0 && is_word_char(bs.text[start - 1])) --start;
  }
  uint32_t tend = start;
  std::string_view tok = read_token_at(bs, start, &tend);
  if (tok.empty()) return std::nullopt;

  // Handle virtual/interface class
  if (tok == "virtual" || tok == "interface") {
    uint32_t nws = forward_syntactic_ws_bytes(bs, tend, n, false);
    uint32_t nend = nws;
    std::string_view ntok = read_token_at(bs, nws, &nend);
    if (ntok == "class" || ntok == "function" || ntok == "task") {
      start = nws;
      tok = ntok;
      tend = nend;
    }
  }

  std::string_view target_cat;
  TokenRole role = classify_block_token(bs, start, tok, &target_cat);
  if (role != TokenRole::OPENER && role != TokenRole::MIDDLE) {
    return std::nullopt;
  }

  int depth = 1;
  uint32_t cur = tend;
  while (cur < n) {
    cur = forward_syntactic_ws_bytes(bs, cur, n, false);
    if (cur >= n) break;
    if (bs.is_inside_comment_or_string_byte(cur, false) ||
        bs.is_inside_protected_byte(cur)) {
      ++cur;
      continue;
    }
    if (!is_word_char(bs.text[cur])) {
      ++cur;
      continue;
    }
    uint32_t wend = cur;
    std::string_view w = read_token_at(bs, cur, &wend);
    std::string_view cat;
    TokenRole r = classify_block_token(bs, cur, w, &cat);
    if (cat == target_cat) {
      if (r == TokenRole::OPENER) {
        ++depth;
      } else if (r == TokenRole::CLOSER) {
        --depth;
        if (depth == 0) return wend;
      } else if (r == TokenRole::MIDDLE && depth == 1) {
        return wend;
      }
    }
    cur = wend;
  }
  return std::nullopt;
}

std::optional<uint32_t> backward_sexp_byte(const BufferState &bs,
                                           uint32_t byte) {
  uint32_t n = static_cast<uint32_t>(bs.text.size());
  if (byte == 0 || byte > n) return std::nullopt;

  // Check if preceding char is ) ] }
  char prev_c = bs.text[byte - 1];
  if (prev_c == ')' || prev_c == ']' || prev_c == '}') {
    char open_c = (prev_c == ')') ? '(' : (prev_c == ']') ? '[' : '{';
    int depth = 1;
    uint32_t i = byte - 1;
    while (i > 0) {
      --i;
      if (const CommentStringSpan *sp = bs.find_span_at_byte(i, false)) {
        i = sp->start_byte;
        continue;
      }
      if (bs.text[i] == prev_c) {
        ++depth;
      } else if (bs.text[i] == open_c) {
        --depth;
        if (depth == 0) return i;
      }
    }
    return std::nullopt;
  }

  return leap_to_head_byte(bs, byte);
}

std::optional<BlockInfo> find_block_at_byte(const BufferState &bs,
                                            uint32_t byte) {
  uint32_t n = static_cast<uint32_t>(bs.text.size());
  if (byte >= n && n > 0) byte = n - 1;

  // First check if point is directly on an opener or closer keyword
  uint32_t wstart = byte;
  while (wstart > 0 && is_word_char(bs.text[wstart - 1])) --wstart;
  uint32_t wend = wstart;
  std::string_view tok = read_token_at(bs, wstart, &wend);
  if (!tok.empty()) {
    std::string_view cat;
    TokenRole r = classify_block_token(bs, wstart, tok, &cat);
    if (r == TokenRole::OPENER) {
      if (auto end_b = forward_sexp_byte(bs, wstart)) {
        BlockInfo bi;
        bi.type = std::string(cat);
        bi.start_byte = wstart;
        bi.end_byte = *end_b;
        bi.end_kwd_byte = *end_b;
        return bi;
      }
    } else if (r == TokenRole::CLOSER) {
      if (auto head_b = leap_to_head_byte(bs, wstart)) {
        BlockInfo bi;
        bi.type = std::string(cat);
        bi.start_byte = *head_b;
        bi.end_kwd_byte = wstart;
        bi.end_byte = wend;
        return bi;
      }
    }
  }

  // Otherwise, use TSTree to find the smallest enclosing AST block node
  if (bs.tree) {
    TSNode root = ts_tree_root_node(bs.tree);
    TSNode cur = ts_node_descendant_for_byte_range(root, byte, byte);
    static const std::unordered_set<std::string_view> kAstBlocks = {
        "seq_block",              "par_block",
        "case_statement",         "randcase_statement",
        "generate_region",        "covergroup_declaration",
        "property_declaration",   "sequence_declaration",
        "randsequence_statement", "clocking_declaration",
        "specify_block",          "table_definition",
        "class_declaration",      "function_declaration",
        "task_declaration",       "module_declaration",
        "interface_declaration",  "program_declaration",
        "package_declaration",    "checker_declaration",
        "config_declaration",     "udp_declaration"};
    while (!ts_node_is_null(cur)) {
      std::string_view nt = ts_node_type(cur);
      if (kAstBlocks.count(nt) > 0) {
        BlockInfo bi;
        bi.type = std::string(nt);
        bi.start_byte = ts_node_start_byte(cur);
        bi.end_byte = ts_node_end_byte(cur);
        bi.end_kwd_byte = bi.end_byte;
        return bi;
      }
      cur = ts_node_parent(cur);
    }
  }
  return std::nullopt;
}

std::pair<uint32_t, uint32_t> find_statement_bounds_bytes(const BufferState &bs,
                                                          uint32_t byte) {
  if (!bs.tree) return {byte, byte};
  uint32_t n = static_cast<uint32_t>(bs.text.size());
  if (byte >= n && n > 0) byte = n - 1;

  TSNode root = ts_tree_root_node(bs.tree);
  TSNode cur = ts_node_descendant_for_byte_range(root, byte, byte);
  static const std::unordered_set<std::string_view> kStmtNodes = {
      "statement_or_null",
      "statement",
      "statement_item",
      "blocking_assignment",
      "nonblocking_assignment",
      "continuous_assign",
      "net_declaration",
      "data_declaration",
      "port_declaration",
      "ansi_port_declaration",
      "parameter_declaration",
      "local_parameter_declaration",
      "type_declaration",
      "module_instantiation",
      "interface_instantiation",
      "program_instantiation",
      "gate_instantiation",
      "conditional_statement",
      "case_statement",
      "loop_statement",
      "Always_construct",
      "always_construct",
      "initial_construct",
      "final_construct",
      "concurrent_assertion_item",
      "deferred_immediate_assertion_item",
      "simple_immediate_assertion_statement"};

  while (!ts_node_is_null(cur)) {
    std::string_view nt = ts_node_type(cur);
    if (kStmtNodes.count(nt) > 0) {
      return {ts_node_start_byte(cur), ts_node_end_byte(cur)};
    }
    cur = ts_node_parent(cur);
  }
  return {bs.line_start_byte(byte), bs.line_end_byte(byte)};
}

bool check_in_context(const BufferState &bs, std::string_view context,
                      uint32_t byte, intmax_t *nested_col_out) {
  if (nested_col_out) *nested_col_out = -1;
  uint32_t n = static_cast<uint32_t>(bs.text.size());
  if (byte > n) byte = n;

  if (context == "attribute") {
    return bs.is_inside_attribute_byte(byte);
  }
  if (context == "protected") {
    return bs.is_inside_protected_byte(byte);
  }
  if (context == "directive") {
    uint32_t ls = bs.line_start_byte(byte);
    uint32_t i = ls;
    while (i < n && (bs.text[i] == ' ' || bs.text[i] == '\t')) ++i;
    if (i < n && bs.text[i] == '`') {
      std::string_view tok = read_token_at(bs, i);
      if (tok.rfind("`uvm_", 0) != 0 && tok.rfind("`ovm_", 0) != 0 &&
          tok.rfind("`vmm_", 0) != 0) {
        return true;
      }
    }
    return false;
  }
  if (context == "parameter") {
    int paren_depth = 0;
    uint32_t i = byte;
    uint32_t open_paren = n;
    while (i > 0) {
      --i;
      if (const CommentStringSpan *sp = bs.find_span_at_byte(i, false)) {
        i = sp->start_byte;
        continue;
      }
      char c = bs.text[i];
      if (c == ')' || c == ']' || c == '}') {
        ++paren_depth;
      } else if (c == '(' || c == '[' || c == '{') {
        if (paren_depth == 0) {
          if (c == '(') open_paren = i;
          break;
        }
        --paren_depth;
      }
    }
    if (open_paren >= n) return false;
    uint32_t before_paren =
        backward_syntactic_ws_bytes(bs, open_paren, 0, false);
    if (before_paren == 0 || bs.text[before_paren - 1] != '#') return false;
    auto d = find_enclosing_defun(bs, before_paren - 1, true);
    if (!d) return false;
    for (uint32_t k = d->kwd_byte; k < before_paren - 1; ++k) {
      if (bs.is_inside_comment_or_string_byte(k, false)) continue;
      if (bs.text[k] == ';') return false;
    }
    return true;
  }

  if (bs.tree) {
    uint32_t qbyte = (byte >= n && n > 0) ? (n - 1) : byte;
    TSNode root = ts_tree_root_node(bs.tree);
    TSNode cur = ts_node_descendant_for_byte_range(root, qbyte, qbyte);

    if (context == "struct" || context == "struct-nested") {
      int struct_count = 0;
      TSNode outer_struct = {};
      TSNode walk = cur;
      while (!ts_node_is_null(walk)) {
        std::string_view nt = ts_node_type(walk);
        if (nt == "struct_union_member" ||
            (nt == "data_type" &&
             bs.node_text(walk).find("struct") != std::string_view::npos) ||
            (nt == "data_type" &&
             bs.node_text(walk).find("union") != std::string_view::npos)) {
          // Verify byte is between '{' and '}'
          uint32_t sb = ts_node_start_byte(walk);
          uint32_t eb = ts_node_end_byte(walk);
          std::string_view sv = bs.slice(sb, eb);
          size_t lbrace = sv.find('{');
          size_t rbrace = sv.rfind('}');
          if (lbrace != std::string_view::npos &&
              byte > sb + static_cast<uint32_t>(lbrace) &&
              (rbrace == std::string_view::npos ||
               byte <= sb + static_cast<uint32_t>(rbrace))) {
            ++struct_count;
            outer_struct = walk;
          }
        }
        walk = ts_node_parent(walk);
      }
      if (struct_count > 0) {
        if (nested_col_out) {
          if (struct_count > 1 && !ts_node_is_null(outer_struct)) {
            *nested_col_out = static_cast<intmax_t>(
                bs.byte_to_column(ts_node_start_byte(outer_struct)));
          } else {
            *nested_col_out = 0;
          }
        }
        return true;
      }
      return false;
    }

    if (context == "coverage") {
      TSNode walk = cur;
      while (!ts_node_is_null(walk)) {
        std::string_view nt = ts_node_type(walk);
        if (nt == "constraint_block" || nt == "constraint_set" ||
            nt == "cover_point" || nt == "cover_cross" ||
            nt == "bins_or_empty") {
          uint32_t sb = ts_node_start_byte(walk);
          uint32_t eb = ts_node_end_byte(walk);
          std::string_view sv = bs.slice(sb, eb);
          size_t lbrace = sv.find('{');
          if (lbrace != std::string_view::npos &&
              byte > sb + static_cast<uint32_t>(lbrace) && byte < eb) {
            return true;
          }
        }
        walk = ts_node_parent(walk);
      }
      return false;
    }

    if (context == "case-region") {
      TSNode walk = cur;
      while (!ts_node_is_null(walk)) {
        std::string_view nt = ts_node_type(walk);
        if (nt == "case_statement" || nt == "randcase_statement") {
          return true;
        }
        walk = ts_node_parent(walk);
      }
      return false;
    }

    if (context == "generate-region") {
      TSNode walk = cur;
      while (!ts_node_is_null(walk)) {
        std::string_view nt = ts_node_type(walk);
        if (nt == "generate_region" || nt == "loop_generate_construct" ||
            nt == "if_generate_construct" || nt == "case_generate_construct") {
          return true;
        }
        walk = ts_node_parent(walk);
      }
      return false;
    }

    if (context == "fork-region") {
      TSNode walk = cur;
      while (!ts_node_is_null(walk)) {
        std::string_view nt = ts_node_type(walk);
        if (nt == "par_block") {
          return true;
        }
        walk = ts_node_parent(walk);
      }
      return false;
    }

    if (context == "parameter") {
      TSNode walk = cur;
      while (!ts_node_is_null(walk)) {
        std::string_view nt = ts_node_type(walk);
        if (nt == "parameter_port_list") {
          return true;
        }
        walk = ts_node_parent(walk);
      }
      return false;
    }
  }
  return false;
}

namespace {

emacs_value defun_to_plist(emacs_env *env, const BufferState &bs,
                           const DefunInfo &d) {
  return elisp_list(env, {elisp_intern(env, ":type"),
                          elisp_string(env, d.type),
                          elisp_intern(env, ":name"),
                          elisp_string(env, d.name),
                          elisp_intern(env, ":start"),
                          elisp_int(env, bs.byte_to_pos(d.start_byte)),
                          elisp_intern(env, ":kwd-pos"),
                          elisp_int(env, bs.byte_to_pos(d.kwd_byte)),
                          elisp_intern(env, ":header-end"),
                          elisp_int(env, bs.byte_to_pos(d.header_end_byte)),
                          elisp_intern(env, ":end"),
                          elisp_int(env, bs.byte_to_pos(d.end_byte))});
}

emacs_value Fverilog_ts_defun_bounds(emacs_env *env, ptrdiff_t nargs,
                                     emacs_value args[],
                                     void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env, false);
  intmax_t pos = (nargs >= 1 && env->is_not_nil(env, args[0]))
                     ? env->extract_integer(env, args[0])
                     : get_point(env);
  bool module_only = (nargs >= 2 && env->is_not_nil(env, args[1]));

  uint32_t byte = bs.pos_to_byte(pos);
  auto d = find_enclosing_defun(bs, byte, module_only);
  if (!d) return elisp_nil(env);
  return defun_to_plist(env, bs, *d);
}

emacs_value Fverilog_ts_module_at_point(emacs_env *env, ptrdiff_t nargs,
                                        emacs_value args[],
                                        void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env, false);
  intmax_t pos = (nargs >= 1 && env->is_not_nil(env, args[0]))
                     ? env->extract_integer(env, args[0])
                     : get_point(env);
  uint32_t byte = bs.pos_to_byte(pos);
  auto d = find_enclosing_defun(bs, byte, true);
  if (!d) return elisp_nil(env);
  return defun_to_plist(env, bs, *d);
}

emacs_value Fverilog_ts_find_modules_in_buffer(emacs_env *env, ptrdiff_t nargs,
                                               emacs_value args[],
                                               void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env, false);
  bool all_defuns = (nargs >= 1 && env->is_not_nil(env, args[0]));
  std::vector<DefunInfo> defuns = find_all_defuns(bs, !all_defuns);
  std::vector<emacs_value> out;
  out.reserve(defuns.size());
  for (const auto &d : defuns) {
    out.push_back(defun_to_plist(env, bs, d));
  }
  return elisp_list(env, out);
}

emacs_value Fverilog_ts_block_bounds(emacs_env *env, ptrdiff_t nargs,
                                     emacs_value args[],
                                     void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env);
  intmax_t pos = (nargs >= 1 && env->is_not_nil(env, args[0]))
                     ? env->extract_integer(env, args[0])
                     : get_point(env);
  uint32_t byte = bs.pos_to_byte(pos);
  auto bi = find_block_at_byte(bs, byte);
  if (!bi) return elisp_nil(env);
  return elisp_list(env, {elisp_intern(env, ":type"),
                          elisp_string(env, bi->type),
                          elisp_intern(env, ":start"),
                          elisp_int(env, bs.byte_to_pos(bi->start_byte)),
                          elisp_intern(env, ":end-kwd"),
                          elisp_int(env, bs.byte_to_pos(bi->end_kwd_byte)),
                          elisp_intern(env, ":end"),
                          elisp_int(env, bs.byte_to_pos(bi->end_byte))});
}

emacs_value Fverilog_ts_leap_to_head(emacs_env *env, ptrdiff_t nargs,
                                     emacs_value args[],
                                     void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env, false);
  intmax_t pos = (nargs >= 1 && env->is_not_nil(env, args[0]))
                     ? env->extract_integer(env, args[0])
                     : get_point(env);
  uint32_t byte = bs.pos_to_byte(pos);
  auto head = leap_to_head_byte(bs, byte);
  if (!head) return elisp_nil(env);
  return elisp_int(env, bs.byte_to_pos(*head));
}

emacs_value Fverilog_ts_forward_sexp(emacs_env *env, ptrdiff_t nargs,
                                     emacs_value args[],
                                     void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env, false);
  intmax_t pos = (nargs >= 1 && env->is_not_nil(env, args[0]))
                     ? env->extract_integer(env, args[0])
                     : get_point(env);
  uint32_t byte = bs.pos_to_byte(pos);
  auto target = forward_sexp_byte(bs, byte);
  if (!target) return elisp_nil(env);
  return elisp_int(env, bs.byte_to_pos(*target));
}

emacs_value Fverilog_ts_backward_sexp(emacs_env *env, ptrdiff_t nargs,
                                      emacs_value args[],
                                      void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env, false);
  intmax_t pos = (nargs >= 1 && env->is_not_nil(env, args[0]))
                     ? env->extract_integer(env, args[0])
                     : get_point(env);
  uint32_t byte = bs.pos_to_byte(pos);
  auto target = backward_sexp_byte(bs, byte);
  if (!target) return elisp_nil(env);
  return elisp_int(env, bs.byte_to_pos(*target));
}

emacs_value Fverilog_ts_statement_bounds(emacs_env *env, ptrdiff_t nargs,
                                         emacs_value args[],
                                         void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env);
  intmax_t pos = (nargs >= 1 && env->is_not_nil(env, args[0]))
                     ? env->extract_integer(env, args[0])
                     : get_point(env);
  uint32_t byte = bs.pos_to_byte(pos);
  auto bounds = find_statement_bounds_bytes(bs, byte);
  return elisp_cons(env, elisp_int(env, bs.byte_to_pos(bounds.first)),
                    elisp_int(env, bs.byte_to_pos(bounds.second)));
}

emacs_value Fverilog_ts_in_context_p(emacs_env *env, ptrdiff_t nargs,
                                     emacs_value args[],
                                     void *data) EMACS_NOEXCEPT {
  if (nargs < 1 || !env->is_not_nil(env, args[0])) return elisp_nil(env);

  std::string ctx;
  emacs_value t0 = env->type_of(env, args[0]);
  if (env->eq(env, t0, env->intern(env, "symbol"))) {
    emacs_value s = env->funcall(env, env->intern(env, "symbol-name"), 1, &args[0]);
    ctx = extract_string(env, s);
  } else {
    ctx = extract_string(env, args[0]);
  }

  bool need_tree = (ctx != "attribute" && ctx != "protected" &&
                    ctx != "directive" && ctx != "parameter");
  BufferState &bs = get_buffer_state(env, need_tree);

  intmax_t pos = (nargs >= 2 && env->is_not_nil(env, args[1]))
                     ? env->extract_integer(env, args[1])
                     : get_point(env);
  uint32_t byte = bs.pos_to_byte(pos);
  intmax_t nested_col = -1;
  bool ok = check_in_context(bs, ctx, byte, &nested_col);
  if (!ok) return elisp_nil(env);
  if (ctx == "struct-nested" && nested_col >= 0) {
    return elisp_int(env, nested_col);
  }
  return elisp_t(env);
}

} // namespace

void register_nav_functions(emacs_env *env) {
  bind_function(
      env, "verilog-ts-defun-bounds", 0, 2, Fverilog_ts_defun_bounds,
      "Return plist (:type :name :start :kwd-pos :header-end :end) for enclosing defun at POS.");
  bind_function(
      env, "verilog-ts-module-at-point", 0, 1, Fverilog_ts_module_at_point,
      "Return plist for enclosing module/interface/program/package at POS.");
  bind_function(
      env, "verilog-ts-find-modules-in-buffer", 0, 1,
      Fverilog_ts_find_modules_in_buffer,
      "Return list of defun plists for all modules/interfaces/programs in buffer (or all defuns if ALL-DEFUNS).");
  bind_function(
      env, "verilog-ts-block-bounds", 0, 1, Fverilog_ts_block_bounds,
      "Return plist (:type :start :end-kwd :end) for the block at POS.");
  bind_function(
      env, "verilog-ts-leap-to-head", 0, 1, Fverilog_ts_leap_to_head,
      "Return position of matching block opener from a closing keyword at POS.");
  bind_function(
      env, "verilog-ts-forward-sexp", 0, 1, Fverilog_ts_forward_sexp,
      "Return position after moving forward across balanced block or sexp at POS.");
  bind_function(
      env, "verilog-ts-backward-sexp", 0, 1, Fverilog_ts_backward_sexp,
      "Return position before moving backward across balanced block or sexp at POS.");
  bind_function(
      env, "verilog-ts-statement-bounds", 0, 1, Fverilog_ts_statement_bounds,
      "Return (START . END) of statement at POS.");
  bind_function(
      env, "verilog-ts-in-context-p", 1, 2, Fverilog_ts_in_context_p,
      "Return non-nil if POS is inside CONTEXT (`struct', `struct-nested', `coverage', `case-region', `generate-region', `fork-region', `attribute', `parameter', `directive').");
}

} // namespace verilog_ts

#include "verilog_ts_common.hpp"

namespace verilog_ts {

namespace {

bool is_id_char(char c) {
  unsigned char uc = static_cast<unsigned char>(c);
  return std::isalnum(uc) || c == '_' || c == '$' || c == '`';
}

std::string strip_spaces(std::string_view sv) {
  std::string out;
  out.reserve(sv.size());
  for (char c : sv) {
    if (c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != '\f') {
      out.push_back(c);
    }
  }
  return out;
}

std::string trim_ws(std::string_view sv) {
  size_t b = 0;
  while (b < sv.size() &&
         (sv[b] == ' ' || sv[b] == '\t' || sv[b] == '\n' || sv[b] == '\r')) {
    ++b;
  }
  size_t e = sv.size();
  while (e > b && (sv[e - 1] == ' ' || sv[e - 1] == '\t' || sv[e - 1] == '\n' ||
                   sv[e - 1] == '\r')) {
    --e;
  }
  return std::string(sv.substr(b, e - b));
}

bool is_builtin_decl_type(std::string_view keywd) {
  static const std::unordered_set<std::string_view> kTypes = {
      "wire",      "reg",      "tri",      "tri0",     "tri1",    "triand",
      "trior",     "trireg",   "uwire",    "wand",     "wor",     "byte",
      "shortint",  "int",      "longint",  "integer",  "time",    "supply0",
      "supply1",   "bit",      "logic",    "shortreal","real",    "realtime",
      "string",    "event",    "chandle"};
  return kTypes.count(keywd) > 0;
}

// Convert SignalInfo to 9-element Elisp list (verilog-sig-new)
emacs_value signal_to_elisp(emacs_env *env, const SignalInfo &s) {
  emacs_value nil = elisp_nil(env);
  emacs_value multidim_val = nil;
  if (!s.multidim.empty()) {
    std::vector<emacs_value> md;
    md.reserve(s.multidim.size());
    for (const auto &m : s.multidim) {
      md.push_back(elisp_string(env, m));
    }
    multidim_val = elisp_list(env, md);
  }
  std::vector<emacs_value> elems = {
      elisp_string(env, s.name),
      s.bits ? elisp_string(env, *s.bits) : nil,
      s.comment ? elisp_string(env, *s.comment) : nil,
      s.memory ? elisp_string(env, *s.memory) : nil,
      s.enum_name ? elisp_string(env, *s.enum_name) : nil,
      s.signed_str ? elisp_string(env, *s.signed_str) : nil,
      s.type_str ? elisp_string(env, *s.type_str) : nil,
      multidim_val,
      s.modport ? elisp_string(env, *s.modport) : nil};
  return elisp_list(env, elems);
}

emacs_value signal_list_to_elisp(emacs_env *env,
                                 const std::vector<SignalInfo> &sigs) {
  std::vector<emacs_value> out;
  out.reserve(sigs.size());
  for (const auto &s : sigs) {
    out.push_back(signal_to_elisp(env, s));
  }
  return elisp_list(env, out);
}

emacs_value modports_to_elisp(emacs_env *env,
                              const std::vector<ModportInfo> &mps) {
  emacs_value nil = elisp_nil(env);
  std::vector<emacs_value> out;
  out.reserve(mps.size());
  for (const auto &mp : mps) {
    std::vector<emacs_value> clks;
    for (const auto &c : mp.clockings) {
      clks.push_back(elisp_string(env, c));
    }
    emacs_value mp_decls = elisp_vector(
        env, {signal_list_to_elisp(env, mp.outs),
              signal_list_to_elisp(env, mp.inouts),
              signal_list_to_elisp(env, mp.ins), nil, nil, nil, nil, nil, nil});
    out.push_back(elisp_list(
        env, {elisp_string(env, mp.name), elisp_list(env, clks), mp_decls}));
  }
  return elisp_list(env, out);
}

// Parse AUTO_CONSTANT(...) in [start_byte, end_byte)
std::vector<SignalInfo> extract_auto_constants(const BufferState &bs,
                                               uint32_t start_byte,
                                               uint32_t end_byte) {
  std::vector<SignalInfo> consts;
  std::string_view sv = bs.slice(start_byte, end_byte);
  size_t pos = 0;
  while ((pos = sv.find("AUTO_CONSTANT", pos)) != std::string_view::npos) {
    if (pos > 0 && is_id_char(sv[pos - 1])) {
      pos += 13;
      continue;
    }
    size_t i = pos + 13;
    while (i < sv.size() && (sv[i] == ' ' || sv[i] == '\t')) ++i;
    if (i < sv.size() && sv[i] == '(') {
      uint32_t open_b = start_byte + static_cast<uint32_t>(i);
      if (auto close_b = forward_sexp_byte(bs, open_b)) {
        std::string_view args_sv = bs.slice(open_b + 1, *close_b - 1);
        size_t k = 0;
        while (k < args_sv.size()) {
          while (k < args_sv.size() &&
                 (args_sv[k] == ' ' || args_sv[k] == '\t' ||
                  args_sv[k] == '\n' || args_sv[k] == '\r' ||
                  args_sv[k] == ',')) {
            ++k;
          }
          if (k >= args_sv.size()) break;
          size_t tok_s = k;
          while (k < args_sv.size() &&
                 (is_id_char(args_sv[k]) || args_sv[k] == '.' ||
                  args_sv[k] == '%' || args_sv[k] == '"')) {
            ++k;
          }
          if (k > tok_s) {
            SignalInfo si;
            si.name = std::string(args_sv.substr(tok_s, k - tok_s));
            consts.push_back(std::move(si));
          } else {
            ++k;
          }
        }
        pos = (*close_b) - start_byte;
        continue;
      }
    }
    pos = i;
  }
  return consts;
}

// Check if comment contains `auto enum <NAME>` or `synopsys enum <NAME>`
std::optional<std::string> check_comment_for_enum(std::string_view cmt) {
  size_t line_end = cmt.find('\n');
  std::string_view first_line =
      (line_end == std::string_view::npos) ? cmt : cmt.substr(0, line_end);
  for (std::string_view kw : {"auto", "synopsys"}) {
    size_t pos = 0;
    while ((pos = first_line.find(kw, pos)) != std::string_view::npos) {
      size_t i = pos + kw.size();
      if (i < first_line.size() &&
          (first_line[i] == ' ' || first_line[i] == '\t')) {
        while (i < first_line.size() &&
               (first_line[i] == ' ' || first_line[i] == '\t')) {
          ++i;
        }
        if (first_line.substr(i, 4) == "enum" && i + 4 < first_line.size() &&
            (first_line[i + 4] == ' ' || first_line[i + 4] == '\t')) {
          i += 4;
          while (i < first_line.size() &&
                 (first_line[i] == ' ' || first_line[i] == '\t')) {
            ++i;
          }
          size_t ns = i;
          while (i < first_line.size() &&
                 (std::isalnum(static_cast<unsigned char>(first_line[i])) ||
                  first_line[i] == '_')) {
            ++i;
          }
          if (i > ns) {
            return std::string(first_line.substr(ns, i - ns));
          }
        }
      }
      pos += kw.size();
    }
  }
  return std::nullopt;
}

} // namespace

ModuleDecls extract_module_decls(const BufferState &bs, uint32_t start_byte,
                                 uint32_t end_byte,
                                 const std::string &typedef_regexp,
                                 bool case_fold) {
  ModuleDecls decls;
  uint32_t n = std::min<uint32_t>(end_byte,
                                  static_cast<uint32_t>(bs.text.size()));
  if (start_byte >= n) return decls;

  decls.consts = extract_auto_constants(bs, start_byte, n);

  std::optional<std::regex> td_re;
  if (!typedef_regexp.empty()) {
    try {
      auto flags = std::regex::ECMAScript;
      if (case_fold) flags |= std::regex::icase;
      td_re.emplace(typedef_regexp, flags);
    } catch (...) {
      // Fallback if Emacs regex syntax differs from ECMAScript
    }
  }

  auto matches_typedef = [&](const std::string &word) -> bool {
    if (typedef_regexp.empty()) return false;
    if (td_re.has_value()) {
      try {
        return std::regex_search(word, *td_re);
      } catch (...) {
      }
    }
    // Common default pattern `_t$`
    if (typedef_regexp == "_t$" && word.size() >= 2 &&
        word.substr(word.size() - 2) == "_t") {
      return true;
    }
    return false;
  };

  enum class ExpectTarget {
    NONE,
    IN,
    OUT,
    INOUT,
    VAR,
    ASSIGN,
    CONST,
    GPARAM,
    INTF
  };

  int functask = 0;
  int paren = 0;
  int sig_paren = 0;
  bool v2kargs_ok = true;
  bool in_ign_to_semi = false;
  bool ptype = false;
  bool ign_prop = false;
  bool rvalue = false;
  bool io = false;
  ExpectTarget expect_signal = ExpectTarget::NONE;

  std::optional<std::string> vec;
  std::optional<std::string> enum_name;
  std::optional<std::string> signed_str;
  std::optional<std::string> typedefed;
  std::optional<std::string> modport;
  std::vector<std::string> multidim;
  SignalInfo *newsig = nullptr;

  bool in_modport_flag = false;
  std::optional<ModportInfo> active_modport;
  bool in_clocking_flag = false;
  std::optional<ModportInfo> active_clocking;

  struct SavedSigs {
    std::vector<SignalInfo> outs;
    std::vector<SignalInfo> inouts;
    std::vector<SignalInfo> ins;
  };
  std::vector<SavedSigs> varstack;

  std::string keywd;
  std::string last_keywd;

  uint32_t i = start_byte;
  while (i < n) {
    // Skip whitespace
    while (i < n && (bs.text[i] == ' ' || bs.text[i] == '\t' ||
                     bs.text[i] == '\n' || bs.text[i] == '\r' ||
                     bs.text[i] == '\f')) {
      ++i;
    }
    if (i >= n) break;

    // Skip protected regions (`protected .. `endprotected and `pragma protect begin_protected .. end_protected)
    bool skipped_prot = false;
    for (const auto &ps : bs.protected_spans) {
      if (i >= ps.start_byte && i < ps.end_byte) {
        i = std::min<uint32_t>(ps.end_byte, n);
        skipped_prot = true;
        break;
      }
    }
    if (skipped_prot) continue;

    // Check comments & strings via spans
    if (const CommentStringSpan *sp = bs.find_span_at_byte(i, false)) {
      if (sp->kind == SpanKind::LINE_COMMENT ||
          sp->kind == SpanKind::BLOCK_COMMENT) {
        auto en = check_comment_for_enum(bs.slice(sp->start_byte, sp->end_byte));
        if (en) enum_name = *en;
      }
      i = std::min<uint32_t>(sp->end_byte, n);
      continue;
    }

    // Check (* ... *) attributes
    if (bs.text[i] == '(' && i + 1 < n && bs.text[i + 1] == '*') {
      uint32_t j = i + 2;
      while (j < n && (bs.text[j] == ' ' || bs.text[j] == '\t')) ++j;
      if (j < n && bs.text[j] == ')') {
        i = j + 1;
        continue;
      }
      while (j + 1 < n && !(bs.text[j] == '*' && bs.text[j + 1] == ')')) ++j;
      i = (j + 1 < n) ? (j + 2) : n;
      continue;
    }

    char c = bs.text[i];
    if (c == ';') {
      if (in_ign_to_semi) {
        in_ign_to_semi = false;
        rvalue = false;
      } else if (active_modport.has_value()) {
        active_modport->outs = std::move(decls.outs);
        active_modport->inouts = std::move(decls.inouts);
        active_modport->ins = std::move(decls.ins);
        decls.modports.push_back(std::move(*active_modport));
        active_modport.reset();
        in_modport_flag = false;
        if (!varstack.empty()) {
          decls.outs = std::move(varstack.back().outs);
          decls.inouts = std::move(varstack.back().inouts);
          decls.ins = std::move(varstack.back().ins);
          varstack.pop_back();
        }
        vec.reset();
        io = false;
        expect_signal = ExpectTarget::NONE;
        newsig = nullptr;
        paren = 0;
        rvalue = false;
        v2kargs_ok = false;
        ign_prop = false;
      } else {
        vec.reset();
        io = false;
        expect_signal = ExpectTarget::NONE;
        newsig = nullptr;
        paren = 0;
        rvalue = false;
        v2kargs_ok = false;
        in_modport_flag = false;
        ign_prop = false;
      }
      ++i;
      continue;
    }

    if (c == '=') {
      rvalue = true;
      newsig = nullptr;
      ++i;
      continue;
    }

    if (c == ',' && paren == sig_paren) {
      rvalue = false;
      ++i;
      continue;
    }

    if (c == '{' || c == '(') {
      ++paren;
      ++i;
      continue;
    }

    if (c == '}' || c == ')') {
      --paren;
      ++i;
      if (paren < sig_paren) {
        expect_signal = ExpectTarget::NONE;
        rvalue = false;
      }
      continue;
    }

    if (c == '[') {
      if (auto close_b = forward_sexp_byte(bs, i)) {
        std::string raw_br(bs.slice(i, *close_b));
        keywd = raw_br;
        if (newsig) {
          if (newsig->memory) {
            *newsig->memory += raw_br;
          } else {
            newsig->memory = raw_br;
          }
        } else if (vec.has_value()) {
          multidim.insert(multidim.begin(), *vec);
          vec = strip_spaces(raw_br);
        } else {
          vec = strip_spaces(raw_br);
        }
        i = *close_b;
        continue;
      }
      ++i;
      continue;
    }

    if (c == '\'' && !rvalue) {
      ++i;
      expect_signal = ExpectTarget::NONE;
      rvalue = false;
      continue;
    }

    if (is_id_char(c) || c == '\\') {
      uint32_t tend = i;
      std::string tok;
      if (c == '\\') {
        ++tend;
        while (tend < n && bs.text[tend] != ' ' && bs.text[tend] != '\t' &&
               bs.text[tend] != '\n' && bs.text[tend] != '\r' &&
               bs.text[tend] != '\f') {
          ++tend;
        }
        tok = std::string(bs.slice(i, tend)) + " ";
      } else {
        while (tend < n && is_id_char(bs.text[tend])) ++tend;
        tok = std::string(bs.slice(i, tend));
      }
      i = tend;

      // Append any :: package scope
      while (true) {
        uint32_t j = i;
        while (j < n && (bs.text[j] == ' ' || bs.text[j] == '\t')) ++j;
        if (j + 1 < n && bs.text[j] == ':' && bs.text[j + 1] == ':') {
          j += 2;
          while (j < n && (bs.text[j] == ' ' || bs.text[j] == '\t')) ++j;
          if (j < n && bs.text[j] == '*') {
            tok += "::*";
            i = j + 1;
          } else if (j < n && bs.text[j] == '\\') {
            uint32_t k = j + 1;
            while (k < n && bs.text[k] != ' ' && bs.text[k] != '\t' &&
                   bs.text[k] != '\n' && bs.text[k] != '\r' &&
                   bs.text[k] != '\f') {
              ++k;
            }
            tok += "::" + std::string(bs.slice(j, k)) + " ";
            i = k;
          } else if (j < n && is_id_char(bs.text[j])) {
            uint32_t k = j;
            while (k < n && is_id_char(bs.text[k])) ++k;
            tok += "::" + std::string(bs.slice(j, k));
            i = k;
          } else {
            break;
          }
        } else {
          break;
        }
      }

      last_keywd = keywd;
      keywd = tok;

      if (keywd == "input") {
        vec.reset();
        enum_name.reset();
        rvalue = false;
        newsig = nullptr;
        signed_str.reset();
        typedefed.reset();
        multidim.clear();
        ptype = false;
        modport.reset();
        expect_signal = ExpectTarget::IN;
        io = true;
        sig_paren = paren;
      } else if (keywd == "output") {
        vec.reset();
        enum_name.reset();
        rvalue = false;
        newsig = nullptr;
        signed_str.reset();
        typedefed.reset();
        multidim.clear();
        ptype = false;
        modport.reset();
        expect_signal = ExpectTarget::OUT;
        io = true;
        sig_paren = paren;
      } else if (keywd == "inout") {
        vec.reset();
        enum_name.reset();
        rvalue = false;
        newsig = nullptr;
        signed_str.reset();
        typedefed.reset();
        multidim.clear();
        ptype = false;
        modport.reset();
        expect_signal = ExpectTarget::INOUT;
        io = true;
        sig_paren = paren;
      } else if (keywd == "parameter") {
        vec.reset();
        enum_name.reset();
        rvalue = false;
        signed_str.reset();
        typedefed.reset();
        multidim.clear();
        ptype = false;
        modport.reset();
        expect_signal = ExpectTarget::GPARAM;
        io = true;
        sig_paren = paren;
      } else if (is_builtin_decl_type(keywd)) {
        if (io) {
          typedefed = typedefed ? (*typedefed + " " + keywd) : keywd;
        } else {
          vec.reset();
          enum_name.reset();
          rvalue = false;
          signed_str.reset();
          typedefed.reset();
          multidim.clear();
          sig_paren = paren;
          expect_signal = ExpectTarget::VAR;
          modport.reset();
        }
      } else if (keywd == "assign") {
        vec.reset();
        enum_name.reset();
        rvalue = false;
        signed_str.reset();
        typedefed.reset();
        multidim.clear();
        ptype = false;
        modport.reset();
        expect_signal = ExpectTarget::ASSIGN;
        sig_paren = paren;
      } else if (keywd == "localparam" || keywd == "genvar") {
        vec.reset();
        enum_name.reset();
        rvalue = false;
        signed_str.reset();
        typedefed.reset();
        multidim.clear();
        ptype = false;
        modport.reset();
        expect_signal = ExpectTarget::CONST;
        sig_paren = paren;
      } else if (keywd == "signed" || keywd == "unsigned") {
        signed_str = keywd;
      } else if (keywd == "assert" || keywd == "assume" || keywd == "cover" ||
                 keywd == "expect" || keywd == "restrict") {
        ign_prop = true;
      } else if (keywd == "class" || keywd == "covergroup" ||
                 keywd == "function" || keywd == "property" ||
                 keywd == "randsequence" || keywd == "sequence" ||
                 keywd == "task") {
        if (!ign_prop) ++functask;
      } else if (keywd == "endclass" || keywd == "endgroup" ||
                 keywd == "endfunction" || keywd == "endproperty" ||
                 keywd == "endsequence" || keywd == "endtask") {
        --functask;
      } else if (keywd == "modport") {
        in_modport_flag = true;
      } else if (keywd == "clocking" && last_keywd != "default") {
        in_clocking_flag = true;
      } else if (keywd == "import") {
        if (v2kargs_ok) {
          in_ign_to_semi = true;
          rvalue = true;
        }
      } else if (keywd == "type") {
        ptype = true;
      } else if (keywd == "var") {
        // no-op
      } else if (keywd == "`ifdef" || keywd == "`ifndef" || keywd == "`elsif") {
        rvalue = true;
      } else if (keywd == "`line") {
        while (i < n && bs.text[i] != '\n') ++i;
        if (i < n) ++i;
      } else if (!ptype && matches_typedef(keywd)) {
        if (io) {
          typedefed = typedefed ? (*typedefed + " " + keywd) : keywd;
        } else {
          vec.reset();
          enum_name.reset();
          rvalue = false;
          signed_str.reset();
          typedefed = keywd;
          multidim.clear();
          sig_paren = paren;
          expect_signal = ExpectTarget::VAR;
          modport.reset();
        }
      } else {
        // Check for interface with optional modport in v2k arglist
        uint32_t j = i;
        while (j < n && (bs.text[j] == ' ' || bs.text[j] == '\t' ||
                         bs.text[j] == '\n' || bs.text[j] == '\r')) {
          ++j;
        }
        bool is_v2k_intf = false;
        std::optional<std::string> found_mp;
        if (v2kargs_ok && paren == 1 && !rvalue) {
          if (j < n && bs.text[j] == '#') {
            is_v2k_intf = true;
          } else if (j < n && bs.text[j] == '.') {
            uint32_t k = j + 1;
            while (k < n && (bs.text[k] == ' ' || bs.text[k] == '\t')) ++k;
            uint32_t mp_s = k;
            while (k < n && is_id_char(bs.text[k])) ++k;
            if (k > mp_s) {
              found_mp = std::string(bs.slice(mp_s, k));
              uint32_t m = k;
              while (m < n && (bs.text[m] == ' ' || bs.text[m] == '\t' ||
                               bs.text[m] == '\n' || bs.text[m] == '\r')) {
                ++m;
              }
              if (m < n && is_id_char(bs.text[m]) &&
                  !std::isdigit(static_cast<unsigned char>(bs.text[m]))) {
                is_v2k_intf = true;
                i = k;
              }
            }
          } else if (j < n && is_id_char(bs.text[j]) &&
                     !std::isdigit(static_cast<unsigned char>(bs.text[j])) &&
                     !is_verilog_keyword(keywd)) {
            is_v2k_intf = true;
          }
        }

        if (is_v2k_intf) {
          vec.reset();
          enum_name.reset();
          rvalue = false;
          signed_str.reset();
          typedefed = keywd;
          multidim.clear();
          ptype = false;
          modport = found_mp;
          newsig = nullptr;
          sig_paren = paren;
          expect_signal = ExpectTarget::INTF;
          io = true;
        } else if (j < n && bs.text[j] == '.') {
          // Dotted LHS assignment: assign foo.bar = z;
          i = j + 1;
          if (!rvalue) expect_signal = ExpectTarget::NONE;
        } else if (in_modport_flag && !active_modport.has_value() &&
                   !is_verilog_keyword(keywd)) {
          ModportInfo mp;
          mp.name = keywd;
          active_modport = std::move(mp);
          varstack.push_back(
              {std::move(decls.outs), std::move(decls.inouts), std::move(decls.ins)});
          decls.outs.clear();
          decls.inouts.clear();
          decls.ins.clear();
        } else if (active_modport.has_value() && in_clocking_flag) {
          active_modport->clockings.insert(active_modport->clockings.begin(),
                                           keywd);
          in_clocking_flag = false;
        } else if (in_clocking_flag && keywd == "endclocking") {
          if (active_clocking.has_value()) {
            active_clocking->outs = std::move(decls.outs);
            active_clocking->inouts = std::move(decls.inouts);
            active_clocking->ins = std::move(decls.ins);
            decls.modports.push_back(std::move(*active_clocking));
            active_clocking.reset();
            if (!varstack.empty()) {
              decls.outs = std::move(varstack.back().outs);
              decls.inouts = std::move(varstack.back().inouts);
              decls.ins = std::move(varstack.back().ins);
              varstack.pop_back();
            }
          }
          in_clocking_flag = false;
        } else if (in_clocking_flag && !active_clocking.has_value() &&
                   !is_verilog_keyword(keywd)) {
          ModportInfo clk;
          clk.name = keywd;
          active_clocking = std::move(clk);
          varstack.push_back(
              {std::move(decls.outs), std::move(decls.inouts), std::move(decls.ins)});
          decls.outs.clear();
          decls.inouts.clear();
          decls.ins.clear();
        } else if (expect_signal != ExpectTarget::NONE && !rvalue &&
                   functask == 0 && !is_verilog_keyword(keywd) &&
                   (!io || paren == sig_paren)) {
          SignalInfo sig;
          sig.name = keywd;
          sig.bits = vec;
          sig.enum_name = enum_name;
          sig.signed_str = signed_str;
          sig.type_str = typedefed;
          sig.multidim = multidim;
          sig.modport = modport;

          std::vector<SignalInfo> *target_vec = nullptr;
          switch (expect_signal) {
            case ExpectTarget::IN:
              target_vec = &decls.ins;
              break;
            case ExpectTarget::OUT:
              target_vec = &decls.outs;
              break;
            case ExpectTarget::INOUT:
              target_vec = &decls.inouts;
              break;
            case ExpectTarget::VAR:
              target_vec = &decls.vars;
              break;
            case ExpectTarget::ASSIGN:
              target_vec = &decls.assigns;
              break;
            case ExpectTarget::CONST:
              target_vec = &decls.consts;
              break;
            case ExpectTarget::GPARAM:
              target_vec = &decls.gparams;
              break;
            case ExpectTarget::INTF:
              target_vec = &decls.interfaces;
              break;
            default:
              break;
          }
          if (target_vec) {
            target_vec->push_back(std::move(sig));
            newsig = &target_vec->back();
          }
        }
      }
      continue;
    }

    ++i;
  }

  return decls;
}

InstInfo extract_inst_at_byte(const BufferState &bs, uint32_t byte) {
  InstInfo info;
  uint32_t n = static_cast<uint32_t>(bs.text.size());
  if (byte > n) byte = n;

  // First try TSTree module_instantiation / interface_instantiation / program_instantiation / gate_instantiation
  if (bs.tree && byte < n) {
    TSNode root = ts_tree_root_node(bs.tree);
    TSNode cur = ts_node_descendant_for_byte_range(root, byte, byte);
    while (!ts_node_is_null(cur)) {
      std::string_view nt = ts_node_type(cur);
      if (nt == "module_instantiation" || nt == "interface_instantiation" ||
          nt == "program_instantiation" || nt == "checker_instantiation" ||
          nt == "udp_instantiation") {
        info.start_byte = ts_node_start_byte(cur);
        info.end_byte = ts_node_end_byte(cur);
        TSNode itype = ts_node_child_by_field_name(cur, "instance_type", 13);
        if (!ts_node_is_null(itype)) {
          info.module_start_byte = ts_node_start_byte(itype);
          info.module_end_byte = ts_node_end_byte(itype);
          info.module_name = std::string(bs.node_text(itype));
        }
        uint32_t ccount = ts_node_named_child_count(cur);
        for (uint32_t i = 0; i < ccount; ++i) {
          TSNode ch = ts_node_named_child(cur, i);
          std::string_view ct = ts_node_type(ch);
          if (ct == "parameter_value_assignment") {
            uint32_t pc = ts_node_named_child_count(ch);
            for (uint32_t j = 0; j < pc; ++j) {
              TSNode list_n = ts_node_named_child(ch, j);
              if (std::string_view(ts_node_type(list_n)) ==
                  "list_of_parameter_value_assignments") {
                uint32_t ac = ts_node_named_child_count(list_n);
                for (uint32_t k = 0; k < ac; ++k) {
                  TSNode pa = ts_node_named_child(list_n, k);
                  if (std::string_view(ts_node_type(pa)) ==
                      "named_parameter_assignment") {
                    std::string pname;
                    std::string pval;
                    uint32_t pac = ts_node_named_child_count(pa);
                    for (uint32_t m = 0; m < pac; ++m) {
                      TSNode sub = ts_node_named_child(pa, m);
                      std::string_view st = ts_node_type(sub);
                      if (pname.empty() &&
                          (st == "simple_identifier" ||
                           st == "escaped_identifier" ||
                           st == "parameter_identifier")) {
                        pname = std::string(bs.node_text(sub));
                      } else if (st == "param_expression" ||
                                 st == "mintypmax_expression" ||
                                 st == "data_type") {
                        pval = strip_spaces(bs.node_text(sub));
                      }
                    }
                    if (!pname.empty()) {
                      info.params.emplace_back(std::move(pname),
                                               std::move(pval));
                    }
                  }
                }
              }
            }
          } else if (ct == "hierarchical_instance") {
            uint32_t hc = ts_node_named_child_count(ch);
            for (uint32_t j = 0; j < hc; ++j) {
              TSNode hsub = ts_node_named_child(ch, j);
              std::string_view hst = ts_node_type(hsub);
              if (hst == "name_of_instance") {
                TSNode iname =
                    ts_node_child_by_field_name(hsub, "instance_name", 13);
                if (!ts_node_is_null(iname)) {
                  info.inst_start_byte = ts_node_start_byte(iname);
                  info.inst_end_byte = ts_node_end_byte(iname);
                  info.inst_name = std::string(bs.node_text(iname));
                }
              } else if (hst == "list_of_port_connections") {
                uint32_t pcount = ts_node_named_child_count(hsub);
                for (uint32_t k = 0; k < pcount; ++k) {
                  TSNode pconn = ts_node_named_child(hsub, k);
                  if (std::string_view(ts_node_type(pconn)) ==
                      "named_port_connection") {
                    TSNode pname_n =
                        ts_node_child_by_field_name(pconn, "port_name", 9);
                    if (!ts_node_is_null(pname_n)) {
                      std::string pname(bs.node_text(pname_n));
                      std::string pexpr;
                      uint32_t npc = ts_node_named_child_count(pconn);
                      for (uint32_t m = 0; m < npc; ++m) {
                        TSNode nch = ts_node_named_child(pconn, m);
                        if (!ts_node_eq(nch, pname_n)) {
                          pexpr = trim_ws(bs.node_text(nch));
                        }
                      }
                      info.ports.emplace_back(std::move(pname),
                                              std::move(pexpr));
                    }
                  }
                }
              }
            }
          }
        }
        if (!info.module_name.empty() && !info.inst_name.empty()) {
          info.valid = true;
          return info;
        }
      }
      cur = ts_node_parent(cur);
    }
  }

  // Fallback token-level backward scan from `byte` (handles `SUBMOD_A `SUBNAME_B (/*AUTOINST*/) and unclosed parens)
  int depth = 0;
  uint32_t open_paren = byte;
  while (open_paren > 0) {
    --open_paren;
    if (bs.is_inside_comment_or_string_byte(open_paren, false)) continue;
    if (bs.text[open_paren] == ')') {
      ++depth;
    } else if (bs.text[open_paren] == '(') {
      if (depth == 0) break;
      --depth;
    }
  }
  if (open_paren >= n || bs.text[open_paren] != '(') return info;
  info.paren_start_byte = open_paren;
  if (auto close_p = forward_sexp_byte(bs, open_paren)) {
    info.paren_end_byte = *close_p;
  }

  // Step backward before any [range] on instance name
  uint32_t p = backward_syntactic_ws_bytes(bs, open_paren, 0, false);
  while (p > 0 && bs.text[p - 1] == ']') {
    if (auto open_br = backward_sexp_byte(bs, p)) {
      p = backward_syntactic_ws_bytes(bs, *open_br, 0, false);
    } else {
      break;
    }
  }
  uint32_t inst_end = p;
  while (p > 0 && is_id_char(bs.text[p - 1])) --p;
  uint32_t inst_start = p;
  if (inst_start == inst_end) return info;
  info.inst_start_byte = inst_start;
  info.inst_end_byte = inst_end;
  info.inst_name = std::string(bs.slice(inst_start, inst_end));

  // Step backward before optional #(params)
  p = backward_syntactic_ws_bytes(bs, inst_start, 0, false);
  if (p > 0 && bs.text[p - 1] == ')') {
    if (auto param_open = backward_sexp_byte(bs, p)) {
      // Parse .PARAM(VAL) inside #( ... )
      std::string_view p_sv = bs.slice(*param_open + 1, p - 1);
      size_t idx = 0;
      while ((idx = p_sv.find('.', idx)) != std::string_view::npos) {
        size_t k = idx + 1;
        while (k < p_sv.size() && (p_sv[k] == ' ' || p_sv[k] == '\t')) ++k;
        size_t ns = k;
        while (k < p_sv.size() && is_id_char(p_sv[k])) ++k;
        std::string pname(p_sv.substr(ns, k - ns));
        while (k < p_sv.size() && p_sv[k] != '(') ++k;
        if (k < p_sv.size() && p_sv[k] == '(') {
          uint32_t val_open = *param_open + 1 + static_cast<uint32_t>(k);
          if (auto val_close = forward_sexp_byte(bs, val_open)) {
            std::string pval =
                strip_spaces(bs.slice(val_open + 1, *val_close - 1));
            if (!pname.empty()) {
              info.params.emplace_back(std::move(pname), std::move(pval));
            }
            idx = (*val_close) - (*param_open + 1);
            continue;
          }
        }
        idx = k;
      }
      p = backward_syntactic_ws_bytes(bs, *param_open, 0, false);
      if (p > 0 && bs.text[p - 1] == '#') {
        p = backward_syntactic_ws_bytes(bs, p - 1, 0, false);
      }
    }
  } else {
    // Check for gate delay `#1`
    uint32_t tmp_e = p;
    while (p > 0 && is_id_char(bs.text[p - 1])) --p;
    uint32_t before_num = backward_syntactic_ws_bytes(bs, p, 0, false);
    if (before_num > 0 && bs.text[before_num - 1] == '#') {
      p = backward_syntactic_ws_bytes(bs, before_num - 1, 0, false);
    } else {
      p = tmp_e;
    }
  }

  uint32_t mod_end = p;
  while (p > 0 && is_id_char(bs.text[p - 1])) --p;
  uint32_t mod_start = p;
  if (mod_start < mod_end) {
    info.module_start_byte = mod_start;
    info.module_end_byte = mod_end;
    info.module_name = std::string(bs.slice(mod_start, mod_end));
    info.start_byte = mod_start;
    info.end_byte =
        (info.paren_end_byte > 0) ? info.paren_end_byte : info.inst_end_byte;
    info.valid = true;
  }
  return info;
}

std::vector<InstInfo> extract_all_insts(const BufferState &bs,
                                        uint32_t start_byte,
                                        uint32_t end_byte) {
  std::vector<InstInfo> results;
  if (!bs.tree) return results;

  uint32_t err_off = 0;
  TSQueryError err_type = TSQueryErrorNone;
  TSQuery *q = get_or_compile_query(
      "[(module_instantiation) (interface_instantiation) (program_instantiation)] @inst",
      &err_off, &err_type);
  if (q) {
    TSQueryCursor *cursor = ts_query_cursor_new();
    ts_query_cursor_set_byte_range(cursor, start_byte, end_byte);
    ts_query_cursor_exec(cursor, q, ts_tree_root_node(bs.tree));
    TSQueryMatch match;
    while (ts_query_cursor_next_match(cursor, &match)) {
      for (uint16_t i = 0; i < match.capture_count; ++i) {
        TSNode n = match.captures[i].node;
        uint32_t sb = ts_node_start_byte(n);
        if (sb >= start_byte && sb < end_byte &&
            !bs.is_inside_protected_byte(sb)) {
          InstInfo inf = extract_inst_at_byte(bs, sb);
          if (inf.valid && !is_verilog_keyword(inf.module_name)) {
            results.push_back(std::move(inf));
          }
        }
      }
    }
    ts_query_cursor_delete(cursor);
  }
  return results;
}

AlwaysSignals extract_always_signals(const BufferState &bs, uint32_t start_byte,
                                     uint32_t end_byte) {
  AlwaysSignals res;
  if (!bs.tree) return res;
  uint32_t n = std::min<uint32_t>(end_byte,
                                  static_cast<uint32_t>(bs.text.size()));
  if (start_byte >= n) return res;

  TSNode root = ts_tree_root_node(bs.tree);
  TSNode alw = ts_node_descendant_for_byte_range(root, start_byte, start_byte);
  while (!ts_node_is_null(alw) &&
         std::string_view(ts_node_type(alw)) != "always_construct") {
    alw = ts_node_parent(alw);
  }
  if (!ts_node_is_null(alw)) {
    start_byte = ts_node_start_byte(alw);
    n = ts_node_end_byte(alw);
  }

  std::unordered_set<std::string> seen_d, seen_i, seen_t, seen_in;
  auto add_unique = [](std::vector<std::string> &vec,
                       std::unordered_set<std::string> &seen,
                       const std::string &s) {
    if (!s.empty() && !is_verilog_keyword(s) && s[0] != '$' &&
        !std::isdigit(static_cast<unsigned char>(s[0])) && seen.insert(s).second) {
      vec.push_back(s);
    }
  };

  // Walk AST nodes inside `alw` (or region) to classify nonblocking LHS, blocking LHS, for-init temps, and RHS inputs
  if (!ts_node_is_null(alw)) {
    std::vector<TSNode> stack = {alw};
    while (!stack.empty()) {
      TSNode cur = stack.back();
      stack.pop_back();
      std::string_view nt = ts_node_type(cur);

      if (nt == "event_control") {
        // Sensitivity list @(...) - skip so we don't treat old sensitivity list as RHS inputs
        continue;
      }
      if (nt == "for_initialization" || nt == "for_step") {
        // Extract loop variable as temp
        uint32_t sb = ts_node_start_byte(cur);
        uint32_t eb = ts_node_end_byte(cur);
        std::string_view sv = bs.slice(sb, eb);
        size_t eq = sv.find('=');
        if (eq != std::string_view::npos) {
          std::string lhs = trim_ws(sv.substr(0, eq));
          add_unique(res.temps, seen_t, lhs);
        }
        continue;
      }
      if (nt == "nonblocking_assignment") {
        TSNode lval = ts_node_named_child(cur, 0);
        if (!ts_node_is_null(lval)) {
          std::string_view lv_sv = bs.node_text(lval);
          size_t br = lv_sv.find('[');
          std::string base_id =
              trim_ws(br == std::string_view::npos ? lv_sv : lv_sv.substr(0, br));
          add_unique(res.out_delayed, seen_d, base_id);
        }
      } else if (nt == "blocking_assignment" || nt == "operator_assignment") {
        TSNode lval = ts_node_named_child(cur, 0);
        if (!ts_node_is_null(lval)) {
          std::string_view lv_sv = bs.node_text(lval);
          size_t br = lv_sv.find('[');
          std::string base_id =
              trim_ws(br == std::string_view::npos ? lv_sv : lv_sv.substr(0, br));
          add_unique(res.out_immediate, seen_i, base_id);
        }
      } else if (nt == "expression" || nt == "primary") {
        if (ts_node_named_child_count(cur) == 1) {
          TSNode ch = ts_node_named_child(cur, 0);
          std::string_view cht = ts_node_type(ch);
          if (cht == "simple_identifier" || cht == "hierarchical_identifier") {
            // Check that `cur` is not the direct variable_lvalue of an assignment
            TSNode p = ts_node_parent(cur);
            bool is_lhs = false;
            while (!ts_node_is_null(p) && p.id != alw.id) {
              if (std::string_view(ts_node_type(p)) == "variable_lvalue") {
                is_lhs = true;
                break;
              }
              if (std::string_view(ts_node_type(p)) == "select" ||
                  std::string_view(ts_node_type(p)) == "bit_select") {
                is_lhs = false;
                break;
              }
              p = ts_node_parent(p);
            }
            if (!is_lhs) {
              add_unique(res.inputs, seen_in, std::string(bs.node_text(ch)));
            }
          }
        }
      }

      uint32_t cc = ts_node_named_child_count(cur);
      for (uint32_t i = cc; i > 0; --i) {
        stack.push_back(ts_node_named_child(cur, i - 1));
      }
    }
  }
  return res;
}

namespace {

emacs_value Fverilog_ts_read_decls(emacs_env *env, ptrdiff_t nargs,
                                   emacs_value args[],
                                   void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env);
  uint32_t start_byte = 0;
  uint32_t end_byte = static_cast<uint32_t>(bs.text.size());

  if (nargs >= 1 && env->is_not_nil(env, args[0])) {
    start_byte = bs.pos_to_byte(env->extract_integer(env, args[0]));
  } else {
    auto d = find_enclosing_defun(bs, bs.pos_to_byte(get_point(env)), true);
    if (d) {
      start_byte = d->start_byte;
      end_byte = d->end_byte;
    }
  }
  if (nargs >= 2 && env->is_not_nil(env, args[1])) {
    end_byte = bs.pos_to_byte(env->extract_integer(env, args[1]));
  }
  std::string td_re;
  if (nargs >= 3 && env->is_not_nil(env, args[2])) {
    td_re = extract_string(env, args[2]);
  }
  bool case_fold = (nargs >= 4 && env->is_not_nil(env, args[3]));

  ModuleDecls d =
      extract_module_decls(bs, start_byte, end_byte, td_re, case_fold);

  std::vector<emacs_value> vec_elems = {
      signal_list_to_elisp(env, d.outs),
      signal_list_to_elisp(env, d.inouts),
      signal_list_to_elisp(env, d.ins),
      signal_list_to_elisp(env, d.vars),
      modports_to_elisp(env, d.modports),
      signal_list_to_elisp(env, d.assigns),
      signal_list_to_elisp(env, d.consts),
      signal_list_to_elisp(env, d.gparams),
      signal_list_to_elisp(env, d.interfaces)};
  return elisp_vector(env, vec_elems);
}

emacs_value Fverilog_ts_ast_decls(emacs_env *env, ptrdiff_t nargs,
                                  emacs_value args[],
                                  void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env);
  uint32_t start_byte = 0;
  uint32_t end_byte = static_cast<uint32_t>(bs.text.size());

  if (nargs >= 1 && env->is_not_nil(env, args[0])) {
    start_byte = bs.pos_to_byte(env->extract_integer(env, args[0]));
  } else {
    auto d = find_enclosing_defun(bs, bs.pos_to_byte(get_point(env)), true);
    if (d) {
      start_byte = d->start_byte;
      end_byte = d->end_byte;
    }
  }
  if (nargs >= 2 && env->is_not_nil(env, args[1])) {
    end_byte = bs.pos_to_byte(env->extract_integer(env, args[1]));
  }
  std::string td_re;
  if (nargs >= 3 && env->is_not_nil(env, args[2])) {
    td_re = extract_string(env, args[2]);
  }
  bool case_fold = (nargs >= 4 && env->is_not_nil(env, args[3]));

  ModuleDecls d =
      extract_module_decls(bs, start_byte, end_byte, td_re, case_fold);

  return elisp_list(env, {elisp_intern(env, ":outputs"),
                          signal_list_to_elisp(env, d.outs),
                          elisp_intern(env, ":inouts"),
                          signal_list_to_elisp(env, d.inouts),
                          elisp_intern(env, ":inputs"),
                          signal_list_to_elisp(env, d.ins),
                          elisp_intern(env, ":vars"),
                          signal_list_to_elisp(env, d.vars),
                          elisp_intern(env, ":modports"),
                          modports_to_elisp(env, d.modports),
                          elisp_intern(env, ":assigns"),
                          signal_list_to_elisp(env, d.assigns),
                          elisp_intern(env, ":consts"),
                          signal_list_to_elisp(env, d.consts),
                          elisp_intern(env, ":gparams"),
                          signal_list_to_elisp(env, d.gparams),
                          elisp_intern(env, ":interfaces"),
                          signal_list_to_elisp(env, d.interfaces)});
}

emacs_value inst_to_plist(emacs_env *env, const BufferState &bs,
                          const InstInfo &inf) {
  std::vector<emacs_value> params;
  for (const auto &kv : inf.params) {
    params.push_back(
        elisp_list(env, {elisp_string(env, kv.first), elisp_string(env, kv.second)}));
  }
  std::vector<emacs_value> ports;
  for (const auto &kv : inf.ports) {
    ports.push_back(
        elisp_list(env, {elisp_string(env, kv.first), elisp_string(env, kv.second)}));
  }
  return elisp_list(env, {elisp_intern(env, ":module"),
                          elisp_string(env, inf.module_name),
                          elisp_intern(env, ":module-start"),
                          elisp_int(env, bs.byte_to_pos(inf.module_start_byte)),
                          elisp_intern(env, ":module-end"),
                          elisp_int(env, bs.byte_to_pos(inf.module_end_byte)),
                          elisp_intern(env, ":instance"),
                          elisp_string(env, inf.inst_name),
                          elisp_intern(env, ":inst-start"),
                          elisp_int(env, bs.byte_to_pos(inf.inst_start_byte)),
                          elisp_intern(env, ":inst-end"),
                          elisp_int(env, bs.byte_to_pos(inf.inst_end_byte)),
                          elisp_intern(env, ":start"),
                          elisp_int(env, bs.byte_to_pos(inf.start_byte)),
                          elisp_intern(env, ":end"),
                          elisp_int(env, bs.byte_to_pos(inf.end_byte)),
                          elisp_intern(env, ":params"),
                          elisp_list(env, params),
                          elisp_intern(env, ":ports"),
                          elisp_list(env, ports)});
}

emacs_value Fverilog_ts_read_inst_info(emacs_env *env, ptrdiff_t nargs,
                                       emacs_value args[],
                                       void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env);
  intmax_t pos = (nargs >= 1 && env->is_not_nil(env, args[0]))
                     ? env->extract_integer(env, args[0])
                     : get_point(env);
  InstInfo inf = extract_inst_at_byte(bs, bs.pos_to_byte(pos));
  if (!inf.valid) return elisp_nil(env);
  return inst_to_plist(env, bs, inf);
}

emacs_value Fverilog_ts_ast_instantiations(emacs_env *env, ptrdiff_t nargs,
                                           emacs_value args[],
                                           void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env);
  uint32_t start_byte = 0;
  uint32_t end_byte = static_cast<uint32_t>(bs.text.size());
  if (nargs >= 1 && env->is_not_nil(env, args[0])) {
    start_byte = bs.pos_to_byte(env->extract_integer(env, args[0]));
  }
  if (nargs >= 2 && env->is_not_nil(env, args[1])) {
    end_byte = bs.pos_to_byte(env->extract_integer(env, args[1]));
  }
  auto insts = extract_all_insts(bs, start_byte, end_byte);
  std::vector<emacs_value> out;
  out.reserve(insts.size());
  for (const auto &inf : insts) {
    out.push_back(inst_to_plist(env, bs, inf));
  }
  return elisp_list(env, out);
}

emacs_value Fverilog_ts_read_always_signals(emacs_env *env, ptrdiff_t nargs,
                                            emacs_value args[],
                                            void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env);
  intmax_t pos = (nargs >= 1 && env->is_not_nil(env, args[0]))
                     ? env->extract_integer(env, args[0])
                     : get_point(env);
  uint32_t sb = bs.pos_to_byte(pos);
  AlwaysSignals sigs =
      extract_always_signals(bs, sb, static_cast<uint32_t>(bs.text.size()));

  auto make_sig_list = [&](const std::vector<std::string> &names) {
    std::vector<emacs_value> l;
    l.reserve(names.size());
    for (const auto &nm : names) {
      l.push_back(
          elisp_list(env, {elisp_string(env, nm), elisp_nil(env), elisp_nil(env)}));
    }
    return elisp_list(env, l);
  };

  return elisp_vector(env, {make_sig_list(sigs.out_delayed),
                            make_sig_list(sigs.out_immediate),
                            make_sig_list(sigs.temps),
                            make_sig_list(sigs.inputs)});
}

emacs_value Fverilog_ts_ast_always_blocks(emacs_env *env, ptrdiff_t nargs,
                                          emacs_value args[],
                                          void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env);
  if (!bs.tree) return elisp_nil(env);

  uint32_t err_off = 0;
  TSQueryError err_type = TSQueryErrorNone;
  TSQuery *q = get_or_compile_query("(always_construct) @alw", &err_off,
                                    &err_type);
  if (!q) return elisp_nil(env);

  TSQueryCursor *cursor = ts_query_cursor_new();
  ts_query_cursor_exec(cursor, q, ts_tree_root_node(bs.tree));
  std::vector<emacs_value> out;
  TSQueryMatch match;
  while (ts_query_cursor_next_match(cursor, &match)) {
    for (uint16_t i = 0; i < match.capture_count; ++i) {
      TSNode n = match.captures[i].node;
      uint32_t sb = ts_node_start_byte(n);
      uint32_t eb = ts_node_end_byte(n);
      std::string kwd = "always";
      if (ts_node_named_child_count(n) > 0) {
        TSNode k = ts_node_named_child(n, 0);
        if (std::string_view(ts_node_type(k)) == "always_keyword") {
          kwd = std::string(bs.node_text(k));
        }
      }
      AlwaysSignals sigs = extract_always_signals(bs, sb, eb);
      auto to_str_list = [&](const std::vector<std::string> &v) {
        std::vector<emacs_value> l;
        for (const auto &s : v) l.push_back(elisp_string(env, s));
        return elisp_list(env, l);
      };
      out.push_back(elisp_list(
          env, {elisp_intern(env, ":keyword"),
                elisp_string(env, kwd),
                elisp_intern(env, ":start"),
                elisp_int(env, bs.byte_to_pos(sb)),
                elisp_intern(env, ":end"),
                elisp_int(env, bs.byte_to_pos(eb)),
                elisp_intern(env, ":outputs-delayed"),
                to_str_list(sigs.out_delayed),
                elisp_intern(env, ":outputs-immediate"),
                to_str_list(sigs.out_immediate),
                elisp_intern(env, ":temps"),
                to_str_list(sigs.temps),
                elisp_intern(env, ":inputs"),
                to_str_list(sigs.inputs)}));
    }
  }
  ts_query_cursor_delete(cursor);
  return elisp_list(env, out);
}

emacs_value Fverilog_ts_read_defines(emacs_env *env, ptrdiff_t nargs,
                                     emacs_value args[],
                                     void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env);
  std::vector<emacs_value> defines;
  std::vector<emacs_value> params;
  std::vector<emacs_value> includes;

  for (uint32_t ls : bs.line_starts) {
    if (bs.is_inside_comment_or_string_byte(ls, true) ||
        bs.is_inside_protected_byte(ls)) {
      continue;
    }
    uint32_t le = bs.line_end_byte(ls);
    uint32_t p = ls;
    while (p < le && (bs.text[p] == ' ' || bs.text[p] == '\t')) ++p;
    std::string_view line = bs.slice(p, le);

    if (line.rfind("`include", 0) == 0 && line.size() > 8 &&
        (line[8] == ' ' || line[8] == '\t')) {
      std::string inc = trim_ws(line.substr(8));
      size_t sp = inc.find_first_of(" \t");
      if (sp != std::string::npos) inc.resize(sp);
      if (!inc.empty()) {
        includes.push_back(elisp_string(env, inc));
      }
    } else if (line.rfind("`define", 0) == 0 && line.size() > 7 &&
               (line[7] == ' ' || line[7] == '\t')) {
      size_t k = 7;
      while (k < line.size() && (line[k] == ' ' || line[k] == '\t')) ++k;
      size_t ns = k;
      while (k < line.size() &&
             (std::isalnum(static_cast<unsigned char>(line[k])) ||
              line[k] == '_' || line[k] == '$')) {
        ++k;
      }
      if (k > ns) {
        std::string dname(line.substr(ns, k - ns));
        if (k < line.size() && line[k] == '(') {
          int depth = 0;
          while (k < line.size()) {
            if (line[k] == '(') ++depth;
            else if (line[k] == ')') {
              --depth;
              if (depth == 0) {
                ++k;
                break;
              }
            }
            ++k;
          }
        }
        while (k < line.size() && (line[k] == ' ' || line[k] == '\t')) ++k;
        std::string dval(k < line.size() ? line.substr(k) : "");
        // Strip trailing // or /* comment
        for (size_t c = 0; c + 1 < dval.size(); ++c) {
          if (dval[c] == '/' && (dval[c + 1] == '/' || dval[c + 1] == '*')) {
            while (c > 0 && (dval[c - 1] == ' ' || dval[c - 1] == '\t')) --c;
            dval.resize(c);
            break;
          }
        }
        defines.push_back(
            elisp_list(env, {elisp_string(env, dname), elisp_string(env, dval)}));
      }
    }
  }

  return elisp_list(env, {elisp_intern(env, ":defines"),
                          elisp_list(env, defines),
                          elisp_intern(env, ":params"),
                          elisp_list(env, params),
                          elisp_intern(env, ":includes"),
                          elisp_list(env, includes)});
}

emacs_value Fverilog_ts_read_signals(emacs_env *env, ptrdiff_t nargs,
                                     emacs_value args[],
                                     void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env);
  uint32_t start_byte =
      (nargs >= 1 && env->is_not_nil(env, args[0]))
          ? bs.pos_to_byte(env->extract_integer(env, args[0]))
          : 0;
  uint32_t end_byte =
      (nargs >= 2 && env->is_not_nil(env, args[1]))
          ? bs.pos_to_byte(env->extract_integer(env, args[1]))
          : static_cast<uint32_t>(bs.text.size());

  uint32_t n = std::min<uint32_t>(end_byte,
                                  static_cast<uint32_t>(bs.text.size()));
  std::unordered_set<std::string> seen;
  std::vector<emacs_value> sigs;

  uint32_t i = start_byte;
  while (i < n) {
    if (const CommentStringSpan *sp = bs.find_span_at_byte(i, false)) {
      i = std::min<uint32_t>(sp->end_byte, n);
      continue;
    }
    if (bs.is_inside_attribute_byte(i) || bs.is_inside_protected_byte(i)) {
      ++i;
      continue;
    }
    char c = bs.text[i];
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == '.' ||
        c == '%' || c == '`') {
      uint32_t s = i;
      while (i < n &&
             (std::isalnum(static_cast<unsigned char>(bs.text[i])) ||
              bs.text[i] == '$' || bs.text[i] == '_' || bs.text[i] == '.' ||
              bs.text[i] == '%' || bs.text[i] == '`')) {
        ++i;
      }
      std::string tok(bs.slice(s, i));
      if (!is_verilog_keyword(tok) && seen.insert(tok).second) {
        sigs.push_back(elisp_string(env, tok));
      }
    } else {
      ++i;
    }
  }
  std::reverse(sigs.begin(), sigs.end());
  return elisp_list(env, sigs);
}

emacs_value Fverilog_ts_imenu_create_index(emacs_env *env, ptrdiff_t nargs,
                                           emacs_value args[],
                                           void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env);
  auto defuns = find_all_defuns(bs, false);
  auto insts = extract_all_insts(bs, 0, static_cast<uint32_t>(bs.text.size()));

  std::vector<emacs_value> mods, intfs, pkgs, classes, tfs, inst_items;
  for (const auto &d : defuns) {
    emacs_value pair = elisp_cons(env, elisp_string(env, d.name),
                                  elisp_int(env, bs.byte_to_pos(d.kwd_byte)));
    if (d.type == "module" || d.type == "macromodule" ||
        d.type == "connectmodule" || d.type == "primitive") {
      mods.push_back(pair);
    } else if (d.type == "interface" || d.type == "program" ||
               d.type == "checker" || d.type == "config") {
      intfs.push_back(pair);
    } else if (d.type == "package") {
      pkgs.push_back(pair);
    } else if (d.type == "class") {
      classes.push_back(pair);
    } else if (d.type == "function" || d.type == "task") {
      tfs.push_back(pair);
    }
  }
  for (const auto &inf : insts) {
    std::string label = inf.inst_name + " (" + inf.module_name + ")";
    inst_items.push_back(
        elisp_cons(env, elisp_string(env, label),
                   elisp_int(env, bs.byte_to_pos(inf.inst_start_byte))));
  }

  std::vector<emacs_value> index;
  if (!mods.empty()) {
    index.push_back(
        elisp_cons(env, elisp_string(env, "Modules"), elisp_list(env, mods)));
  }
  if (!intfs.empty()) {
    index.push_back(elisp_cons(env, elisp_string(env, "Interfaces"),
                               elisp_list(env, intfs)));
  }
  if (!pkgs.empty()) {
    index.push_back(
        elisp_cons(env, elisp_string(env, "Packages"), elisp_list(env, pkgs)));
  }
  if (!classes.empty()) {
    index.push_back(elisp_cons(env, elisp_string(env, "Classes"),
                               elisp_list(env, classes)));
  }
  if (!tfs.empty()) {
    index.push_back(elisp_cons(env, elisp_string(env, "Tasks & Functions"),
                               elisp_list(env, tfs)));
  }
  if (!inst_items.empty()) {
    index.push_back(elisp_cons(env, elisp_string(env, "Instances"),
                               elisp_list(env, inst_items)));
  }
  return elisp_list(env, index);
}

} // namespace

void register_decls_functions(emacs_env *env) {
  bind_function(
      env, "verilog-ts-read-decls", 0, 4, Fverilog_ts_read_decls,
      "Extract module declarations vector [outputs inouts inputs vars modports assigns consts gparams interfaces].");
  bind_function(
      env, "verilog-ts-ast-decls", 0, 4, Fverilog_ts_ast_decls,
      "Extract module declarations as a property list (:outputs ... :inputs ... :vars ...).");
  bind_function(
      env, "verilog-ts-read-inst-info", 0, 1, Fverilog_ts_read_inst_info,
      "Return plist of module instantiation info at POS (default point).");
  bind_function(
      env, "verilog-ts-ast-instantiations", 0, 2,
      Fverilog_ts_ast_instantiations,
      "Return list of all module/interface/program instantiations in region BEG..END.");
  bind_function(
      env, "verilog-ts-read-always-signals", 0, 1,
      Fverilog_ts_read_always_signals,
      "Extract always block signals vector [outputs-delayed outputs-immediate temps inputs] at POS.");
  bind_function(
      env, "verilog-ts-ast-always-blocks", 0, 0,
      Fverilog_ts_ast_always_blocks,
      "Return list of all always blocks and their extracted signals in current buffer.");
  bind_function(
      env, "verilog-ts-read-defines", 0, 0, Fverilog_ts_read_defines,
      "Extract `define and `include directives in current buffer.");
  bind_function(
      env, "verilog-ts-read-signals", 0, 2, Fverilog_ts_read_signals,
      "Extract all non-keyword signal identifiers in region BEG..END.");
  bind_function(
      env, "verilog-ts-imenu-create-index", 0, 0,
      Fverilog_ts_imenu_create_index,
      "Create Imenu index alist for current SystemVerilog buffer using tree-sitter.");
}

} // namespace verilog_ts

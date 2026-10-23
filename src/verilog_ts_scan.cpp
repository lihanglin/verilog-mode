#include "verilog_ts_common.hpp"

namespace verilog_ts {

namespace {

bool is_ident_char(char c) {
  unsigned char uc = static_cast<unsigned char>(c);
  return std::isalnum(uc) || c == '_' || c == '$';
}

bool match_word_at(std::string_view text, uint32_t pos, std::string_view word) {
  if (pos + word.size() > text.size()) return false;
  if (text.substr(pos, word.size()) != word) return false;
  if (pos > 0 && is_ident_char(text[pos - 1])) return false;
  if (pos + word.size() < text.size() && is_ident_char(text[pos + word.size()]))
    return false;
  return true;
}

bool is_directive_line_start(const BufferState &bs, uint32_t ls_byte,
                             uint32_t *dir_end_out = nullptr) {
  uint32_t n = static_cast<uint32_t>(bs.text.size());
  uint32_t i = ls_byte;
  while (i < n && (bs.text[i] == ' ' || bs.text[i] == '\t')) ++i;
  if (i >= n || bs.text[i] != '`') return false;
  uint32_t tick_pos = i;
  ++i;
  while (i < n && (bs.text[i] == ' ' || bs.text[i] == '\t')) ++i;
  uint32_t wstart = i;
  while (i < n && is_ident_char(bs.text[i])) ++i;
  std::string_view dir = bs.slice(wstart, i);
  if (dir.empty()) return false;

  static const std::unordered_set<std::string_view> kDirectives = {
      "define",           "else",          "elsif",
      "endif",            "ifdef",         "ifndef",
      "include",          "line",          "timescale",
      "undef",            "default_nettype", "pragma",
      "resetall",         "celldefine",    "endcelldefine",
      "begin_keywords",   "end_keywords",  "unconnected_drive",
      "nounconnected_drive", "protected",  "endprotected"};
  if (kDirectives.count(dir) == 0) return false;
  (void)tick_pos;

  // Find end of directive line (handling backslash newline continuations)
  uint32_t cur = i;
  while (cur < n) {
    uint32_t lend = bs.line_end_byte(cur);
    bool continued = (lend > cur && bs.text[lend - 1] == '\\');
    if (lend < n && bs.text[lend] == '\n') {
      cur = lend + 1;
    } else {
      cur = lend;
    }
    if (!continued) break;
  }
  if (dir_end_out) *dir_end_out = cur;
  return true;
}

void scan_slice_for_spans(std::string_view text, uint32_t base_offset,
                          std::vector<CommentStringSpan> &spans,
                          std::vector<CommentStringSpan> &attr_spans,
                          std::vector<CommentStringSpan> &protected_spans) {
  uint32_t n = static_cast<uint32_t>(text.size());
  uint32_t i = 0;
  while (i < n) {
    char c = text[i];
    // Check for `protected or `pragma protect begin_protected at line start
    if (c == '`') {
      if (match_word_at(text, i + 1, "protected")) {
        uint32_t pstart = base_offset + i;
        uint32_t j = i + 10;
        while (j < n) {
          if (text[j] == '`' && match_word_at(text, j + 1, "endprotected")) {
            j += 13;
            while (j < n && text[j] != '\n') ++j;
            if (j < n && text[j] == '\n') ++j;
            break;
          }
          ++j;
        }
        protected_spans.push_back(
            {pstart, base_offset + j, SpanKind::PROTECTED_REGION});
      }
      if (match_word_at(text, i + 1, "pragma")) {
        uint32_t j = i + 7;
        while (j < n && (text[j] == ' ' || text[j] == '\t')) ++j;
        if (match_word_at(text, j, "protect")) {
          j += 7;
          while (j < n && (text[j] == ' ' || text[j] == '\t')) ++j;
          if (match_word_at(text, j, "begin_protected")) {
            uint32_t pstart = base_offset + i;
            j += 15;
            while (j < n) {
              if (text[j] == '`' && match_word_at(text, j + 1, "pragma")) {
                uint32_t k = j + 7;
                while (k < n && (text[k] == ' ' || text[k] == '\t')) ++k;
                if (match_word_at(text, k, "protect")) {
                  k += 7;
                  while (k < n && (text[k] == ' ' || text[k] == '\t')) ++k;
                  if (match_word_at(text, k, "end_protected")) {
                    k += 13;
                    while (k < n && text[k] != '\n') ++k;
                    if (k < n && text[k] == '\n') ++k;
                    j = k;
                    break;
                  }
                }
              }
              ++j;
            }
            protected_spans.push_back(
                {pstart, base_offset + j, SpanKind::PROTECTED_REGION});
          }
        }
      }
    }

    if (c == '/' && i + 1 < n && text[i + 1] == '/') {
      uint32_t start = base_offset + i;
      i += 2;
      while (i < n && text[i] != '\n') ++i;
      if (i < n && text[i] == '\n') ++i;
      spans.push_back({start, base_offset + i, SpanKind::LINE_COMMENT});
    } else if (c == '/' && i + 1 < n && text[i + 1] == '*') {
      uint32_t start = base_offset + i;
      i += 2;
      while (i + 1 < n && !(text[i] == '*' && text[i + 1] == '/')) ++i;
      if (i + 1 < n) {
        i += 2;
      } else {
        i = n;
      }
      spans.push_back({start, base_offset + i, SpanKind::BLOCK_COMMENT});
    } else if (c == '"') {
      uint32_t start = base_offset + i;
      ++i;
      while (i < n) {
        if (text[i] == '"' && text[i - 1] != '\\') {
          ++i;
          break;
        }
        ++i;
      }
      spans.push_back({start, base_offset + i, SpanKind::STRING_LITERAL});
    } else if (c == '(' && i + 1 < n && text[i + 1] == '*') {
      // Check if it is (*) or (*   *) event sensitivity vs (* attr *)
      uint32_t j = i + 2;
      while (j < n && (text[j] == ' ' || text[j] == '\t')) ++j;
      if (j < n && text[j] == ')') {
        // It's (* ) or (*) sensitivity list, not an attribute
        i = j + 1;
        continue;
      }
      uint32_t start = base_offset + i;
      j = i + 2;
      bool closed = false;
      while (j + 1 < n) {
        if (text[j] == '*' && text[j + 1] == ')') {
          j += 2;
          closed = true;
          break;
        }
        ++j;
      }
      if (closed) {
        attr_spans.push_back({start, base_offset + j, SpanKind::ATTRIBUTE});
      }
      ++i;
    } else {
      ++i;
    }
  }
}

} // namespace

void build_buffer_spans(BufferState &bs) {
  bs.spans.clear();
  bs.attr_spans.clear();
  bs.protected_spans.clear();

  // First, collect AST-identified attribute_instance nodes from TSTree if available,
  // and run lexical span collection across bs.text so `//` comments include trailing
  // `\n` and comments/strings inside `define macro_text or unclosed EOF comments
  // match `verilog-scan-region` 100%.
  scan_slice_for_spans(bs.text, 0, bs.spans, bs.attr_spans,
                       bs.protected_spans);

  if (bs.tree) {
    // Also cross-check with TSTree attribute_instance nodes so any AST-parsed
    // attributes are guaranteed to be in bs.attr_spans
    uint32_t err_off = 0;
    TSQueryError err_type = TSQueryErrorNone;
    TSQuery *q = get_or_compile_query("(attribute_instance) @attr", &err_off,
                                      &err_type);
    if (q) {
      TSQueryCursor *cursor = ts_query_cursor_new();
      ts_query_cursor_exec(cursor, q, ts_tree_root_node(bs.tree));
      TSQueryMatch match;
      while (ts_query_cursor_next_match(cursor, &match)) {
        for (uint16_t i = 0; i < match.capture_count; ++i) {
          TSNode n = match.captures[i].node;
          uint32_t sb = ts_node_start_byte(n);
          uint32_t eb = ts_node_end_byte(n);
          if (sb < eb && !bs.is_inside_attribute_byte(sb + 1)) {
            bs.attr_spans.push_back({sb, eb, SpanKind::ATTRIBUTE});
          }
        }
      }
      ts_query_cursor_delete(cursor);
      std::sort(bs.attr_spans.begin(), bs.attr_spans.end(),
                [](const CommentStringSpan &a, const CommentStringSpan &b) {
                  return a.start_byte < b.start_byte;
                });
    }
  }
}

uint32_t forward_syntactic_ws_bytes(const BufferState &bs, uint32_t byte,
                                    uint32_t bound_byte, bool skip_directives) {
  uint32_t n = std::min<uint32_t>(bound_byte,
                                  static_cast<uint32_t>(bs.text.size()));
  if (byte >= n) return byte;

  uint32_t cur = byte;

  // If starting inside a comment or string, jump to its end
  if (const CommentStringSpan *sp = bs.find_span_at_byte(cur, true)) {
    if (sp->kind == SpanKind::LINE_COMMENT ||
        sp->kind == SpanKind::BLOCK_COMMENT) {
      cur = std::min<uint32_t>(sp->end_byte, n);
    }
  }
  // If starting inside an attribute, jump to its end
  for (const auto &asp : bs.attr_spans) {
    if (cur > asp.start_byte && cur < asp.end_byte) {
      cur = std::min<uint32_t>(asp.end_byte, n);
      break;
    }
  }

  while (cur < n) {
    uint32_t prev = cur;
    // Skip whitespace
    while (cur < n && (bs.text[cur] == ' ' || bs.text[cur] == '\t' ||
                       bs.text[cur] == '\n' || bs.text[cur] == '\r' ||
                       bs.text[cur] == '\f')) {
      ++cur;
    }
    if (cur >= n) break;

    // Check if at comment start
    const CommentStringSpan *sp = bs.find_span_at_byte(cur, false);
    if (sp && (sp->kind == SpanKind::LINE_COMMENT ||
               sp->kind == SpanKind::BLOCK_COMMENT)) {
      if (sp->end_byte <= n || sp->kind == SpanKind::LINE_COMMENT) {
        cur = std::min<uint32_t>(sp->end_byte, n);
        continue;
      }
    }

    // Check if at (* ... *) attribute start
    bool jumped_attr = false;
    for (const auto &asp : bs.attr_spans) {
      if (cur == asp.start_byte && asp.end_byte <= n) {
        cur = asp.end_byte;
        jumped_attr = true;
        break;
      }
    }
    if (jumped_attr) continue;

    // Check if on a compiler directive line when skip_directives is true
    if (skip_directives) {
      uint32_t ls = bs.line_start_byte(cur);
      uint32_t dir_end = 0;
      if (is_directive_line_start(bs, ls, &dir_end) && dir_end > cur) {
        cur = std::min<uint32_t>(dir_end, n);
        continue;
      }
    }

    if (cur == prev) break;
  }
  return cur;
}

uint32_t backward_syntactic_ws_bytes(const BufferState &bs, uint32_t byte,
                                     uint32_t bound_byte,
                                     bool skip_directives) {
  uint32_t limit = std::min<uint32_t>(bound_byte,
                                      static_cast<uint32_t>(bs.text.size()));
  uint32_t cur = std::min<uint32_t>(byte, static_cast<uint32_t>(bs.text.size()));
  if (cur <= limit) return cur;

  // If starting inside a comment, move to start of comment
  if (const CommentStringSpan *sp = bs.find_span_at_byte(cur, true)) {
    if (sp->kind == SpanKind::LINE_COMMENT ||
        sp->kind == SpanKind::BLOCK_COMMENT) {
      cur = std::max<uint32_t>(sp->start_byte, limit);
      if (sp->kind == SpanKind::LINE_COMMENT) {
        while (cur > limit && bs.text[cur - 1] == '/') --cur;
      }
    }
  }

  while (cur > limit) {
    uint32_t prev = cur;

    // Skip whitespace backward
    while (cur > limit &&
           (bs.text[cur - 1] == ' ' || bs.text[cur - 1] == '\t' ||
            bs.text[cur - 1] == '\n' || bs.text[cur - 1] == '\r' ||
            bs.text[cur - 1] == '\f')) {
      // Wait: if bs.text[cur - 1] == '\n', check if it's the end of a // line comment!
      if (bs.text[cur - 1] == '\n') {
        const CommentStringSpan *sp = bs.find_span_at_byte(cur - 1, true);
        if (sp && sp->kind == SpanKind::LINE_COMMENT) {
          cur = std::max<uint32_t>(sp->start_byte, limit);
          while (cur > limit && bs.text[cur - 1] == '/') --cur;
          continue;
        }
      }
      --cur;
    }
    if (cur <= limit) break;

    // Check if cur - 1 is inside a comment (e.g. end of /* ... */ or //)
    const CommentStringSpan *sp = bs.find_span_at_byte(cur - 1, false);
    if (sp && (sp->kind == SpanKind::LINE_COMMENT ||
               sp->kind == SpanKind::BLOCK_COMMENT)) {
      cur = std::max<uint32_t>(sp->start_byte, limit);
      if (sp->kind == SpanKind::LINE_COMMENT) {
        while (cur > limit && bs.text[cur - 1] == '/') --cur;
      }
      continue;
    }

    // Check if cur is right after an attribute (* ... *)
    bool jumped_attr = false;
    for (const auto &asp : bs.attr_spans) {
      if (cur == asp.end_byte && asp.start_byte >= limit) {
        cur = asp.start_byte;
        jumped_attr = true;
        break;
      }
    }
    if (jumped_attr) continue;

    // Check if on a compiler directive line when skip_directives is true
    if (skip_directives) {
      uint32_t ls = bs.line_start_byte(cur);
      // Handle multiline backslash-continued directives
      while (ls > limit) {
        uint32_t prev_ls = bs.line_start_byte(ls - 1);
        uint32_t prev_le = bs.line_end_byte(ls - 1);
        if (prev_le > prev_ls && bs.text[prev_le - 1] == '\\') {
          ls = prev_ls;
        } else {
          break;
        }
      }
      if (is_directive_line_start(bs, ls, nullptr) && ls < cur) {
        cur = std::max<uint32_t>(ls, limit);
        continue;
      }
    }

    if (cur == prev) break;
  }
  return cur;
}

namespace {

emacs_value Fverilog_ts_scan_region(emacs_env *env, ptrdiff_t nargs,
                                    emacs_value args[],
                                    void *data) EMACS_NOEXCEPT {
  intmax_t beg = (nargs >= 1 && env->is_not_nil(env, args[0]))
                     ? env->extract_integer(env, args[0])
                     : get_point_min(env);
  intmax_t end = (nargs >= 2 && env->is_not_nil(env, args[1]))
                     ? env->extract_integer(env, args[1])
                     : get_point_max(env);
  if (beg >= end) return elisp_nil(env);

  intmax_t pmin = get_point_min(env);
  intmax_t pmax = get_point_max(env);

  emacs_value fn_put = env->intern(env, "put-text-property");
  emacs_value sym_vcmts = env->intern(env, "v-cmts");
  emacs_value sym_t = elisp_t(env);

  if (beg <= pmin && end >= pmax) {
    BufferState &bs = get_buffer_state(env, false);
    for (const auto &sp : bs.spans) {
      intmax_t s_pos = bs.byte_to_pos(sp.start_byte) + 1;
      intmax_t e_pos = bs.byte_to_pos(sp.end_byte);
      if (s_pos < e_pos) {
        emacs_value put_args[4] = {elisp_int(env, s_pos), elisp_int(env, e_pos),
                                   sym_vcmts, sym_t};
        env->funcall(env, fn_put, 4, put_args);
      }
    }
    return sym_t;
  }

  // Partial region scan (e.g. from verilog-insert inside verilog-save-scan-cache)
  emacs_value sub_args[2] = {elisp_int(env, beg), elisp_int(env, end)};
  emacs_value str_val = env->funcall(
      env, env->intern(env, "buffer-substring-no-properties"), 2, sub_args);
  std::string slice_text = extract_string(env, str_val);
  std::vector<CommentStringSpan> local_spans;
  std::vector<CommentStringSpan> local_attrs;
  std::vector<CommentStringSpan> local_prot;
  scan_slice_for_spans(slice_text, 0, local_spans, local_attrs, local_prot);

  for (const auto &sp : local_spans) {
    intmax_t s_pos = beg + static_cast<intmax_t>(sp.start_byte) + 1;
    intmax_t e_pos = beg + static_cast<intmax_t>(sp.end_byte);
    if (s_pos < e_pos && e_pos <= end) {
      emacs_value put_args[4] = {elisp_int(env, s_pos), elisp_int(env, e_pos),
                                 sym_vcmts, sym_t};
      env->funcall(env, fn_put, 4, put_args);
    }
  }
  return sym_t;
}

emacs_value Fverilog_ts_inside_comment_or_string_p(
    emacs_env *env, ptrdiff_t nargs, emacs_value args[],
    void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env, false);
  intmax_t pos = (nargs >= 1 && env->is_not_nil(env, args[0]))
                     ? env->extract_integer(env, args[0])
                     : get_point(env);
  if (pos < bs.point_min || pos >= bs.point_max) {
    return elisp_nil(env);
  }
  uint32_t byte = bs.pos_to_byte(pos);
  const CommentStringSpan *sp = bs.find_span_at_byte(byte, true);
  if (!sp) return elisp_nil(env);
  switch (sp->kind) {
    case SpanKind::LINE_COMMENT:
      return elisp_intern(env, "line-comment");
    case SpanKind::BLOCK_COMMENT:
      return elisp_intern(env, "block-comment");
    case SpanKind::STRING_LITERAL:
      return elisp_intern(env, "string");
    default:
      return elisp_t(env);
  }
}

emacs_value Fverilog_ts_comment_or_string_bounds(
    emacs_env *env, ptrdiff_t nargs, emacs_value args[],
    void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env, false);
  intmax_t pos = (nargs >= 1 && env->is_not_nil(env, args[0]))
                     ? env->extract_integer(env, args[0])
                     : get_point(env);
  if (pos < bs.point_min || pos >= bs.point_max) {
    return elisp_nil(env);
  }
  uint32_t byte = bs.pos_to_byte(pos);
  const CommentStringSpan *sp = bs.find_span_at_byte(byte, false);
  if (!sp) return elisp_nil(env);
  const char *kind_str = "comment";
  if (sp->kind == SpanKind::LINE_COMMENT) kind_str = "line-comment";
  else if (sp->kind == SpanKind::BLOCK_COMMENT) kind_str = "block-comment";
  else if (sp->kind == SpanKind::STRING_LITERAL) kind_str = "string";

  return elisp_list(env, {elisp_intern(env, ":type"),
                          elisp_intern(env, kind_str),
                          elisp_intern(env, ":start"),
                          elisp_int(env, bs.byte_to_pos(sp->start_byte)),
                          elisp_intern(env, ":end"),
                          elisp_int(env, bs.byte_to_pos(sp->end_byte))});
}

emacs_value Fverilog_ts_comment_spans(emacs_env *env, ptrdiff_t nargs,
                                      emacs_value args[],
                                      void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env, false);
  std::vector<emacs_value> out;
  out.reserve(bs.spans.size());
  for (const auto &sp : bs.spans) {
    const char *kind_str = "comment";
    if (sp.kind == SpanKind::LINE_COMMENT) kind_str = "line-comment";
    else if (sp.kind == SpanKind::BLOCK_COMMENT) kind_str = "block-comment";
    else if (sp.kind == SpanKind::STRING_LITERAL) kind_str = "string";
    out.push_back(elisp_list(env, {elisp_intern(env, kind_str),
                                   elisp_int(env, bs.byte_to_pos(sp.start_byte)),
                                   elisp_int(env, bs.byte_to_pos(sp.end_byte))}));
  }
  return elisp_list(env, out);
}

emacs_value Fverilog_ts_forward_syntactic_ws(emacs_env *env, ptrdiff_t nargs,
                                             emacs_value args[],
                                             void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env, false);
  intmax_t pt = get_point(env);
  intmax_t bound = (nargs >= 1 && env->is_not_nil(env, args[0]))
                       ? env->extract_integer(env, args[0])
                       : bs.point_max;
  bool skip_dirs = (nargs >= 2 && env->is_not_nil(env, args[1]));

  uint32_t cur_byte = bs.pos_to_byte(pt);
  uint32_t bound_byte = bs.pos_to_byte(bound);
  uint32_t next_byte =
      forward_syntactic_ws_bytes(bs, cur_byte, bound_byte, skip_dirs);
  return elisp_int(env, bs.byte_to_pos(next_byte));
}

emacs_value Fverilog_ts_backward_syntactic_ws(emacs_env *env, ptrdiff_t nargs,
                                              emacs_value args[],
                                              void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env, false);
  intmax_t pt = get_point(env);
  intmax_t bound = (nargs >= 1 && env->is_not_nil(env, args[0]))
                       ? env->extract_integer(env, args[0])
                       : bs.point_min;
  bool skip_dirs = (nargs >= 2 && env->is_not_nil(env, args[1]));

  uint32_t cur_byte = bs.pos_to_byte(pt);
  uint32_t bound_byte = bs.pos_to_byte(bound);
  uint32_t prev_byte =
      backward_syntactic_ws_bytes(bs, cur_byte, bound_byte, skip_dirs);
  return elisp_int(env, bs.byte_to_pos(prev_byte));
}

} // namespace

void register_scan_functions(emacs_env *env) {
  bind_function(
      env, "verilog-ts-scan-region", 0, 2, Fverilog_ts_scan_region,
      "Scan region between BEG and END and apply `v-cmts' text properties.");
  bind_function(
      env, "verilog-ts-inside-comment-or-string-p", 0, 1,
      Fverilog_ts_inside_comment_or_string_p,
      "Return `line-comment', `block-comment', `string', or nil at POS (default point).");
  bind_function(
      env, "verilog-ts-comment-or-string-bounds", 0, 1,
      Fverilog_ts_comment_or_string_bounds,
      "Return plist (:type TYPE :start START :end END) of comment/string at POS.");
  bind_function(
      env, "verilog-ts-comment-spans", 0, 0, Fverilog_ts_comment_spans,
      "Return list of (KIND START END) for all comments and strings in buffer.");
  bind_function(
      env, "verilog-ts-forward-syntactic-ws", 0, 2,
      Fverilog_ts_forward_syntactic_ws,
      "Return position after skipping forward over whitespace/comments (and directives if SKIP-DIRECTIVES).");
  bind_function(
      env, "verilog-ts-backward-syntactic_ws", 0, 2,
      Fverilog_ts_backward_syntactic_ws,
      "Return position after skipping backward over whitespace/comments.");
  bind_function(
      env, "verilog-ts-backward-syntactic-ws", 0, 2,
      Fverilog_ts_backward_syntactic_ws,
      "Return position after skipping backward over whitespace/comments (and directives if SKIP-DIRECTIVES).");
}

} // namespace verilog_ts

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
  uint32_t j = byte;
  while (j < n && is_word_char(bs.text[j])) ++j;
  if (end_out) *end_out = j;
  return bs.slice(byte, j);
}

// Compute compact expression string after keyword (like `verilog-get-expr`)
std::string extract_header_expr(const BufferState &bs, uint32_t after_kwd) {
  uint32_t n = static_cast<uint32_t>(bs.text.size());
  uint32_t b = forward_syntactic_ws_bytes(bs, after_kwd, n, false);
  while (b < n && (bs.text[b] == ' ' || bs.text[b] == '\t')) ++b;
  if (b >= n) return {};

  uint32_t e = b;
  if (bs.text[e] == '@') {
    ++e;
    e = forward_syntactic_ws_bytes(bs, e, n, false);
    if (e < n && bs.text[e] == '(') {
      if (auto close_b = forward_sexp_byte(bs, e)) {
        e = *close_b;
      }
    }
  } else if (bs.text[e] == '(' || bs.text[e] == '[') {
    if (auto close_b = forward_sexp_byte(bs, e)) {
      e = *close_b;
    }
  } else {
    while (e < n && bs.text[e] != ':' && bs.text[e] != ' ' &&
           bs.text[e] != '\t' && bs.text[e] != '\n' && bs.text[e] != '\r' &&
           bs.text[e] != '\f' && bs.text[e] != ';') {
      ++e;
    }
  }

  std::string raw(bs.slice(b, e));
  // Truncate at newline or comment with "..."
  for (size_t i = 0; i < raw.size(); ++i) {
    if (raw[i] == '\n' ||
        (i + 1 < raw.size() && raw[i] == '/' &&
         (raw[i + 1] == '/' || raw[i + 1] == '*'))) {
      while (i > 0 && (raw[i - 1] == ' ' || raw[i - 1] == '\t')) --i;
      return raw.substr(0, i) + "...";
    }
  }
  return raw;
}

emacs_value Fverilog_ts_calculate_indent(emacs_env *env, ptrdiff_t nargs,
                                         emacs_value args[],
                                         void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env);
  intmax_t pos = (nargs >= 1 && env->is_not_nil(env, args[0]))
                     ? env->extract_integer(env, args[0])
                     : get_point(env);
  uint32_t byte = bs.pos_to_byte(pos);
  uint32_t ls = bs.line_start_byte(byte);
  uint32_t n = static_cast<uint32_t>(bs.text.size());

  // Move to first non-blank on current line
  uint32_t bol_nonblank = ls;
  while (bol_nonblank < n &&
         (bs.text[bol_nonblank] == ' ' || bs.text[bol_nonblank] == '\t')) {
    ++bol_nonblank;
  }

  // 1. Inside block comment?
  if (const CommentStringSpan *sp = bs.find_span_at_byte(bol_nonblank, true)) {
    if (sp->kind == SpanKind::BLOCK_COMMENT) {
      uint32_t c_col = bs.byte_to_column(sp->start_byte) + 1;
      return elisp_list(env,
                        {elisp_intern(env, "comment"), elisp_int(env, c_col)});
    }
  }

  // 2. Preprocessor directive line?
  if (check_in_context(bs, "directive", bol_nonblank)) {
    return elisp_list(env, {elisp_intern(env, "directive"), elisp_int(env, 0)});
  }

  // 3. Inside struct / union?
  intmax_t nested_struct_col = -1;
  if (check_in_context(bs, "struct-nested", bol_nonblank, &nested_struct_col)) {
    return elisp_list(env, {elisp_intern(env, "block"),
                            elisp_int(env, std::max<intmax_t>(0, nested_struct_col))});
  }

  // 4. Walk enclosing TSTree nodes from bol_nonblank
  if (bs.tree) {
    uint32_t qbyte = (bol_nonblank >= n && n > 0) ? (n - 1) : bol_nonblank;
    TSNode root = ts_tree_root_node(bs.tree);
    TSNode cur = ts_node_descendant_for_byte_range(root, qbyte, qbyte);

    // If cur starts at bol_nonblank, step up to its parent to find the governing scope
    while (!ts_node_is_null(cur) &&
           ts_node_start_byte(cur) >= bol_nonblank) {
      cur = ts_node_parent(cur);
    }

    while (!ts_node_is_null(cur)) {
      std::string_view nt = ts_node_type(cur);
      uint32_t sb = ts_node_start_byte(cur);
      uint32_t base_col = bs.byte_to_column(
          forward_syntactic_ws_bytes(bs, bs.line_start_byte(sb), n, false));

      if (nt == "list_of_port_declarations" ||
          nt == "list_of_port_connections" ||
          nt == "parameter_port_list" ||
          nt == "list_of_parameter_assignments" ||
          nt == "tf_port_list") {
        return elisp_list(env,
                          {elisp_intern(env, "cparenexp"), elisp_int(env, 1)});
      }
      if (nt == "case_statement" || nt == "randcase_statement") {
        return elisp_list(env, {elisp_intern(env, "case"),
                                elisp_int(env, base_col)});
      }
      if (nt == "seq_block" || nt == "par_block" || nt == "generate_region" ||
          nt == "covergroup_declaration" || nt == "property_declaration" ||
          nt == "sequence_declaration" || nt == "clocking_declaration" ||
          nt == "specify_block" || nt == "table_definition" ||
          nt == "constraint_block" || nt == "constraint_set" ||
          nt == "function_declaration" || nt == "task_declaration" ||
          nt == "class_declaration") {
        return elisp_list(env, {elisp_intern(env, "block"),
                                elisp_int(env, base_col)});
      }
      if (nt == "conditional_statement" || nt == "loop_statement" ||
          nt == "always_construct" || nt == "initial_construct" ||
          nt == "final_construct") {
        return elisp_list(env, {elisp_intern(env, "cexp"),
                                elisp_int(env, base_col)});
      }
      if (nt == "module_declaration" || nt == "interface_declaration" ||
          nt == "program_declaration" || nt == "package_declaration" ||
          nt == "checker_declaration" || nt == "config_declaration" ||
          nt == "udp_declaration") {
        return elisp_list(env, {elisp_intern(env, "defun"), elisp_int(env, 0)});
      }
      cur = ts_node_parent(cur);
    }
  }

  return elisp_list(env, {elisp_intern(env, "cpp"), elisp_int(env, 0)});
}

emacs_value Fverilog_ts_decl_align_info(emacs_env *env, ptrdiff_t nargs,
                                        emacs_value args[],
                                        void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env);
  intmax_t pos = (nargs >= 1 && env->is_not_nil(env, args[0]))
                     ? env->extract_integer(env, args[0])
                     : get_point(env);
  if (!bs.tree) return elisp_nil(env);

  uint32_t byte = bs.pos_to_byte(pos);
  uint32_t n = static_cast<uint32_t>(bs.text.size());
  byte = forward_syntactic_ws_bytes(bs, byte, n, false);
  if (byte >= n) return elisp_nil(env);

  TSNode root = ts_tree_root_node(bs.tree);
  TSNode cur = ts_node_descendant_for_byte_range(root, byte, byte);
  while (!ts_node_is_null(cur)) {
    std::string_view nt = ts_node_type(cur);
    if (nt == "data_declaration" || nt == "net_declaration" ||
        nt == "port_declaration" || nt == "ansi_port_declaration" ||
        nt == "parameter_declaration" || nt == "local_parameter_declaration" ||
        nt == "tf_port_item") {
      std::string_view text = bs.node_text(cur);
      if (text.find("struct") != std::string_view::npos ||
          text.find("union") != std::string_view::npos ||
          text.find("enum") != std::string_view::npos ||
          text.find("typedef") != std::string_view::npos) {
        return elisp_nil(env);
      }
      uint32_t sb = ts_node_start_byte(cur);
      uint32_t eb = ts_node_end_byte(cur);
      return elisp_list(env, {elisp_intern(env, ":is-decl"),
                              elisp_t(env),
                              elisp_intern(env, ":node-type"),
                              elisp_string(env, nt),
                              elisp_intern(env, ":start"),
                              elisp_int(env, bs.byte_to_pos(sb)),
                              elisp_intern(env, ":end"),
                              elisp_int(env, bs.byte_to_pos(eb))});
    }
    cur = ts_node_parent(cur);
  }
  return elisp_nil(env);
}

emacs_value Fverilog_ts_expr_align_info(emacs_env *env, ptrdiff_t nargs,
                                        emacs_value args[],
                                        void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env);
  intmax_t pos = (nargs >= 1 && env->is_not_nil(env, args[0]))
                     ? env->extract_integer(env, args[0])
                     : get_point(env);
  if (!bs.tree) return elisp_nil(env);

  uint32_t byte = bs.pos_to_byte(pos);
  uint32_t ls = bs.line_start_byte(byte);
  uint32_t le = bs.line_end_byte(byte);
  uint32_t first_nb = forward_syntactic_ws_bytes(bs, ls, le, false);
  if (first_nb >= le) return elisp_nil(env);

  TSNode root = ts_tree_root_node(bs.tree);
  TSNode cur = ts_node_descendant_for_byte_range(root, first_nb, first_nb);
  while (!ts_node_is_null(cur)) {
    std::string_view nt = ts_node_type(cur);
    if (nt == "blocking_assignment" || nt == "nonblocking_assignment" ||
        nt == "continuous_assign" || nt == "net_assignment" ||
        nt == "variable_decl_assignment" || nt == "param_assignment") {
      uint32_t sb = ts_node_start_byte(cur);
      uint32_t eb = ts_node_end_byte(cur);
      std::string_view sv = bs.slice(sb, eb);
      size_t eq_pos = sv.find("<=");
      std::string_view op_str = "<=";
      if (eq_pos == std::string_view::npos) {
        eq_pos = sv.find('=');
        op_str = "=";
      }
      if (eq_pos != std::string_view::npos) {
        uint32_t op_b = sb + static_cast<uint32_t>(eq_pos);
        uint32_t op_e = op_b + static_cast<uint32_t>(op_str.size());
        return elisp_list(env, {elisp_intern(env, ":is-assign"),
                                elisp_t(env),
                                elisp_intern(env, ":node-type"),
                                elisp_string(env, nt),
                                elisp_intern(env, ":start"),
                                elisp_int(env, bs.byte_to_pos(sb)),
                                elisp_intern(env, ":op-start"),
                                elisp_int(env, bs.byte_to_pos(op_b)),
                                elisp_intern(env, ":op-end"),
                                elisp_int(env, bs.byte_to_pos(op_e)),
                                elisp_intern(env, ":op"),
                                elisp_string(env, op_str),
                                elisp_intern(env, ":end"),
                                elisp_int(env, bs.byte_to_pos(eb))});
      }
    }
    cur = ts_node_parent(cur);
  }
  return elisp_nil(env);
}

emacs_value Fverilog_ts_endcomment_info(emacs_env *env, ptrdiff_t nargs,
                                        emacs_value args[],
                                        void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env);
  intmax_t pos = (nargs >= 1 && env->is_not_nil(env, args[0]))
                     ? env->extract_integer(env, args[0])
                     : get_point(env);
  uint32_t byte = bs.pos_to_byte(pos);
  uint32_t n = static_cast<uint32_t>(bs.text.size());
  byte = forward_syntactic_ws_bytes(bs, byte, n, false);
  if (byte >= n) return elisp_nil(env);

  uint32_t tend = byte;
  std::string_view closer = read_token_at(bs, byte, &tend);
  if (closer.empty()) return elisp_nil(env);

  auto head_opt = leap_to_head_byte(bs, byte);
  if (!head_opt) {
    return elisp_list(env, {elisp_intern(env, ":closer"),
                            elisp_string(env, closer),
                            elisp_intern(env, ":matched"),
                            elisp_nil(env)});
  }

  uint32_t opener_b = *head_opt;
  uint32_t opener_end = opener_b;
  std::string_view opener_tok = read_token_at(bs, opener_b, &opener_end);

  std::string label;
  if (closer == "endcase") {
    if (opener_tok == "randcase") {
      label = "randcase";
    } else {
      label = std::string(opener_tok) + " " +
              extract_header_expr(bs, opener_end);
    }
  } else if (closer == "endmodule" || closer == "endinterface" ||
             closer == "endpackage" || closer == "endprogram" ||
             closer == "endclass" || closer == "endfunction" ||
             closer == "endtask" || closer == "endprimitive" ||
             closer == "endconnectmodule" || closer == "endchecker" ||
             closer == "endconfig") {
    auto d = find_enclosing_defun(bs, opener_b, false);
    if (d) label = d->name;
  } else if (closer == "`endif" || closer == "`else") {
    uint32_t m_start = forward_syntactic_ws_bytes(bs, opener_end, n, false);
    uint32_t m_end = m_start;
    std::string_view m_name = read_token_at(bs, m_start, &m_end);
    std::string prefix = (closer == "`else") ? "!" : " ";
    label = prefix + std::string(opener_tok) + " " + std::string(m_name);
  } else if (closer == "end") {
    // Check if begin has : block_name
    uint32_t after_beg = forward_syntactic_ws_bytes(bs, opener_end, n, false);
    if (after_beg < n && bs.text[after_beg] == ':') {
      uint32_t bname_s =
          forward_syntactic_ws_bytes(bs, after_beg + 1, n, false);
      uint32_t bname_e = bname_s;
      std::string_view bname = read_token_at(bs, bname_s, &bname_e);
      label = "block: " + std::string(bname);
    }
  }

  return elisp_list(env, {elisp_intern(env, ":closer"),
                          elisp_string(env, closer),
                          elisp_intern(env, ":matched"),
                          elisp_t(env),
                          elisp_intern(env, ":opener-pos"),
                          elisp_int(env, bs.byte_to_pos(opener_b)),
                          elisp_intern(env, ":opener-line"),
                          elisp_int(env, bs.byte_to_line(opener_b)),
                          elisp_intern(env, ":label"),
                          elisp_string(env, label)});
}

} // namespace

void register_indent_functions(emacs_env *env) {
  bind_function(
      env, "verilog-ts-calculate-indent", 0, 1, Fverilog_ts_calculate_indent,
      "Calculate AST-based (INDENT-TYPE INDENT-LEVEL) for line at POS (default point).");
  bind_function(
      env, "verilog-ts-decl-align-info", 0, 3, Fverilog_ts_decl_align_info,
      "Return AST declaration alignment plist at POS, or nil if not a declaration.");
  bind_function(
      env, "verilog-ts-expr-align-info", 0, 1, Fverilog_ts_expr_align_info,
      "Return AST assignment alignment plist at POS, or nil if not an assignment.");
  bind_function(
      env, "verilog-ts-endcomment-info", 0, 1, Fverilog_ts_endcomment_info,
      "Return AST endcomment resolution plist for closing keyword at POS.");
}

} // namespace verilog_ts

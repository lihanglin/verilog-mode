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
          nt == "list_of_checker_port_connections" ||
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
        nt == "tf_port_item" || nt == "class_property") {
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

std::string trim_ws_indent(std::string_view sv) {
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

std::string normalize_ws_indent(std::string_view sv) {
  std::string trimmed = trim_ws_indent(sv);
  std::string out;
  out.reserve(trimmed.size());
  bool prev_space = false;
  for (char c : trimmed) {
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      if (!prev_space) {
        out.push_back(' ');
        prev_space = true;
      }
    } else {
      out.push_back(c);
      prev_space = false;
    }
  }
  return out;
}

struct AlignRow {
  uint32_t line_sb = 0;
  uint32_t line_eb = 0;
  uint32_t first_nb = 0;
  std::string dir_str;
  std::string type_str;
  std::vector<std::string> packed_dims;
  std::string rest_str;
  std::string assign_op_str;
  std::string assign_rhs_str;
  std::string trailing_comment;
};

void split_code_and_trailing_comment(const BufferState &bs, uint32_t start_b,
                                     uint32_t line_eb, uint32_t &code_end_b,
                                     std::string &trailing_comment) {
  code_end_b = line_eb;
  trailing_comment.clear();
  if (start_b >= line_eb) return;

  auto it = std::lower_bound(
      bs.spans.begin(), bs.spans.end(), start_b,
      [](const CommentStringSpan &s, uint32_t b) { return s.end_byte <= b; });
  uint32_t candidate_cmt_sb = line_eb;
  for (; it != bs.spans.end() && it->start_byte < line_eb; ++it) {
    if (it->start_byte >= start_b &&
        (it->kind == SpanKind::LINE_COMMENT ||
         (it->kind == SpanKind::BLOCK_COMMENT && it->end_byte <= line_eb))) {
      bool only_ws_or_cmt = true;
      uint32_t p = it->start_byte;
      while (p < line_eb) {
        const CommentStringSpan *sp = bs.find_span_at_byte(p, false);
        if (sp &&
            (sp->kind == SpanKind::LINE_COMMENT ||
             (sp->kind == SpanKind::BLOCK_COMMENT && sp->end_byte <= line_eb))) {
          p = std::min(sp->end_byte, line_eb);
        } else if (bs.text[p] == ' ' || bs.text[p] == '\t') {
          ++p;
        } else {
          only_ws_or_cmt = false;
          break;
        }
      }
      if (only_ws_or_cmt) {
        candidate_cmt_sb = it->start_byte;
        break;
      }
    }
  }
  if (candidate_cmt_sb < line_eb) {
    code_end_b = candidate_cmt_sb;
    trailing_comment = trim_ws_indent(bs.slice(candidate_cmt_sb, line_eb));
  }
}

std::optional<uint32_t> find_top_level_assign_eq(const BufferState &bs,
                                                 uint32_t start_b,
                                                 uint32_t end_b,
                                                 bool allow_nonblocking = false,
                                                 uint32_t *op_len_out = nullptr) {
  int depth = 0;
  for (uint32_t b = start_b; b < end_b; ++b) {
    if (bs.is_inside_comment_or_string_byte(b, false)) continue;
    char c = bs.text[b];
    if (c == '(' || c == '[' || c == '{') {
      ++depth;
    } else if (c == ')' || c == ']' || c == '}') {
      if (depth > 0) --depth;
    } else if (depth == 0) {
      if (allow_nonblocking && c == '<' && b + 1 < end_b &&
          bs.text[b + 1] == '=') {
        char prev_c = (b > start_b) ? bs.text[b - 1] : '\0';
        char next_c = (b + 2 < end_b) ? bs.text[b + 2] : '\0';
        if (prev_c != '<' && prev_c != '=' && next_c != '=') {
          if (op_len_out) *op_len_out = 2;
          return b;
        }
      } else if (c == '=') {
        char prev_c = (b > start_b) ? bs.text[b - 1] : '\0';
        char next_c = (b + 1 < end_b) ? bs.text[b + 1] : '\0';
        if (prev_c != '=' && prev_c != '!' && prev_c != '<' && prev_c != '>' &&
            prev_c != '+' && prev_c != '-' && prev_c != '*' && prev_c != '/' &&
            prev_c != '%' && prev_c != '&' && prev_c != '|' && prev_c != '^' &&
            next_c != '=' && next_c != '>') {
          if (op_len_out) *op_len_out = 1;
          return b;
        }
      }
    }
  }
  return std::nullopt;
}

TSNode find_descendant_of_type(TSNode node, std::string_view target_type,
                               uint32_t limit_byte) {
  if (ts_node_is_null(node) || ts_node_start_byte(node) >= limit_byte) {
    return {};
  }
  if (ts_node_type(node) == target_type &&
      ts_node_end_byte(node) <= limit_byte) {
    return node;
  }
  uint32_t count = ts_node_child_count(node);
  for (uint32_t i = 0; i < count; ++i) {
    TSNode found =
        find_descendant_of_type(ts_node_child(node, i), target_type, limit_byte);
    if (!ts_node_is_null(found)) return found;
  }
  return {};
}

void collect_packed_dim_nodes(TSNode node, uint32_t limit_byte,
                              std::vector<TSNode> &out) {
  if (ts_node_is_null(node) || ts_node_start_byte(node) >= limit_byte) {
    return;
  }
  if (std::string_view(ts_node_type(node)) == "packed_dimension" &&
      ts_node_end_byte(node) <= limit_byte) {
    out.push_back(node);
    return;
  }
  uint32_t count = ts_node_child_count(node);
  for (uint32_t i = 0; i < count; ++i) {
    collect_packed_dim_nodes(ts_node_child(node, i), limit_byte, out);
  }
}

TSNode find_decl_assign_list_child(TSNode node) {
  uint32_t count = ts_node_child_count(node);
  for (uint32_t i = 0; i < count; ++i) {
    TSNode ch = ts_node_child(node, i);
    std::string_view ct = ts_node_type(ch);
    if (ct == "list_of_variable_decl_assignments" ||
        ct == "list_of_net_decl_assignments") {
      return ch;
    }
    if (ct == "data_declaration" || ct == "net_declaration") {
      TSNode sub = find_decl_assign_list_child(ch);
      if (!ts_node_is_null(sub)) return sub;
    }
  }
  return {};
}

bool has_compound_type_descendant(TSNode node, uint32_t limit_byte) {
  if (ts_node_is_null(node) || ts_node_start_byte(node) >= limit_byte) {
    return false;
  }
  std::string_view nt = ts_node_type(node);
  if (nt == "type_declaration" || nt == "package_import_declaration" ||
      nt == "struct_union" || nt == "enum_base_type" ||
      nt == "enum_name_declaration" || nt == "parameter_declaration" ||
      nt == "local_parameter_declaration") {
    return true;
  }
  uint32_t count = ts_node_child_count(node);
  for (uint32_t i = 0; i < count; ++i) {
    if (has_compound_type_descendant(ts_node_child(node, i), limit_byte)) {
      return true;
    }
  }
  return false;
}

bool parse_decl_or_member_row(const BufferState &bs, TSNode node,
                              AlignRow &row) {
  uint32_t raw_sb = ts_node_start_byte(node);
  uint32_t raw_eb = ts_node_end_byte(node);
  uint32_t sb = forward_syntactic_ws_bytes(bs, raw_sb, raw_eb, true);
  uint32_t eb = backward_syntactic_ws_bytes(bs, raw_eb, sb, true);
  if (sb >= eb || bs.byte_to_line(sb) != bs.byte_to_line(eb)) return false;

  TSNode assign_list = find_decl_assign_list_child(node);
  if (ts_node_is_null(assign_list)) return false;

  uint32_t id_sb = ts_node_start_byte(assign_list);
  if (id_sb <= sb) return false;
  if (has_compound_type_descendant(node, id_sb)) return false;

  uint32_t line_sb = bs.line_start_byte(sb);
  uint32_t line_eb = bs.line_end_byte(eb);
  uint32_t first_nb = line_sb;
  while (first_nb < line_eb &&
         (bs.text[first_nb] == ' ' || bs.text[first_nb] == '\t')) {
    ++first_nb;
  }
  if (first_nb != sb) return false;

  std::vector<TSNode> pd_nodes;
  collect_packed_dim_nodes(node, id_sb, pd_nodes);

  uint32_t type_end_b =
      pd_nodes.empty() ? id_sb : ts_node_start_byte(pd_nodes.front());
  std::string type_str = normalize_ws_indent(bs.slice(sb, type_end_b));
  if (type_str.empty()) return false;

  uint32_t code_eb = line_eb;
  split_code_and_trailing_comment(bs, id_sb, line_eb, code_eb,
                                  row.trailing_comment);

  row.line_sb = line_sb;
  row.line_eb = line_eb;
  row.first_nb = first_nb;
  row.dir_str.clear();
  row.type_str = std::move(type_str);
  row.packed_dims.clear();
  for (TSNode pd : pd_nodes) {
    row.packed_dims.push_back(trim_ws_indent(bs.node_text(pd)));
  }
  row.rest_str = trim_ws_indent(bs.slice(id_sb, code_eb));
  row.assign_op_str.clear();
  row.assign_rhs_str.clear();
  return !row.rest_str.empty();
}

bool parse_ansi_port_row(const BufferState &bs, TSNode pnode, AlignRow &row) {
  uint32_t raw_sb = ts_node_start_byte(pnode);
  uint32_t raw_eb = ts_node_end_byte(pnode);
  uint32_t sb = forward_syntactic_ws_bytes(bs, raw_sb, raw_eb, true);
  uint32_t eb = backward_syntactic_ws_bytes(bs, raw_eb, sb, true);
  if (sb >= eb || bs.byte_to_line(sb) != bs.byte_to_line(eb)) return false;

  uint32_t line_sb = bs.line_start_byte(sb);
  uint32_t line_eb = bs.line_end_byte(eb);
  uint32_t first_nb = line_sb;
  while (first_nb < line_eb &&
         (bs.text[first_nb] == ' ' || bs.text[first_nb] == '\t')) {
    ++first_nb;
  }
  if (first_nb != sb) return false;

  // Workaround for tree-sitter-systemverilog GLR ambiguity on
  // `input user_type_t [D1] [D2] port_name` inside `tf_port_list`, where
  // `user_type_t` is parsed as `name` of `tf_port_item`, `[D1] [D2]` as
  // `unpacked_dimension`, and `port_name` as a trailing `(ERROR ...)` on the
  // same line.
  uint32_t after_pnode = raw_eb;
  while (after_pnode < line_eb &&
         (bs.text[after_pnode] == ' ' || bs.text[after_pnode] == '\t')) {
    ++after_pnode;
  }
  if (std::string_view(ts_node_type(pnode)) == "tf_port_item" &&
      after_pnode < line_eb &&
      (std::isalpha(static_cast<unsigned char>(bs.text[after_pnode])) ||
       bs.text[after_pnode] == '_' || bs.text[after_pnode] == '\\') &&
      ts_node_is_null(
          find_descendant_of_type(pnode, "data_type_or_implicit", raw_eb))) {
    uint32_t id_sb = after_pnode;
    TSNode dir_node =
        find_descendant_of_type(pnode, "tf_port_direction", id_sb);
    if (ts_node_is_null(dir_node)) {
      dir_node = find_descendant_of_type(pnode, "port_direction", id_sb);
    }
    std::string dir_str;
    uint32_t after_dir_b = sb;
    if (!ts_node_is_null(dir_node) && ts_node_start_byte(dir_node) >= sb) {
      dir_str = normalize_ws_indent(bs.node_text(dir_node));
      after_dir_b = ts_node_end_byte(dir_node);
    }
    std::vector<TSNode> ud_nodes;
    uint32_t pcount = ts_node_child_count(pnode);
    for (uint32_t i = 0; i < pcount; ++i) {
      TSNode ch = ts_node_child(pnode, i);
      std::string_view ct = ts_node_type(ch);
      if (ct == "unpacked_dimension" || ct == "packed_dimension") {
        ud_nodes.push_back(ch);
      }
    }
    uint32_t type_end_b =
        ud_nodes.empty() ? id_sb : ts_node_start_byte(ud_nodes.front());
    std::string type_str =
        normalize_ws_indent(bs.slice(after_dir_b, type_end_b));
    uint32_t code_eb = line_eb;
    split_code_and_trailing_comment(bs, id_sb, line_eb, code_eb,
                                    row.trailing_comment);
    row.line_sb = line_sb;
    row.line_eb = line_eb;
    row.first_nb = first_nb;
    row.dir_str = std::move(dir_str);
    row.type_str = std::move(type_str);
    row.packed_dims.clear();
    for (TSNode ud : ud_nodes) {
      row.packed_dims.push_back(trim_ws_indent(bs.node_text(ud)));
    }
    row.rest_str = trim_ws_indent(bs.slice(id_sb, code_eb));
    row.assign_op_str.clear();
    row.assign_rhs_str.clear();
    return !row.rest_str.empty();
  }

  TSNode name_node = ts_node_child_by_field_name(pnode, "port_name", 9);
  if (ts_node_is_null(name_node)) {
    name_node = ts_node_child_by_field_name(pnode, "name", 4);
  }
  if (ts_node_is_null(name_node)) {
    uint32_t count = ts_node_child_count(pnode);
    for (uint32_t i = count; i > 0; --i) {
      TSNode ch = ts_node_child(pnode, i - 1);
      std::string_view ct = ts_node_type(ch);
      if ((ct == "simple_identifier" || ct == "escaped_identifier" ||
           ct == "port_identifier") &&
          ts_node_start_byte(ch) >= sb) {
        name_node = ch;
        break;
      }
    }
  }
  if (ts_node_is_null(name_node)) return false;

  uint32_t id_sb = ts_node_start_byte(name_node);
  if (id_sb <= sb) return false;

  TSNode dir_node = find_descendant_of_type(pnode, "tf_port_direction", id_sb);
  if (ts_node_is_null(dir_node)) {
    dir_node = find_descendant_of_type(pnode, "port_direction", id_sb);
  }
  std::string dir_str;
  uint32_t after_dir_b = sb;
  if (!ts_node_is_null(dir_node) && ts_node_start_byte(dir_node) >= sb) {
    dir_str = normalize_ws_indent(bs.node_text(dir_node));
    after_dir_b = ts_node_end_byte(dir_node);
  }

  std::vector<TSNode> pd_nodes;
  collect_packed_dim_nodes(pnode, id_sb, pd_nodes);

  uint32_t type_end_b =
      pd_nodes.empty() ? id_sb : ts_node_start_byte(pd_nodes.front());
  std::string type_str = normalize_ws_indent(bs.slice(after_dir_b, type_end_b));

  uint32_t code_eb = line_eb;
  split_code_and_trailing_comment(bs, id_sb, line_eb, code_eb,
                                  row.trailing_comment);

  row.line_sb = line_sb;
  row.line_eb = line_eb;
  row.first_nb = first_nb;
  row.dir_str = std::move(dir_str);
  row.type_str = std::move(type_str);
  row.packed_dims.clear();
  for (TSNode pd : pd_nodes) {
    row.packed_dims.push_back(trim_ws_indent(bs.node_text(pd)));
  }
  row.rest_str = trim_ws_indent(bs.slice(id_sb, code_eb));
  row.assign_op_str.clear();
  row.assign_rhs_str.clear();
  return !row.rest_str.empty();
}

bool parse_inst_port_row(const BufferState &bs, TSNode cnode, AlignRow &row) {
  uint32_t raw_sb = ts_node_start_byte(cnode);
  uint32_t raw_eb = ts_node_end_byte(cnode);
  uint32_t sb = forward_syntactic_ws_bytes(bs, raw_sb, raw_eb, true);
  uint32_t eb = backward_syntactic_ws_bytes(bs, raw_eb, sb, true);
  if (sb >= eb || bs.byte_to_line(sb) != bs.byte_to_line(eb)) return false;

  uint32_t line_sb = bs.line_start_byte(sb);
  uint32_t line_eb = bs.line_end_byte(eb);
  uint32_t first_nb = line_sb;
  while (first_nb < line_eb &&
         (bs.text[first_nb] == ' ' || bs.text[first_nb] == '\t')) {
    ++first_nb;
  }
  if (first_nb != sb || bs.text[sb] != '.') return false;

  TSNode pname = ts_node_child_by_field_name(cnode, "port_name", 9);
  if (ts_node_is_null(pname) || ts_node_start_byte(pname) < sb) {
    pname = {};
    uint32_t ccount = ts_node_child_count(cnode);
    for (uint32_t i = 0; i < ccount; ++i) {
      TSNode ch = ts_node_child(cnode, i);
      std::string_view ct = ts_node_type(ch);
      if ((ct == "simple_identifier" || ct == "escaped_identifier") &&
          ts_node_start_byte(ch) >= sb) {
        pname = ch;
        break;
      }
    }
  }
  if (ts_node_is_null(pname)) return false;

  std::string port_spec = "." + trim_ws_indent(bs.node_text(pname));
  uint32_t after_pname = ts_node_end_byte(pname);

  uint32_t code_eb = line_eb;
  split_code_and_trailing_comment(bs, after_pname, line_eb, code_eb,
                                  row.trailing_comment);

  uint32_t open_p = after_pname;
  while (open_p < code_eb && bs.text[open_p] != '(' && bs.text[open_p] != ',') {
    ++open_p;
  }
  std::string rest_str;
  if (open_p < code_eb && bs.text[open_p] == '(') {
    auto close_opt = forward_sexp_byte(bs, open_p);
    if (!close_opt || *close_opt > code_eb) return false;
    uint32_t close_p = *close_opt;
    std::string inner = trim_ws_indent(bs.slice(open_p + 1, close_p - 1));
    std::string tail = trim_ws_indent(bs.slice(close_p, code_eb));
    rest_str = "(" + inner + ")";
    if (!tail.empty()) {
      if (tail[0] == ',') {
        rest_str += tail;
      } else {
        rest_str += " " + tail;
      }
    }
  } else {
    rest_str = trim_ws_indent(bs.slice(after_pname, code_eb));
  }

  row.line_sb = line_sb;
  row.line_eb = line_eb;
  row.first_nb = first_nb;
  row.dir_str.clear();
  row.type_str = std::move(port_spec);
  row.packed_dims.clear();
  row.rest_str = std::move(rest_str);
  row.assign_op_str.clear();
  row.assign_rhs_str.clear();
  return true;
}

TSNode find_param_assign_node(TSNode node, uint32_t min_byte,
                              uint32_t limit_byte) {
  if (ts_node_is_null(node) || ts_node_start_byte(node) >= limit_byte) {
    return {};
  }
  std::string_view nt = ts_node_type(node);
  if ((nt == "list_of_param_assignments" || nt == "param_assignment" ||
       nt == "list_of_type_assignments" || nt == "type_assignment" ||
       nt == "list_of_variable_decl_assignments" ||
       nt == "variable_decl_assignment") &&
      ts_node_start_byte(node) >= min_byte &&
      ts_node_end_byte(node) <= limit_byte) {
    return node;
  }
  uint32_t count = ts_node_child_count(node);
  for (uint32_t i = 0; i < count; ++i) {
    TSNode found =
        find_param_assign_node(ts_node_child(node, i), min_byte, limit_byte);
    if (!ts_node_is_null(found)) return found;
  }
  return {};
}

bool parse_param_row(const BufferState &bs, TSNode pnode, AlignRow &row) {
  uint32_t raw_sb = ts_node_start_byte(pnode);
  uint32_t raw_eb = ts_node_end_byte(pnode);
  uint32_t sb = forward_syntactic_ws_bytes(bs, raw_sb, raw_eb, true);
  uint32_t eb = backward_syntactic_ws_bytes(bs, raw_eb, sb, true);
  if (sb >= eb || bs.byte_to_line(sb) != bs.byte_to_line(eb)) return false;

  uint32_t line_sb = bs.line_start_byte(sb);
  uint32_t line_eb = bs.line_end_byte(eb);
  uint32_t first_nb = line_sb;
  while (first_nb < line_eb &&
         (bs.text[first_nb] == ' ' || bs.text[first_nb] == '\t')) {
    ++first_nb;
  }
  if (first_nb != sb) return false;

  uint32_t code_eb = line_eb;
  split_code_and_trailing_comment(bs, sb, line_eb, code_eb,
                                  row.trailing_comment);

  uint32_t tail_p = eb;
  bool seen_punct = false;
  while (tail_p < code_eb) {
    char c = bs.text[tail_p];
    if (c == ' ' || c == '\t') {
      ++tail_p;
    } else if (!seen_punct && (c == ',' || c == ';')) {
      seen_punct = true;
      ++tail_p;
    } else {
      return false;
    }
  }

  std::string dir_str;
  uint32_t after_dir_b = sb;
  auto match_kw = [&](std::string_view kw) -> bool {
    if (sb + kw.size() <= code_eb && bs.slice(sb, sb + kw.size()) == kw &&
        (sb + kw.size() == code_eb || !is_word_char(bs.text[sb + kw.size()]))) {
      dir_str = std::string(kw);
      after_dir_b = sb + static_cast<uint32_t>(kw.size());
      return true;
    }
    return false;
  };
  if (!match_kw("parameter")) {
    match_kw("localparam");
  }

  auto eq_opt = find_top_level_assign_eq(bs, after_dir_b, code_eb, false);
  if (eq_opt && find_top_level_assign_eq(bs, *eq_opt + 1, code_eb, false)) {
    return false; // Reject multiple parameter assignments on a single line
  }

  uint32_t lhs_end = eq_opt ? *eq_opt : code_eb;
  TSNode assign_node = find_param_assign_node(pnode, after_dir_b, raw_eb);
  uint32_t id_sb = 0;
  if (!ts_node_is_null(assign_node) &&
      ts_node_start_byte(assign_node) < lhs_end) {
    id_sb = ts_node_start_byte(assign_node);
  } else {
    // Fallback: locate the parameter identifier before lhs_end
    uint32_t p = lhs_end;
    while (p > after_dir_b &&
           (bs.text[p - 1] == ' ' || bs.text[p - 1] == '\t' ||
            bs.text[p - 1] == ',' || bs.text[p - 1] == ';')) {
      --p;
    }
    // Skip any unpacked dimensions `[...]` after the identifier
    while (p > after_dir_b && bs.text[p - 1] == ']') {
      int depth = 1;
      --p;
      while (p > after_dir_b && depth > 0) {
        --p;
        if (bs.text[p] == ']')
          ++depth;
        else if (bs.text[p] == '[')
          --depth;
      }
      while (p > after_dir_b &&
             (bs.text[p - 1] == ' ' || bs.text[p - 1] == '\t')) {
        --p;
      }
    }
    uint32_t id_end = p;
    while (p > after_dir_b && is_word_char(bs.text[p - 1])) {
      --p;
    }
    if (p >= id_end) return false;
    id_sb = p;
  }

  // If `parameter type T = ...` had `id_sb` on `type`, step past `type`
  if (id_sb + 4 <= lhs_end && bs.slice(id_sb, id_sb + 4) == "type" &&
      !is_word_char(bs.text[id_sb + 4])) {
    uint32_t next_id = id_sb + 4;
    while (next_id < lhs_end &&
           (bs.text[next_id] == ' ' || bs.text[next_id] == '\t')) {
      ++next_id;
    }
    if (next_id < lhs_end) id_sb = next_id;
  }

  std::vector<TSNode> all_pd_nodes;
  collect_packed_dim_nodes(pnode, id_sb, all_pd_nodes);
  std::vector<TSNode> pd_nodes;
  for (TSNode pd : all_pd_nodes) {
    if (ts_node_start_byte(pd) >= after_dir_b) {
      pd_nodes.push_back(pd);
    }
  }

  uint32_t type_end_b =
      pd_nodes.empty() ? id_sb : ts_node_start_byte(pd_nodes.front());
  std::string type_str = normalize_ws_indent(bs.slice(after_dir_b, type_end_b));

  row.line_sb = line_sb;
  row.line_eb = line_eb;
  row.first_nb = first_nb;
  row.dir_str = std::move(dir_str);
  row.type_str = std::move(type_str);
  row.packed_dims.clear();
  for (TSNode pd : pd_nodes) {
    row.packed_dims.push_back(trim_ws_indent(bs.node_text(pd)));
  }
  if (eq_opt && *eq_opt > id_sb) {
    row.rest_str = trim_ws_indent(bs.slice(id_sb, *eq_opt));
    row.assign_op_str = "=";
    row.assign_rhs_str = trim_ws_indent(bs.slice(*eq_opt + 1, code_eb));
  } else {
    row.rest_str = trim_ws_indent(bs.slice(id_sb, code_eb));
    row.assign_op_str.clear();
    row.assign_rhs_str.clear();
  }
  return !row.rest_str.empty();
}

bool parse_enum_member_row(const BufferState &bs, TSNode enode, AlignRow &row) {
  uint32_t raw_sb = ts_node_start_byte(enode);
  uint32_t raw_eb = ts_node_end_byte(enode);
  uint32_t sb = forward_syntactic_ws_bytes(bs, raw_sb, raw_eb, true);
  uint32_t eb = backward_syntactic_ws_bytes(bs, raw_eb, sb, true);
  if (sb >= eb || bs.byte_to_line(sb) != bs.byte_to_line(eb)) return false;

  uint32_t line_sb = bs.line_start_byte(sb);
  uint32_t line_eb = bs.line_end_byte(eb);
  uint32_t first_nb = line_sb;
  while (first_nb < line_eb &&
         (bs.text[first_nb] == ' ' || bs.text[first_nb] == '\t')) {
    ++first_nb;
  }
  if (first_nb != sb) return false;

  uint32_t code_eb = line_eb;
  split_code_and_trailing_comment(bs, sb, line_eb, code_eb,
                                  row.trailing_comment);

  uint32_t tail_p = eb;
  bool seen_comma = false;
  while (tail_p < code_eb) {
    char c = bs.text[tail_p];
    if (c == ' ' || c == '\t') {
      ++tail_p;
    } else if (!seen_comma && c == ',') {
      seen_comma = true;
      ++tail_p;
    } else {
      return false;
    }
  }

  auto eq_opt = find_top_level_assign_eq(bs, sb, code_eb, false);
  row.line_sb = line_sb;
  row.line_eb = line_eb;
  row.first_nb = first_nb;
  row.dir_str.clear();
  row.type_str.clear();
  row.packed_dims.clear();
  if (eq_opt && *eq_opt > sb) {
    row.rest_str = trim_ws_indent(bs.slice(sb, *eq_opt));
    row.assign_op_str = "=";
    row.assign_rhs_str = trim_ws_indent(bs.slice(*eq_opt + 1, code_eb));
  } else {
    row.rest_str = trim_ws_indent(bs.slice(sb, code_eb));
    row.assign_op_str.clear();
    row.assign_rhs_str.clear();
  }
  return !row.rest_str.empty();
}

bool parse_assign_row(const BufferState &bs, TSNode anode, bool is_cont_assign,
                      AlignRow &row) {
  uint32_t raw_sb = ts_node_start_byte(anode);
  uint32_t raw_eb = ts_node_end_byte(anode);
  uint32_t sb = forward_syntactic_ws_bytes(bs, raw_sb, raw_eb, true);
  uint32_t eb = backward_syntactic_ws_bytes(bs, raw_eb, sb, true);
  if (sb >= eb || bs.byte_to_line(sb) != bs.byte_to_line(eb)) return false;

  uint32_t line_sb = bs.line_start_byte(sb);
  uint32_t line_eb = bs.line_end_byte(eb);
  uint32_t first_nb = line_sb;
  while (first_nb < line_eb &&
         (bs.text[first_nb] == ' ' || bs.text[first_nb] == '\t')) {
    ++first_nb;
  }
  if (first_nb != sb) return false;

  uint32_t code_eb = line_eb;
  split_code_and_trailing_comment(bs, sb, line_eb, code_eb,
                                  row.trailing_comment);

  uint32_t tail_p = eb;
  bool seen_semi = false;
  while (tail_p < code_eb) {
    char c = bs.text[tail_p];
    if (c == ' ' || c == '\t') {
      ++tail_p;
    } else if (!seen_semi && c == ';') {
      seen_semi = true;
      ++tail_p;
    } else {
      return false;
    }
  }

  uint32_t lhs_sb = sb;
  std::string dir_str;
  if (is_cont_assign) {
    if (sb + 6 > code_eb || bs.slice(sb, sb + 6) != "assign" ||
        is_word_char(bs.text[sb + 6])) {
      return false;
    }
    dir_str = "assign";
    lhs_sb = sb + 6;
  }

  uint32_t op_len = 0;
  auto eq_opt = find_top_level_assign_eq(bs, lhs_sb, code_eb, !is_cont_assign,
                                         &op_len);
  if (!eq_opt || op_len == 0 || *eq_opt <= lhs_sb) return false;
  if (find_top_level_assign_eq(bs, *eq_opt + op_len, code_eb, !is_cont_assign)) {
    return false;
  }

  row.line_sb = line_sb;
  row.line_eb = line_eb;
  row.first_nb = first_nb;
  row.dir_str = std::move(dir_str);
  row.type_str.clear();
  row.packed_dims.clear();
  row.rest_str = trim_ws_indent(bs.slice(lhs_sb, *eq_opt));
  row.assign_op_str = std::string(bs.slice(*eq_opt, *eq_opt + op_len));
  row.assign_rhs_str = trim_ws_indent(bs.slice(*eq_opt + op_len, code_eb));
  return !row.rest_str.empty() && !row.assign_rhs_str.empty();
}

TSNode find_single_line_decl_at_byte(const BufferState &bs, TSNode root,
                                     uint32_t nb) {
  TSNode cur = ts_node_descendant_for_byte_range(root, nb, nb);
  TSNode best = {};
  while (!ts_node_is_null(cur)) {
    std::string_view nt = ts_node_type(cur);
    if (nt == "data_declaration" || nt == "net_declaration" ||
        nt == "class_property") {
      uint32_t sb = forward_syntactic_ws_bytes(
          bs, ts_node_start_byte(cur), ts_node_end_byte(cur), true);
      uint32_t eb = backward_syntactic_ws_bytes(
          bs, ts_node_end_byte(cur), sb, true);
      if (sb == nb && bs.byte_to_line(sb) == bs.byte_to_line(eb)) {
        best = cur;
      }
    }
    cur = ts_node_parent(cur);
  }
  return best;
}

TSNode find_single_line_param_decl_at_byte(const BufferState &bs, TSNode root,
                                           uint32_t nb) {
  TSNode cur = ts_node_descendant_for_byte_range(root, nb, nb);
  TSNode best = {};
  while (!ts_node_is_null(cur)) {
    std::string_view nt = ts_node_type(cur);
    if (nt == "parameter_port_list" || nt == "parameter_port_declaration") {
      return {};
    }
    if (nt == "parameter_declaration" || nt == "local_parameter_declaration") {
      uint32_t sb = forward_syntactic_ws_bytes(
          bs, ts_node_start_byte(cur), ts_node_end_byte(cur), true);
      uint32_t eb = backward_syntactic_ws_bytes(
          bs, ts_node_end_byte(cur), sb, true);
      if (sb == nb && bs.byte_to_line(sb) == bs.byte_to_line(eb)) {
        best = cur;
      }
    }
    cur = ts_node_parent(cur);
  }
  return best;
}

TSNode find_single_line_assign_at_byte(const BufferState &bs, TSNode root,
                                       uint32_t nb, bool align_assign_expr,
                                       bool *is_cont_assign_out = nullptr) {
  TSNode cur = ts_node_descendant_for_byte_range(root, nb, nb);
  TSNode best = {};
  bool best_cont = false;
  while (!ts_node_is_null(cur)) {
    std::string_view nt = ts_node_type(cur);
    if (nt == "continuous_assign") {
      if (!align_assign_expr) return {};
      uint32_t sb = forward_syntactic_ws_bytes(
          bs, ts_node_start_byte(cur), ts_node_end_byte(cur), true);
      uint32_t eb = backward_syntactic_ws_bytes(
          bs, ts_node_end_byte(cur), sb, true);
      if (sb == nb && bs.byte_to_line(sb) == bs.byte_to_line(eb)) {
        best = cur;
        best_cont = true;
      }
    } else if (nt == "blocking_assignment" || nt == "nonblocking_assignment" ||
               nt == "operator_assignment") {
      uint32_t sb = forward_syntactic_ws_bytes(
          bs, ts_node_start_byte(cur), ts_node_end_byte(cur), true);
      uint32_t eb = backward_syntactic_ws_bytes(
          bs, ts_node_end_byte(cur), sb, true);
      if (sb == nb && bs.byte_to_line(sb) == bs.byte_to_line(eb)) {
        best = cur;
        best_cont = false;
      }
    }
    cur = ts_node_parent(cur);
  }
  if (!ts_node_is_null(best) && is_cont_assign_out) {
    *is_cont_assign_out = best_cont;
  }
  return best;
}

TSNode find_enclosing_stmt_scope(TSNode node) {
  for (TSNode p = ts_node_parent(node); !ts_node_is_null(p);
       p = ts_node_parent(p)) {
    std::string_view pt = ts_node_type(p);
    if (pt == "conditional_statement" || pt == "loop_statement" ||
        pt == "case_item" || pt == "always_construct" ||
        pt == "initial_construct" || pt == "final_construct" ||
        pt == "seq_block" || pt == "par_block" ||
        pt == "function_declaration" || pt == "task_declaration" ||
        pt == "class_constructor_declaration" || pt == "module_declaration" ||
        pt == "interface_declaration" || pt == "program_declaration" ||
        pt == "package_declaration" || pt == "checker_declaration" ||
        pt == "class_declaration" || pt == "generate_region" ||
        pt == "generate_block" || pt == "source_file") {
      return p;
    }
  }
  return {};
}

uint32_t compute_scope_base_col(const BufferState &bs, TSNode start_node,
                                const std::vector<AlignRow> &rows,
                                intmax_t indent_level) {
  uint32_t n = static_cast<uint32_t>(bs.text.size());
  uint32_t base_col = 0;
  bool found_scope = false;
  uint32_t first_col = bs.byte_to_column(rows.front().first_nb);
  for (TSNode p = ts_node_parent(start_node); !ts_node_is_null(p);
       p = ts_node_parent(p)) {
    std::string_view pt = ts_node_type(p);
    if (pt == "conditional_statement" || pt == "loop_statement" ||
        pt == "case_item" || pt == "always_construct" ||
        pt == "initial_construct" || pt == "final_construct") {
      // Unbraced single-statement body: keep its own calculated indent
      break;
    }
    if (pt == "module_declaration" || pt == "interface_declaration" ||
        pt == "program_declaration" || pt == "package_declaration" ||
        pt == "checker_declaration" || pt == "class_declaration" ||
        pt == "function_declaration" || pt == "task_declaration" ||
        pt == "class_constructor_declaration" || pt == "seq_block" ||
        pt == "par_block" || pt == "generate_region" ||
        pt == "generate_block") {
      uint32_t p_sb = bs.line_start_byte(ts_node_start_byte(p));
      uint32_t p_nb = p_sb;
      while (p_nb < n && (bs.text[p_nb] == ' ' || bs.text[p_nb] == '\t')) {
        ++p_nb;
      }
      uint32_t header_col = bs.byte_to_column(p_nb);
      base_col = header_col +
                 static_cast<uint32_t>(std::max<intmax_t>(1, indent_level));
      found_scope = true;
      break;
    }
  }
  if (!found_scope) {
    bool found_indent = false;
    for (const auto &r : rows) {
      bool has_tab = false;
      for (uint32_t i = r.line_sb; i < r.first_nb; ++i) {
        if (bs.text[i] == '\t') {
          has_tab = true;
          break;
        }
      }
      if (!has_tab && r.first_nb > r.line_sb) {
        base_col = r.first_nb - r.line_sb;
        found_indent = true;
        break;
      }
    }
    if (!found_indent) {
      base_col = first_col;
    }
  }
  return base_col;
}

emacs_value format_align_rows(
    emacs_env *env, const BufferState &bs, const std::vector<AlignRow> &rows,
    uint32_t base_col, bool is_inst_ports, bool is_struct_or_port,
    bool align_dim, bool align_sub_dim, bool align_comments = true,
    intmax_t comment_distance = 1,
    std::optional<std::tuple<uint32_t, uint32_t, std::string>> closing_edit =
        std::nullopt,
    std::optional<std::tuple<uint32_t, uint32_t, std::string>> opening_edit =
        std::nullopt) {
  if (rows.empty()) return elisp_nil(env);

  std::string indent_prefix(base_col, ' ');
  std::vector<emacs_value> out;
  out.reserve(rows.size() + (opening_edit.has_value() ? 1 : 0) +
              (closing_edit.has_value() ? 1 : 0));

  if (opening_edit.has_value()) {
    const auto &[o_sb, o_eb, o_str] = *opening_edit;
    out.push_back(elisp_list(env, {elisp_int(env, bs.byte_to_pos(o_sb)),
                                   elisp_int(env, bs.byte_to_pos(o_eb)),
                                   elisp_string(env, o_str)}));
  }

  auto finalize_rows = [&](std::vector<std::string> &code_lines) {
    size_t max_commented_code_len = 0;
    for (size_t i = 0; i < rows.size(); ++i) {
      if (!rows[i].trailing_comment.empty()) {
        max_commented_code_len =
            std::max(max_commented_code_len, code_lines[i].size());
      }
    }
    size_t cmt_dist =
        static_cast<size_t>(std::max<intmax_t>(1, comment_distance));
    for (size_t i = 0; i < rows.size(); ++i) {
      std::string line = std::move(code_lines[i]);
      if (!rows[i].trailing_comment.empty()) {
        if (align_comments) {
          line.append(max_commented_code_len - line.size() + cmt_dist, ' ');
        } else {
          line.push_back(' ');
        }
        line += rows[i].trailing_comment;
      }
      out.push_back(elisp_list(
          env, {elisp_int(env, bs.byte_to_pos(rows[i].line_sb)),
                elisp_int(env, bs.byte_to_pos(rows[i].line_eb)),
                elisp_string(env, line)}));
    }
    if (closing_edit.has_value()) {
      const auto &[c_sb, c_eb, c_str] = *closing_edit;
      out.push_back(elisp_list(env, {elisp_int(env, bs.byte_to_pos(c_sb)),
                                     elisp_int(env, bs.byte_to_pos(c_eb)),
                                     elisp_string(env, c_str)}));
    }
    return elisp_list(env, out);
  };

  std::vector<std::string> code_lines;
  code_lines.reserve(rows.size());

  if (is_inst_ports) {
    size_t w_port = 0;
    for (const auto &r : rows) {
      w_port = std::max(w_port, r.type_str.size());
    }
    for (const auto &r : rows) {
      std::string line = indent_prefix + r.type_str;
      if (!r.rest_str.empty()) {
        if (r.rest_str[0] == '(') {
          line.append(w_port - r.type_str.size() + 1, ' ');
          line += r.rest_str;
        } else {
          line += r.rest_str;
        }
      }
      code_lines.push_back(std::move(line));
    }
    return finalize_rows(code_lines);
  }

  size_t w_dir = 0;
  size_t w_type = 0;
  size_t max_k = 0;
  size_t w_rest = 0;
  bool has_2char_assign_op = false;
  for (const auto &r : rows) {
    w_dir = std::max(w_dir, r.dir_str.size());
    w_type = std::max(w_type, r.type_str.size());
    max_k = std::max(max_k, r.packed_dims.size());
    w_rest = std::max(w_rest, r.rest_str.size());
    if (r.assign_op_str.size() == 2) {
      has_2char_assign_op = true;
    }
  }

  auto append_rest_and_assign = [&](std::string &line, const AlignRow &r) {
    line += r.rest_str;
    if (!r.assign_op_str.empty()) {
      line.append(w_rest - r.rest_str.size() + 1, ' ');
      if (has_2char_assign_op && r.assign_op_str == "=") {
        line.push_back(' ');
      }
      line += r.assign_op_str;
      if (!r.assign_rhs_str.empty()) {
        line.push_back(' ');
        line += r.assign_rhs_str;
      }
    }
  };

  bool use_sub_dim = align_dim && align_sub_dim;
  if (use_sub_dim && max_k > 0) {
    // Map each row's packed dimensions from right to left into max_k columns:
    // col_idx = (max_k - m) + j for j in [0, m - 1]
    std::vector<size_t> w_dim(max_k, 0);
    for (const auto &r : rows) {
      size_t m = r.packed_dims.size();
      size_t offset = max_k - m;
      for (size_t j = 0; j < m; ++j) {
        w_dim[offset + j] =
            std::max(w_dim[offset + j], r.packed_dims[j].size());
      }
    }
    for (const auto &r : rows) {
      std::string line = indent_prefix;
      if (w_dir > 0) {
        line += r.dir_str;
        line.append(w_dir - r.dir_str.size() + 1, ' ');
      }
      if (w_type > 0) {
        line += r.type_str;
        line.append(w_type - r.type_str.size() + 1, ' ');
      }
      size_t m = r.packed_dims.size();
      size_t offset = max_k - m;
      for (size_t k = 0; k < max_k; ++k) {
        if (k >= offset) {
          std::string_view d = r.packed_dims[k - offset];
          line.append(d.data(), d.size());
          line.append(w_dim[k] - d.size() + 1, ' ');
        } else {
          line.append(w_dim[k] + 1, ' ');
        }
      }
      append_rest_and_assign(line, r);
      code_lines.push_back(std::move(line));
    }
    return finalize_rows(code_lines);
  }

  std::vector<std::string> combined_dims(rows.size());
  size_t w_dims = 0;
  for (size_t i = 0; i < rows.size(); ++i) {
    std::string comb;
    for (const auto &d : rows[i].packed_dims) {
      comb += d;
    }
    w_dims = std::max(w_dims, comb.size());
    combined_dims[i] = std::move(comb);
  }

  for (size_t i = 0; i < rows.size(); ++i) {
    const auto &r = rows[i];
    std::string line = indent_prefix;
    if (w_dir > 0) {
      line += r.dir_str;
      line.append(w_dir - r.dir_str.size() + 1, ' ');
    }
    if (w_type > 0) {
      line += r.type_str;
      line.append(w_type - r.type_str.size() + 1, ' ');
    }
    if (w_dims > 0) {
      if (align_dim || is_struct_or_port) {
        line += combined_dims[i];
        line.append(w_dims - combined_dims[i].size() + 1, ' ');
      } else if (!combined_dims[i].empty()) {
        line += combined_dims[i] + " ";
      }
    }
    append_rest_and_assign(line, r);
    code_lines.push_back(std::move(line));
  }
  return finalize_rows(code_lines);
}

std::optional<std::tuple<uint32_t, uint32_t, std::string>>
make_closing_line_edit(const BufferState &bs, uint32_t close_b,
                       uint32_t header_col, char expected_char) {
  uint32_t n = static_cast<uint32_t>(bs.text.size());
  if (close_b >= n || bs.text[close_b] != expected_char) return std::nullopt;
  uint32_t c_sb = bs.line_start_byte(close_b);
  uint32_t c_eb = bs.line_end_byte(close_b);
  uint32_t c_nb = c_sb;
  while (c_nb < c_eb && (bs.text[c_nb] == ' ' || bs.text[c_nb] == '\t')) {
    ++c_nb;
  }
  bool has_hash_prefix = false;
  if (c_nb != close_b) {
    if (expected_char == '(' && c_nb < close_b && bs.text[c_nb] == '#') {
      uint32_t p = c_nb + 1;
      while (p < close_b && (bs.text[p] == ' ' || bs.text[p] == '\t')) ++p;
      if (p == close_b) {
        has_hash_prefix = true;
      } else {
        return std::nullopt;
      }
    } else {
      return std::nullopt;
    }
  }
  uint32_t after_b = close_b + 1;
  while (after_b < c_eb &&
         (bs.text[after_b] == ' ' || bs.text[after_b] == '\t')) {
    ++after_b;
  }
  std::string line(header_col, ' ');
  if (has_hash_prefix) line.push_back('#');
  line.push_back(expected_char);
  if (after_b < c_eb) {
    if (after_b > close_b + 1) line.push_back(' ');
    line += trim_ws_indent(bs.slice(after_b, c_eb));
  }
  return std::make_tuple(c_sb, c_eb, std::move(line));
}

uint32_t compute_block_base_col(uint32_t header_col, uint32_t first_col,
                                intmax_t indent_level) {
  uint32_t ind = static_cast<uint32_t>(std::max<intmax_t>(1, indent_level));
  if (indent_level == 3 && first_col == header_col + 4) {
    return first_col;
  }
  return header_col + ind;
}

emacs_value Fverilog_ts_align_block(emacs_env *env, ptrdiff_t nargs,
                                    emacs_value args[],
                                    void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env);
  if (!bs.tree) return elisp_nil(env);

  intmax_t pos = (nargs >= 1 && env->is_not_nil(env, args[0]))
                     ? env->extract_integer(env, args[0])
                     : get_point(env);
  bool align_dim = (nargs >= 2 && env->is_not_nil(env, args[1]));
  bool align_sub_dim =
      align_dim && (nargs >= 3 && env->is_not_nil(env, args[2]));
  intmax_t indent_level = (nargs >= 4 && env->is_not_nil(env, args[3]))
                              ? env->extract_integer(env, args[3])
                              : 4;
  bool align_comments = (nargs < 5) ? true : env->is_not_nil(env, args[4]);
  intmax_t comment_distance = (nargs >= 6 && env->is_not_nil(env, args[5]))
                                  ? env->extract_integer(env, args[5])
                                  : 1;
  bool align_decls =
      (nargs < 7 || !env->is_not_nil(env, args[6]) ||
       env->eq(env, args[6], env->intern(env, "all")) ||
       env->eq(env, args[6], env->intern(env, "declarations")));
  bool align_assignments =
      (nargs >= 7 && env->is_not_nil(env, args[6]) &&
       (env->eq(env, args[6], env->intern(env, "all")) ||
        env->eq(env, args[6], env->intern(env, "assignments"))));
  bool align_assign_expr = (nargs < 8) ? true : env->is_not_nil(env, args[7]);

  uint32_t n = static_cast<uint32_t>(bs.text.size());
  uint32_t byte = bs.pos_to_byte(pos);
  uint32_t ls = bs.line_start_byte(byte);
  uint32_t le = bs.line_end_byte(byte);
  uint32_t raw_nb = ls;
  while (raw_nb < le &&
         (bs.text[raw_nb] == ' ' || bs.text[raw_nb] == '\t')) {
    ++raw_nb;
  }
  if (raw_nb >= le) return elisp_nil(env);
  uint32_t first_nb = forward_syntactic_ws_bytes(bs, ls, n, true);
  if (first_nb >= le) return elisp_nil(env);

  TSNode root = ts_tree_root_node(bs.tree);
  uint32_t probe_b = first_nb;
  uint32_t open_paren_at_line = n;
  if (bs.text[first_nb] == ')') {
    uint32_t prev = backward_syntactic_ws_bytes(bs, first_nb, 0, true);
    if (prev > 0) probe_b = prev - 1;
  } else if (bs.text[first_nb] == '(') {
    open_paren_at_line = first_nb;
    uint32_t next = forward_syntactic_ws_bytes(bs, first_nb + 1, n, true);
    if (next < n) probe_b = next;
  } else if (bs.text[first_nb] == '#') {
    uint32_t p = first_nb + 1;
    while (p < le && (bs.text[p] == ' ' || bs.text[p] == '\t')) ++p;
    if (p < le && bs.text[p] == '(') {
      open_paren_at_line = p;
      uint32_t next = forward_syntactic_ws_bytes(bs, p + 1, n, true);
      if (next < n) probe_b = next;
    }
  }
  TSNode cur = ts_node_descendant_for_byte_range(root, probe_b, probe_b);

  for (TSNode walk = align_decls ? cur : TSNode{}; !ts_node_is_null(walk);
       walk = ts_node_parent(walk)) {
    std::string_view nt = ts_node_type(walk);

    // 1.1 ANSI Port List Tabular Alignment (module/interface/program & function/task)
    if (nt == "list_of_port_declarations" || nt == "tf_port_list" ||
        nt == "class_constructor_arg_list") {
      uint32_t open_b = n;
      uint32_t close_b = n;
      if (nt == "list_of_port_declarations") {
        uint32_t pcount = ts_node_child_count(walk);
        for (uint32_t i = 0; i < pcount; ++i) {
          TSNode ch = ts_node_child(walk, i);
          std::string_view ct = ts_node_type(ch);
          if (ct == "(" && open_b == n) {
            open_b = ts_node_start_byte(ch);
          } else if (ct == ")") {
            close_b = ts_node_start_byte(ch);
          }
        }
        if (open_b == n) open_b = ts_node_start_byte(walk);
        if (close_b == n) {
          close_b = ts_node_end_byte(walk) > 0 ? ts_node_end_byte(walk) - 1 : 0;
        }
      } else {
        uint32_t prev =
            backward_syntactic_ws_bytes(bs, ts_node_start_byte(walk), 0, true);
        if (prev > 0 && bs.text[prev - 1] == '(') open_b = prev - 1;
        close_b =
            forward_syntactic_ws_bytes(bs, ts_node_end_byte(walk), n, true);
      }
      if (open_paren_at_line < n && open_paren_at_line != open_b) {
        return elisp_nil(env);
      }
      TSNode port_parent = ts_node_parent(walk);
      while (!ts_node_is_null(port_parent)) {
        TSNode gp = ts_node_parent(port_parent);
        if (ts_node_is_null(gp)) break;
        std::string_view gpt = ts_node_type(gp);
        if (gpt == "function_declaration" || gpt == "task_declaration" ||
            gpt == "function_prototype" || gpt == "task_prototype" ||
            gpt == "class_method" || gpt == "dpi_function_proto" ||
            gpt == "dpi_task_proto" || gpt == "dpi_import_export") {
          port_parent = gp;
        } else {
          break;
        }
      }
      uint32_t port_hdr_b = !ts_node_is_null(port_parent)
                                ? ts_node_start_byte(port_parent)
                                : ts_node_start_byte(walk);
      uint32_t header_sb = bs.line_start_byte(port_hdr_b);
      if (ls == header_sb) return elisp_nil(env);
      std::vector<AlignRow> rows;
      uint32_t count = ts_node_child_count(walk);
      for (uint32_t i = 0; i < count; ++i) {
        TSNode ch = ts_node_child(walk, i);
        std::string_view ct = ts_node_type(ch);
        TSNode port_node = ch;
        if (ct == "class_constructor_arg" && ts_node_child_count(ch) > 0) {
          TSNode inner = ts_node_child(ch, 0);
          if (std::string_view(ts_node_type(inner)) == "tf_port_item") {
            port_node = inner;
            ct = "tf_port_item";
          }
        }
        if (ct == "ansi_port_declaration" || ct == "port_declaration" ||
            ct == "tf_port_item") {
          AlignRow row;
          if (!parse_ansi_port_row(bs, port_node, row)) return elisp_nil(env);
          rows.push_back(std::move(row));
        }
      }
      if (rows.empty()) return elisp_nil(env);
      if (nt != "list_of_port_declarations" &&
          rows.back().line_eb > ts_node_end_byte(walk)) {
        close_b = forward_syntactic_ws_bytes(bs, rows.back().line_eb, n, true);
      }
      uint32_t header_nb = header_sb;
      while (header_nb < n &&
             (bs.text[header_nb] == ' ' || bs.text[header_nb] == '\t')) {
        ++header_nb;
      }
      uint32_t header_col = bs.byte_to_column(header_nb);
      uint32_t first_col = bs.byte_to_column(rows.front().first_nb);
      uint32_t base_col =
          compute_block_base_col(header_col, first_col, indent_level);
      auto opening_line =
          (open_b < n) ? make_closing_line_edit(bs, open_b, header_col, '(')
                       : std::nullopt;
      auto closing_line = make_closing_line_edit(bs, close_b, header_col, ')');
      return format_align_rows(env, bs, rows, base_col, false, true, align_dim,
                               align_sub_dim, align_comments, comment_distance,
                               closing_line, opening_line);
    }

    // 1.1b Parameter Port List Alignment (`#(...)` on module/interface/class)
    if (nt == "parameter_port_list") {
      if (!align_dim) return elisp_nil(env);
      uint32_t open_b = n;
      uint32_t close_b = n;
      uint32_t pcount = ts_node_child_count(walk);
      for (uint32_t i = 0; i < pcount; ++i) {
        TSNode ch = ts_node_child(walk, i);
        std::string_view ct = ts_node_type(ch);
        if (ct == "(" && open_b == n) {
          open_b = ts_node_start_byte(ch);
        } else if (ct == ")") {
          close_b = ts_node_start_byte(ch);
        }
      }
      if (open_b == n) open_b = ts_node_start_byte(walk);
      if (close_b == n) {
        close_b = ts_node_end_byte(walk) > 0 ? ts_node_end_byte(walk) - 1 : 0;
      }
      if (open_paren_at_line < n && open_paren_at_line != open_b) {
        return elisp_nil(env);
      }
      TSNode param_parent = ts_node_parent(walk);
      while (!ts_node_is_null(param_parent)) {
        TSNode gp = ts_node_parent(param_parent);
        if (ts_node_is_null(gp)) break;
        std::string_view gpt = ts_node_type(gp);
        if (gpt == "module_declaration" || gpt == "interface_declaration" ||
            gpt == "program_declaration" || gpt == "class_declaration" ||
            gpt == "package_declaration" || gpt == "checker_declaration" ||
            gpt == "module_ansi_header" || gpt == "module_nonansi_header" ||
            gpt == "interface_ansi_header" ||
            gpt == "interface_nonansi_header" ||
            gpt == "program_ansi_header" || gpt == "program_nonansi_header") {
          param_parent = gp;
        } else {
          break;
        }
      }
      uint32_t param_hdr_b = !ts_node_is_null(param_parent)
                                 ? ts_node_start_byte(param_parent)
                                 : ts_node_start_byte(walk);
      uint32_t header_sb = bs.line_start_byte(param_hdr_b);
      if (ls == header_sb) return elisp_nil(env);
      std::vector<AlignRow> rows;
      for (uint32_t i = 0; i < pcount; ++i) {
        TSNode ch = ts_node_child(walk, i);
        std::string_view ct = ts_node_type(ch);
        if (ct == "parameter_port_declaration" ||
            ct == "parameter_declaration" ||
            ct == "local_parameter_declaration") {
          AlignRow row;
          if (!parse_param_row(bs, ch, row)) return elisp_nil(env);
          rows.push_back(std::move(row));
        }
      }
      if (rows.empty()) return elisp_nil(env);
      uint32_t header_nb = header_sb;
      while (header_nb < n &&
             (bs.text[header_nb] == ' ' || bs.text[header_nb] == '\t')) {
        ++header_nb;
      }
      uint32_t header_col = bs.byte_to_column(header_nb);
      uint32_t first_col = bs.byte_to_column(rows.front().first_nb);
      uint32_t base_col =
          compute_block_base_col(header_col, first_col, indent_level);
      auto opening_line =
          (open_b < n) ? make_closing_line_edit(bs, open_b, header_col, '(')
                       : std::nullopt;
      auto closing_line = make_closing_line_edit(bs, close_b, header_col, ')');
      return format_align_rows(env, bs, rows, base_col, false, true, align_dim,
                               align_sub_dim, align_comments, comment_distance,
                               closing_line, opening_line);
    }

    // 1.2 Packed Struct, Union, and Enum Member Alignment
    if (nt == "data_type" || nt == "type_declaration") {
      TSNode dtype = walk;
      if (nt == "type_declaration") {
        dtype = {};
        uint32_t tcount = ts_node_child_count(walk);
        for (uint32_t i = 0; i < tcount; ++i) {
          TSNode ch = ts_node_child(walk, i);
          if (std::string_view(ts_node_type(ch)) == "data_type") {
            dtype = ch;
            break;
          }
        }
      }
      if (!ts_node_is_null(dtype)) {
        bool has_struct_union = false;
        bool has_enum = false;
        uint32_t open_b = n;
        uint32_t close_b = n;
        uint32_t count = ts_node_child_count(dtype);
        for (uint32_t i = 0; i < count; ++i) {
          TSNode ch = ts_node_child(dtype, i);
          std::string_view ct = ts_node_type(ch);
          if (ct == "struct_union") {
            has_struct_union = true;
          } else if (ct == "enum" || ct == "enum_base_type" ||
                     ct == "enum_name_declaration") {
            has_enum = true;
          } else if (ct == "{" && open_b == n) {
            open_b = ts_node_start_byte(ch);
          } else if (ct == "}") {
            close_b = ts_node_start_byte(ch);
          }
        }
        if (close_b == n) {
          close_b =
              ts_node_end_byte(dtype) > 0 ? ts_node_end_byte(dtype) - 1 : 0;
        }
        if (has_struct_union) {
          uint32_t header_sb = bs.line_start_byte(ts_node_start_byte(dtype));
          if (ls == header_sb) return elisp_nil(env);
          std::vector<AlignRow> rows;
          for (uint32_t i = 0; i < count; ++i) {
            TSNode ch = ts_node_child(dtype, i);
            if (std::string_view(ts_node_type(ch)) == "struct_union_member") {
              AlignRow row;
              if (!parse_decl_or_member_row(bs, ch, row)) return elisp_nil(env);
              rows.push_back(std::move(row));
            }
          }
          if (rows.empty()) return elisp_nil(env);
          uint32_t header_nb = header_sb;
          while (header_nb < n &&
                 (bs.text[header_nb] == ' ' || bs.text[header_nb] == '\t')) {
            ++header_nb;
          }
          uint32_t header_col = bs.byte_to_column(header_nb);
          uint32_t first_col = bs.byte_to_column(rows.front().first_nb);
          uint32_t base_col =
              compute_block_base_col(header_col, first_col, indent_level);
          auto opening_line =
              (open_b < n)
                  ? make_closing_line_edit(bs, open_b, header_col, '{')
                  : std::nullopt;
          auto closing_line =
              make_closing_line_edit(bs, close_b, header_col, '}');
          return format_align_rows(env, bs, rows, base_col, false, true,
                                   align_dim, align_sub_dim, align_comments,
                                   comment_distance, closing_line,
                                   opening_line);
        }
        if (has_enum && align_dim) {
          uint32_t header_sb = bs.line_start_byte(ts_node_start_byte(dtype));
          if (ls == header_sb) return elisp_nil(env);
          std::vector<AlignRow> rows;
          for (uint32_t i = 0; i < count; ++i) {
            TSNode ch = ts_node_child(dtype, i);
            if (std::string_view(ts_node_type(ch)) == "enum_name_declaration") {
              AlignRow row;
              if (!parse_enum_member_row(bs, ch, row)) return elisp_nil(env);
              rows.push_back(std::move(row));
            }
          }
          if (rows.empty()) return elisp_nil(env);
          uint32_t header_nb = header_sb;
          while (header_nb < n &&
                 (bs.text[header_nb] == ' ' || bs.text[header_nb] == '\t')) {
            ++header_nb;
          }
          uint32_t header_col = bs.byte_to_column(header_nb);
          uint32_t first_col = bs.byte_to_column(rows.front().first_nb);
          uint32_t base_col =
              compute_block_base_col(header_col, first_col, indent_level);
          auto opening_line =
              (open_b < n)
                  ? make_closing_line_edit(bs, open_b, header_col, '{')
                  : std::nullopt;
          auto closing_line =
              make_closing_line_edit(bs, close_b, header_col, '}');
          return format_align_rows(env, bs, rows, base_col, false, true,
                                   align_dim, align_sub_dim, align_comments,
                                   comment_distance, closing_line,
                                   opening_line);
        }
      }
    }

    // 1.3 Module Instantiation Port & Parameter Connection Alignment
    if (nt == "list_of_port_connections" ||
        nt == "list_of_checker_port_connections" ||
        nt == "list_of_parameter_value_assignments") {
      uint32_t prev_b =
          backward_syntactic_ws_bytes(bs, ts_node_start_byte(walk), 0, true);
      uint32_t open_b =
          (prev_b > 0 && bs.text[prev_b - 1] == '(') ? (prev_b - 1) : n;
      if (open_paren_at_line < n && open_paren_at_line != open_b) {
        return elisp_nil(env);
      }
      TSNode inst_parent = ts_node_parent(walk);
      TSNode inst_outer =
          (!ts_node_is_null(inst_parent) &&
           std::string_view(ts_node_type(ts_node_parent(inst_parent))) ==
               "module_instantiation")
              ? ts_node_parent(inst_parent)
              : inst_parent;
      TSNode check_node = !ts_node_is_null(inst_outer) ? inst_outer : walk;
      if (bs.node_text(check_node).find("/*AUTOINST*/") !=
          std::string_view::npos) {
        return elisp_nil(env);
      }
      uint32_t container_sb = !ts_node_is_null(inst_outer)
                                  ? ts_node_start_byte(inst_outer)
                                  : ts_node_start_byte(walk);
      uint32_t header_sb = bs.line_start_byte(container_sb);
      if (ls == header_sb) return elisp_nil(env);
      std::vector<AlignRow> rows;
      uint32_t count = ts_node_child_count(walk);
      for (uint32_t i = 0; i < count; ++i) {
        TSNode ch = ts_node_child(walk, i);
        std::string_view ct = ts_node_type(ch);
        if (ct == "named_port_connection" ||
            ct == "named_checker_port_connection" ||
            ct == "named_parameter_assignment") {
          AlignRow row;
          if (!parse_inst_port_row(bs, ch, row)) return elisp_nil(env);
          rows.push_back(std::move(row));
        }
      }
      if (rows.empty()) return elisp_nil(env);
      uint32_t header_nb = header_sb;
      while (header_nb < n &&
             (bs.text[header_nb] == ' ' || bs.text[header_nb] == '\t')) {
        ++header_nb;
      }
      uint32_t header_col = bs.byte_to_column(header_nb);
      uint32_t first_col = bs.byte_to_column(rows.front().first_nb);
      uint32_t base_col =
          compute_block_base_col(header_col, first_col, indent_level);
      auto opening_line =
          (open_b < n) ? make_closing_line_edit(bs, open_b, header_col, '(')
                       : std::nullopt;
      uint32_t close_b =
          forward_syntactic_ws_bytes(bs, ts_node_end_byte(walk), n, true);
      auto closing_line = make_closing_line_edit(bs, close_b, header_col, ')');
      return format_align_rows(env, bs, rows, base_col, true, false, align_dim,
                               align_sub_dim, align_comments, comment_distance,
                               closing_line, opening_line);
    }
  }

  uint32_t cur_line_idx = bs.byte_to_line(first_nb) - 1;
  uint32_t total_lines = static_cast<uint32_t>(bs.line_starts.size());

  // 1.4 Contiguous Signal / Net Declarations
  TSNode start_decl = align_decls
                          ? find_single_line_decl_at_byte(bs, root, first_nb)
                          : TSNode{};
  if (!ts_node_is_null(start_decl)) {
    AlignRow center_row;
    if (parse_decl_or_member_row(bs, start_decl, center_row)) {
      TSNode center_scope = find_enclosing_stmt_scope(start_decl);
      std::vector<AlignRow> before_rows;
      for (uint32_t l = cur_line_idx; l > 0;) {
        --l;
        uint32_t l_sb = bs.line_starts[l];
        uint32_t l_eb = bs.line_end_byte(l_sb);
        uint32_t raw_l_nb = l_sb;
        while (raw_l_nb < l_eb &&
               (bs.text[raw_l_nb] == ' ' || bs.text[raw_l_nb] == '\t')) {
          ++raw_l_nb;
        }
        if (raw_l_nb >= l_eb) break;
        uint32_t nb = forward_syntactic_ws_bytes(bs, l_sb, n, true);
        if (nb >= l_eb) continue;
        TSNode dnode = find_single_line_decl_at_byte(bs, root, nb);
        if (ts_node_is_null(dnode) ||
            !ts_node_eq(find_enclosing_stmt_scope(dnode), center_scope)) {
          break;
        }
        AlignRow r;
        if (!parse_decl_or_member_row(bs, dnode, r)) break;
        before_rows.push_back(std::move(r));
      }

      std::vector<AlignRow> rows;
      rows.reserve(before_rows.size() + 1);
      for (auto it = before_rows.rbegin(); it != before_rows.rend(); ++it) {
        rows.push_back(std::move(*it));
      }
      rows.push_back(std::move(center_row));

      for (uint32_t l = cur_line_idx + 1; l < total_lines; ++l) {
        uint32_t l_sb = bs.line_starts[l];
        uint32_t l_eb = bs.line_end_byte(l_sb);
        uint32_t raw_l_nb = l_sb;
        while (raw_l_nb < l_eb &&
               (bs.text[raw_l_nb] == ' ' || bs.text[raw_l_nb] == '\t')) {
          ++raw_l_nb;
        }
        if (raw_l_nb >= l_eb) break;
        uint32_t nb = forward_syntactic_ws_bytes(bs, l_sb, n, true);
        if (nb >= l_eb) continue;
        TSNode dnode = find_single_line_decl_at_byte(bs, root, nb);
        if (ts_node_is_null(dnode) ||
            !ts_node_eq(find_enclosing_stmt_scope(dnode), center_scope)) {
          break;
        }
        AlignRow r;
        if (!parse_decl_or_member_row(bs, dnode, r)) break;
        rows.push_back(std::move(r));
      }

      uint32_t base_col =
          compute_scope_base_col(bs, start_decl, rows, indent_level);
      return format_align_rows(env, bs, rows, base_col, false, false, align_dim,
                               align_sub_dim, align_comments, comment_distance);
    }
  }

  // 1.4b Contiguous Parameter / Localparam Declarations
  if (align_decls && align_dim) {
    TSNode start_param =
        find_single_line_param_decl_at_byte(bs, root, first_nb);
    if (!ts_node_is_null(start_param)) {
      AlignRow center_row;
      if (parse_param_row(bs, start_param, center_row)) {
        TSNode center_scope = find_enclosing_stmt_scope(start_param);
        std::vector<AlignRow> before_rows;
        for (uint32_t l = cur_line_idx; l > 0;) {
          --l;
          uint32_t l_sb = bs.line_starts[l];
          uint32_t l_eb = bs.line_end_byte(l_sb);
          uint32_t raw_l_nb = l_sb;
          while (raw_l_nb < l_eb &&
                 (bs.text[raw_l_nb] == ' ' || bs.text[raw_l_nb] == '\t')) {
            ++raw_l_nb;
          }
          if (raw_l_nb >= l_eb) break;
          uint32_t nb = forward_syntactic_ws_bytes(bs, l_sb, n, true);
          if (nb >= l_eb) continue;
          TSNode pnode = find_single_line_param_decl_at_byte(bs, root, nb);
          if (ts_node_is_null(pnode) ||
              !ts_node_eq(find_enclosing_stmt_scope(pnode), center_scope)) {
            break;
          }
          AlignRow r;
          if (!parse_param_row(bs, pnode, r)) break;
          before_rows.push_back(std::move(r));
        }

        std::vector<AlignRow> rows;
        rows.reserve(before_rows.size() + 1);
        for (auto it = before_rows.rbegin(); it != before_rows.rend(); ++it) {
          rows.push_back(std::move(*it));
        }
        rows.push_back(std::move(center_row));

        for (uint32_t l = cur_line_idx + 1; l < total_lines; ++l) {
          uint32_t l_sb = bs.line_starts[l];
          uint32_t l_eb = bs.line_end_byte(l_sb);
          uint32_t raw_l_nb = l_sb;
          while (raw_l_nb < l_eb &&
                 (bs.text[raw_l_nb] == ' ' || bs.text[raw_l_nb] == '\t')) {
            ++raw_l_nb;
          }
          if (raw_l_nb >= l_eb) break;
          uint32_t nb = forward_syntactic_ws_bytes(bs, l_sb, n, true);
          if (nb >= l_eb) continue;
          TSNode pnode = find_single_line_param_decl_at_byte(bs, root, nb);
          if (ts_node_is_null(pnode) ||
              !ts_node_eq(find_enclosing_stmt_scope(pnode), center_scope)) {
            break;
          }
          AlignRow r;
          if (!parse_param_row(bs, pnode, r)) break;
          rows.push_back(std::move(r));
        }

        uint32_t base_col =
            compute_scope_base_col(bs, start_param, rows, indent_level);
        return format_align_rows(env, bs, rows, base_col, false, false,
                                 align_dim, align_sub_dim, align_comments,
                                 comment_distance);
      }
    }
  }

  // 1.5 Contiguous Procedural (`=`, `<=`) and Continuous (`assign`) Assignments
  if (align_assignments) {
    bool center_is_cont = false;
    TSNode start_assign = find_single_line_assign_at_byte(
        bs, root, first_nb, align_assign_expr, &center_is_cont);
    if (!ts_node_is_null(start_assign)) {
      AlignRow center_row;
      if (parse_assign_row(bs, start_assign, center_is_cont, center_row)) {
        TSNode center_scope = find_enclosing_stmt_scope(start_assign);
        std::vector<AlignRow> before_rows;
        for (uint32_t l = cur_line_idx; l > 0;) {
          --l;
          uint32_t l_sb = bs.line_starts[l];
          uint32_t l_eb = bs.line_end_byte(l_sb);
          uint32_t raw_l_nb = l_sb;
          while (raw_l_nb < l_eb &&
                 (bs.text[raw_l_nb] == ' ' || bs.text[raw_l_nb] == '\t')) {
            ++raw_l_nb;
          }
          if (raw_l_nb >= l_eb) break;
          uint32_t nb = forward_syntactic_ws_bytes(bs, l_sb, n, true);
          if (nb >= l_eb) continue;
          bool is_cont = false;
          TSNode anode = find_single_line_assign_at_byte(
              bs, root, nb, align_assign_expr, &is_cont);
          if (ts_node_is_null(anode) || is_cont != center_is_cont ||
              !ts_node_eq(find_enclosing_stmt_scope(anode), center_scope)) {
            break;
          }
          AlignRow r;
          if (!parse_assign_row(bs, anode, is_cont, r)) break;
          before_rows.push_back(std::move(r));
        }

        std::vector<AlignRow> rows;
        rows.reserve(before_rows.size() + 1);
        for (auto it = before_rows.rbegin(); it != before_rows.rend(); ++it) {
          rows.push_back(std::move(*it));
        }
        rows.push_back(std::move(center_row));

        for (uint32_t l = cur_line_idx + 1; l < total_lines; ++l) {
          uint32_t l_sb = bs.line_starts[l];
          uint32_t l_eb = bs.line_end_byte(l_sb);
          uint32_t raw_l_nb = l_sb;
          while (raw_l_nb < l_eb &&
                 (bs.text[raw_l_nb] == ' ' || bs.text[raw_l_nb] == '\t')) {
            ++raw_l_nb;
          }
          if (raw_l_nb >= l_eb) break;
          uint32_t nb = forward_syntactic_ws_bytes(bs, l_sb, n, true);
          if (nb >= l_eb) continue;
          bool is_cont = false;
          TSNode anode = find_single_line_assign_at_byte(
              bs, root, nb, align_assign_expr, &is_cont);
          if (ts_node_is_null(anode) || is_cont != center_is_cont ||
              !ts_node_eq(find_enclosing_stmt_scope(anode), center_scope)) {
            break;
          }
          AlignRow r;
          if (!parse_assign_row(bs, anode, is_cont, r)) break;
          rows.push_back(std::move(r));
        }

        uint32_t base_col =
            compute_scope_base_col(bs, start_assign, rows, indent_level);
        return format_align_rows(env, bs, rows, base_col, false, false,
                                 align_dim, align_sub_dim, align_comments,
                                 comment_distance);
      }
    }
  }

  return elisp_nil(env);
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
  bind_function(
      env, "verilog-ts-align-block", 0, 8, Fverilog_ts_align_block,
      "Return list of (LINE-START LINE-END NEW-TEXT) for tabular alignment at POS.");
}

} // namespace verilog_ts

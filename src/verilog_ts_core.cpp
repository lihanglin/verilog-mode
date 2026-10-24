#include "verilog_ts_common.hpp"

int plugin_is_GPL_compatible;

namespace verilog_ts {

namespace {

constexpr size_t kMaxCachedBuffers = 32;

struct CacheEntry {
  intmax_t buf_hash = 0;
  std::string buf_name;
  std::unique_ptr<BufferState> state;
};

std::list<CacheEntry> g_buffer_lru;
TSParser *g_parser = nullptr;
std::unordered_map<std::string, TSQuery *> g_query_cache;
std::vector<emacs_value> g_temp_refs;

emacs_value protect_temp_ref(emacs_env *env, emacs_value val) {
  if (val && env->non_local_exit_check(env) == emacs_funcall_exit_return) {
    emacs_value gref = env->make_global_ref(env, val);
    if (gref && env->non_local_exit_check(env) == emacs_funcall_exit_return) {
      g_temp_refs.push_back(gref);
    }
  }
  return val;
}

void flush_temp_refs(emacs_env *env) {
  if (g_temp_refs.empty()) return;
  auto exit_state = env->non_local_exit_check(env);
  emacs_value sym = nullptr;
  emacs_value data = nullptr;
  if (exit_state != emacs_funcall_exit_return) {
    exit_state = env->non_local_exit_get(env, &sym, &data);
    env->non_local_exit_clear(env);
  }
  for (emacs_value gref : g_temp_refs) {
    env->free_global_ref(env, gref);
  }
  g_temp_refs.clear();
  if (exit_state == emacs_funcall_exit_signal) {
    env->non_local_exit_signal(env, sym, data);
  } else if (exit_state == emacs_funcall_exit_throw) {
    env->non_local_exit_throw(env, sym, data);
  }
}

TSParser *ensure_parser() {
  if (!g_parser) {
    g_parser = ts_parser_new();
    ts_parser_set_language(g_parser, tree_sitter_systemverilog());
  }
  return g_parser;
}

const std::unordered_set<std::string_view> &verilog_keyword_set() {
  static const std::unordered_set<std::string_view> kKeywords = {
      "`begin_keywords", "`celldefine", "`default_nettype", "`define",
      "`else", "`elsif", "`end_keywords", "`endcelldefine", "`endif",
      "`ifdef", "`ifndef", "`include", "`line", "`nounconnected_drive",
      "`pragma", "`resetall", "`timescale", "`unconnected_drive", "`undef",
      "`undefineall",
      "after", "alias", "always", "always_comb", "always_ff", "always_latch",
      "analog", "and", "assert", "assign", "assume", "automatic", "before",
      "begin", "bind", "bins", "binsof", "bit", "break", "buf", "bufif0",
      "bufif1", "byte", "case", "casex", "casez", "cell", "chandle", " class",
      "class", "clocking", "cmos", "config", "const", "constraint", "context",
      "continue", "cover", "covergroup", "coverpoint", "cross", "deassign",
      "default", "defparam", "design", "disable", "dist", "do", "edge", "else",
      "end", "endcase", "endclass", "endclocking", "endconfig", "endfunction",
      "endgenerate", "endgroup", "endinterface", "endmodule", "endpackage",
      "endprimitive", "endprogram", "endproperty", "endspecify", "endsequence",
      "endtable", "endtask", "enum", "event", "expect", "export", "extends",
      "extern", "final", "first_match", "for", "force", "foreach", "forever",
      "fork", "forkjoin", "function", "generate", "genvar", "highz0", "highz1",
      "if", "iff", "ifnone", "ignore_bins", "illegal_bins", "import", "incdir",
      "include", "initial", "inout", "input", "inside", "instance", "int",
      "integer", "interface", "intersect", "join", "join_any", "join_none",
      "large", "liblist", "library", "local", "localparam", "logic", "longint",
      "macromodule", "mailbox", "matches", "medium", "modport", "module",
      "nand", "negedge", "new", "nmos", "nor", "noshowcancelled", "not",
      "notif0", "notif1", "null", "or", "output", "package", "packed",
      "parameter", "pmos", "posedge", "primitive", "priority", "program",
      "property", "protected", "pull0", "pull1", "pulldown", "pullup",
      "pulsestyle_onevent", "pulsestyle_ondetect", "pure", "rand", "randc",
      "randcase", "randsequence", "rcmos", "real", "realtime", "ref", "reg",
      "release", "repeat", "return", "rnmos", "rpmos", "rtran", "rtranif0",
      "rtranif1", "scalared", "semaphore", "sequence", "shortint", "shortreal",
      "showcancelled", "signed", "small", "solve", "specify", "specparam",
      "static", "string", "strong0", "strong1", "struct", "super", "supply0",
      "supply1", "table", "tagged", "task", "this", "throughout", "time",
      "timeprecision", "timeunit", "tran", "tranif0", "tranif1", "tri", "tri0",
      "tri1", "triand", "trior", "trireg", "type", "typedef", "union",
      "unique", "unsigned", "use", "uwire", "var", "vectored", "virtual",
      "void", "wait", "wait_order", "wand", "weak0", "weak1", "while",
      "wildcard", "wire", "with", "within", "wor", "xnor", "xor",
      // 1800-2009
      "accept_on", "checker", "endchecker", "eventually", "global", "implies",
      "let", "nexttime", "reject_on", "restrict", "s_always", "s_eventually",
      "s_nexttime", "s_until", "s_until_with", "strong", "sync_accept_on",
      "sync_reject_on", "unique0", "until", "until_with", "untyped", "weak",
      // 1800-2012
      "implements", "interconnect", "nettype", "soft",
      // AMS
      "connectmodule", "endconnectmodule"};
  return kKeywords;
}

} // namespace

bool is_verilog_keyword(std::string_view word) {
  return verilog_keyword_set().count(word) > 0;
}

emacs_value elisp_nil(emacs_env *env) { return env->intern(env, "nil"); }

emacs_value elisp_t(emacs_env *env) { return env->intern(env, "t"); }

emacs_value elisp_intern(emacs_env *env, const char *name) {
  return env->intern(env, name);
}

emacs_value elisp_string(emacs_env *env, std::string_view sv) {
  std::string s(sv);
  emacs_value v =
      env->make_string(env, s.c_str(), static_cast<ptrdiff_t>(s.size()));
  return protect_temp_ref(env, v);
}

emacs_value elisp_int(emacs_env *env, intmax_t val) {
  return env->make_integer(env, val);
}

emacs_value elisp_cons(emacs_env *env, emacs_value car, emacs_value cdr) {
  emacs_value args[2] = {car, cdr};
  emacs_value v = env->funcall(env, env->intern(env, "cons"), 2, args);
  return protect_temp_ref(env, v);
}

emacs_value elisp_list(emacs_env *env, const std::vector<emacs_value> &elems) {
  if (elems.empty()) return elisp_nil(env);
  emacs_value v = env->funcall(env, env->intern(env, "list"),
                               static_cast<ptrdiff_t>(elems.size()),
                               const_cast<emacs_value *>(elems.data()));
  return protect_temp_ref(env, v);
}

emacs_value elisp_vector(emacs_env *env,
                         const std::vector<emacs_value> &elems) {
  emacs_value v = env->funcall(env, env->intern(env, "vector"),
                               static_cast<ptrdiff_t>(elems.size()),
                               const_cast<emacs_value *>(elems.data()));
  return protect_temp_ref(env, v);
}

std::string extract_string(emacs_env *env, emacs_value val) {
  if (!env->is_not_nil(env, val)) return {};
  ptrdiff_t size = 0;
  if (!env->copy_string_contents(env, val, nullptr, &size) || size <= 1) {
    return {};
  }
  std::string buf(static_cast<size_t>(size), '\0');
  if (!env->copy_string_contents(env, val, buf.data(), &size)) {
    return {};
  }
  buf.resize(static_cast<size_t>(size - 1));
  return buf;
}

intmax_t get_point(emacs_env *env) {
  emacs_value pt = env->funcall(env, env->intern(env, "point"), 0, nullptr);
  return env->extract_integer(env, pt);
}

intmax_t get_point_min(emacs_env *env) {
  emacs_value pt = env->funcall(env, env->intern(env, "point-min"), 0, nullptr);
  return env->extract_integer(env, pt);
}

intmax_t get_point_max(emacs_env *env) {
  emacs_value pt = env->funcall(env, env->intern(env, "point-max"), 0, nullptr);
  return env->extract_integer(env, pt);
}

BufferState &get_buffer_state(emacs_env *env, bool need_tree) {
  emacs_value cur_buf =
      env->funcall(env, env->intern(env, "current-buffer"), 0, nullptr);
  emacs_value hash_val =
      env->funcall(env, env->intern(env, "sxhash-eq"), 1, &cur_buf);
  intmax_t buf_hash = env->extract_integer(env, hash_val);

  emacs_value tick_val = env->funcall(
      env, env->intern(env, "buffer-chars-modified-tick"), 0, nullptr);
  intmax_t tick = env->extract_integer(env, tick_val);

  intmax_t pmin = get_point_min(env);
  intmax_t pmax = get_point_max(env);

  for (auto it = g_buffer_lru.begin(); it != g_buffer_lru.end(); ++it) {
    if (it->buf_hash == buf_hash) {
      if (it != g_buffer_lru.begin()) {
        g_buffer_lru.splice(g_buffer_lru.begin(), g_buffer_lru, it);
      }
      BufferState &bs = *it->state;
      if (bs.chars_modified_tick == tick && bs.point_min == pmin &&
          bs.point_max == pmax) {
        if (need_tree && bs.tree == nullptr) {
          TSParser *parser = ensure_parser();
          bs.tree = ts_parser_parse_string(parser, nullptr, bs.text.data(),
                                           static_cast<uint32_t>(bs.text.size()));
        }
        return bs;
      }
      // Re-scan modified buffer
      emacs_value sym_tw = env->intern(env, "tab-width");
      emacs_value tw_val =
          env->funcall(env, env->intern(env, "symbol-value"), 1, &sym_tw);
      bs.tab_width = static_cast<int>(env->extract_integer(env, tw_val));
      bs.chars_modified_tick = tick;
      bs.point_min = pmin;
      bs.point_max = pmax;

      emacs_value sub_args[2] = {elisp_int(env, pmin), elisp_int(env, pmax)};
      emacs_value str_val = env->funcall(
          env, env->intern(env, "buffer-substring-no-properties"), 2, sub_args);
      bs.text = extract_string(env, str_val);

      size_t char_len = static_cast<size_t>(std::max<intmax_t>(0, pmax - pmin));
      bs.is_ascii = (bs.text.size() == char_len);
      bs.multibyte_cont_bytes.clear();
      bs.line_starts.clear();
      bs.line_starts.push_back(0);
      for (uint32_t i = 0; i < bs.text.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(bs.text[i]);
        if (!bs.is_ascii && (c & 0xC0) == 0x80) {
          bs.multibyte_cont_bytes.push_back(i);
        }
        if (c == '\n') {
          bs.line_starts.push_back(i + 1);
        }
      }

      if (bs.tree) {
        ts_tree_delete(bs.tree);
        bs.tree = nullptr;
      }
      if (need_tree) {
        TSParser *parser = ensure_parser();
        bs.tree = ts_parser_parse_string(parser, nullptr, bs.text.data(),
                                         static_cast<uint32_t>(bs.text.size()));
      }
      build_buffer_spans(bs);
      return bs;
    }
  }

  // Create new cache entry
  if (g_buffer_lru.size() >= kMaxCachedBuffers) {
    g_buffer_lru.pop_back();
  }
  CacheEntry entry;
  entry.buf_hash = buf_hash;
  entry.state = std::make_unique<BufferState>();
  g_buffer_lru.push_front(std::move(entry));

  BufferState &bs = *g_buffer_lru.front().state;
  emacs_value sym_tw = env->intern(env, "tab-width");
  emacs_value tw_val =
      env->funcall(env, env->intern(env, "symbol-value"), 1, &sym_tw);
  bs.tab_width = static_cast<int>(env->extract_integer(env, tw_val));
  bs.chars_modified_tick = tick;
  bs.point_min = pmin;
  bs.point_max = pmax;

  emacs_value sub_args[2] = {elisp_int(env, pmin), elisp_int(env, pmax)};
  emacs_value str_val = env->funcall(
      env, env->intern(env, "buffer-substring-no-properties"), 2, sub_args);
  bs.text = extract_string(env, str_val);

  size_t char_len = static_cast<size_t>(std::max<intmax_t>(0, pmax - pmin));
  bs.is_ascii = (bs.text.size() == char_len);
  bs.multibyte_cont_bytes.clear();
  bs.line_starts.clear();
  bs.line_starts.push_back(0);
  for (uint32_t i = 0; i < bs.text.size(); ++i) {
    unsigned char c = static_cast<unsigned char>(bs.text[i]);
    if (!bs.is_ascii && (c & 0xC0) == 0x80) {
      bs.multibyte_cont_bytes.push_back(i);
    }
    if (c == '\n') {
      bs.line_starts.push_back(i + 1);
    }
  }

  if (need_tree) {
    TSParser *parser = ensure_parser();
    bs.tree = ts_parser_parse_string(parser, nullptr, bs.text.data(),
                                     static_cast<uint32_t>(bs.text.size()));
  }
  build_buffer_spans(bs);
  return bs;
}

TSQuery *get_or_compile_query(const std::string &query_str,
                              uint32_t *err_offset,
                              TSQueryError *err_type) {
  auto it = g_query_cache.find(query_str);
  if (it != g_query_cache.end()) {
    return it->second;
  }
  uint32_t offset = 0;
  TSQueryError type = TSQueryErrorNone;
  TSQuery *q = ts_query_new(tree_sitter_systemverilog(), query_str.data(),
                            static_cast<uint32_t>(query_str.size()), &offset,
                            &type);
  if (err_offset) *err_offset = offset;
  if (err_type) *err_type = type;
  if (q) {
    g_query_cache.emplace(query_str, q);
  }
  return q;
}

emacs_value make_node_plist(emacs_env *env, const BufferState &bs, TSNode node,
                            bool include_sexp) {
  if (ts_node_is_null(node)) {
    return elisp_nil(env);
  }
  uint32_t sb = ts_node_start_byte(node);
  uint32_t eb = ts_node_end_byte(node);
  TSPoint sp = ts_node_start_point(node);
  TSPoint ep = ts_node_end_point(node);
  std::string_view ntext = bs.slice(sb, std::min<uint32_t>(eb, sb + 512));

  std::vector<emacs_value> elems = {
      elisp_intern(env, ":type"),
      elisp_string(env, ts_node_type(node)),
      elisp_intern(env, ":start"),
      elisp_int(env, bs.byte_to_pos(sb)),
      elisp_intern(env, ":end"),
      elisp_int(env, bs.byte_to_pos(eb)),
      elisp_intern(env, ":start-line"),
      elisp_int(env, static_cast<intmax_t>(sp.row) + 1),
      elisp_intern(env, ":start-col"),
      elisp_int(env, static_cast<intmax_t>(sp.column)),
      elisp_intern(env, ":end-line"),
      elisp_int(env, static_cast<intmax_t>(ep.row) + 1),
      elisp_intern(env, ":end-col"),
      elisp_int(env, static_cast<intmax_t>(ep.column)),
      elisp_intern(env, ":named"),
      ts_node_is_named(node) ? elisp_t(env) : elisp_nil(env),
      elisp_intern(env, ":error"),
      (ts_node_is_error(node) || ts_node_has_error(node)) ? elisp_t(env)
                                                          : elisp_nil(env),
      elisp_intern(env, ":text"),
      elisp_string(env, ntext)};

  if (include_sexp) {
    char *sexp = ts_node_string(node);
    elems.push_back(elisp_intern(env, ":sexp"));
    elems.push_back(elisp_string(env, sexp ? sexp : ""));
    if (sexp) free(sexp);
  }
  return elisp_list(env, elems);
}

namespace {

emacs_value module_trampoline(emacs_env *env, ptrdiff_t nargs,
                              emacs_value args[], void *data) EMACS_NOEXCEPT {
  auto fn = reinterpret_cast<emacs_value (*)(
      emacs_env *, ptrdiff_t, emacs_value *, void *) EMACS_NOEXCEPT>(data);
  emacs_value ret = fn(env, nargs, args, nullptr);
  flush_temp_refs(env);
  return ret;
}

} // namespace

void bind_function(
    emacs_env *env, const char *name, ptrdiff_t min_arity, ptrdiff_t max_arity,
    emacs_value (*func)(emacs_env *env, ptrdiff_t nargs, emacs_value args[],
                        void *data) EMACS_NOEXCEPT,
    const char *doc) {
  emacs_value fn = env->make_function(env, min_arity, max_arity,
                                      module_trampoline, doc,
                                      reinterpret_cast<void *>(func));
  emacs_value sym = env->intern(env, name);
  emacs_value fset_args[2] = {sym, fn};
  env->funcall(env, env->intern(env, "fset"), 2, fset_args);
}

namespace {

emacs_value Fverilog_ts_version(emacs_env *env, ptrdiff_t nargs,
                                emacs_value args[], void *data) EMACS_NOEXCEPT {
  return elisp_string(env, "verilog-ts-core 1.0 (tree-sitter IEEE 1800-2023)");
}

emacs_value Fverilog_ts_parse_buffer(emacs_env *env, ptrdiff_t nargs,
                                     emacs_value args[],
                                     void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env);
  if (!bs.tree) return elisp_nil(env);
  TSNode root = ts_tree_root_node(bs.tree);
  if (ts_node_has_error(root)) {
    return elisp_intern(env, "has-errors");
  }
  return elisp_t(env);
}

emacs_value Fverilog_ts_cache_flush(emacs_env *env, ptrdiff_t nargs,
                                    emacs_value args[],
                                    void *data) EMACS_NOEXCEPT {
  if (nargs >= 1 && env->is_not_nil(env, args[0])) {
    g_buffer_lru.clear();
    return elisp_t(env);
  }
  emacs_value cur_buf =
      env->funcall(env, env->intern(env, "current-buffer"), 0, nullptr);
  emacs_value hash_val =
      env->funcall(env, env->intern(env, "sxhash-eq"), 1, &cur_buf);
  intmax_t buf_hash = env->extract_integer(env, hash_val);

  for (auto it = g_buffer_lru.begin(); it != g_buffer_lru.end(); ++it) {
    if (it->buf_hash == buf_hash) {
      g_buffer_lru.erase(it);
      break;
    }
  }
  return elisp_t(env);
}

emacs_value Fverilog_ts_root_sexp(emacs_env *env, ptrdiff_t nargs,
                                  emacs_value args[],
                                  void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env);
  if (!bs.tree) return elisp_nil(env);
  TSNode root = ts_tree_root_node(bs.tree);
  char *sexp = ts_node_string(root);
  emacs_value res = elisp_string(env, sexp ? sexp : "");
  if (sexp) free(sexp);
  return res;
}

emacs_value Fverilog_ts_node_at(emacs_env *env, ptrdiff_t nargs,
                                emacs_value args[],
                                void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env);
  if (!bs.tree) return elisp_nil(env);

  intmax_t pos = (nargs >= 1 && env->is_not_nil(env, args[0]))
                     ? env->extract_integer(env, args[0])
                     : get_point(env);
  bool named_only = (nargs >= 2 && env->is_not_nil(env, args[1]));

  uint32_t byte = bs.pos_to_byte(pos);
  if (byte >= bs.text.size() && byte > 0) {
    byte = static_cast<uint32_t>(bs.text.size() - 1);
  }
  TSNode root = ts_tree_root_node(bs.tree);
  TSNode node = named_only
                    ? ts_node_named_descendant_for_byte_range(root, byte, byte)
                    : ts_node_descendant_for_byte_range(root, byte, byte);
  return make_node_plist(env, bs, node, true);
}

emacs_value Fverilog_ts_enclosing_node(emacs_env *env, ptrdiff_t nargs,
                                       emacs_value args[],
                                       void *data) EMACS_NOEXCEPT {
  if (nargs < 1 || !env->is_not_nil(env, args[0])) return elisp_nil(env);
  BufferState &bs = get_buffer_state(env);
  if (!bs.tree) return elisp_nil(env);

  std::unordered_set<std::string> wanted;
  emacs_value types_arg = args[0];
  emacs_value type_sym = env->type_of(env, types_arg);
  if (env->eq(env, type_sym, env->intern(env, "string"))) {
    wanted.insert(extract_string(env, types_arg));
  } else if (env->eq(env, type_sym, env->intern(env, "cons"))) {
    emacs_value cur = types_arg;
    while (env->is_not_nil(env, cur)) {
      emacs_value head = env->funcall(env, env->intern(env, "car"), 1, &cur);
      std::string s = extract_string(env, head);
      if (!s.empty()) wanted.insert(std::move(s));
      cur = env->funcall(env, env->intern(env, "cdr"), 1, &cur);
    }
  }
  if (wanted.empty()) return elisp_nil(env);

  intmax_t pos = (nargs >= 2 && env->is_not_nil(env, args[1]))
                     ? env->extract_integer(env, args[1])
                     : get_point(env);
  uint32_t byte = bs.pos_to_byte(pos);
  if (byte >= bs.text.size() && byte > 0) {
    byte = static_cast<uint32_t>(bs.text.size() - 1);
  }
  TSNode root = ts_tree_root_node(bs.tree);
  TSNode cur = ts_node_descendant_for_byte_range(root, byte, byte);
  while (!ts_node_is_null(cur)) {
    const char *t = ts_node_type(cur);
    if (t && wanted.count(t) > 0) {
      return make_node_plist(env, bs, cur, true);
    }
    cur = ts_node_parent(cur);
  }
  return elisp_nil(env);
}

emacs_value Fverilog_ts_children(emacs_env *env, ptrdiff_t nargs,
                                 emacs_value args[],
                                 void *data) EMACS_NOEXCEPT {
  BufferState &bs = get_buffer_state(env);
  if (!bs.tree) return elisp_nil(env);

  bool named_only = (nargs >= 2 && env->is_not_nil(env, args[1]));
  TSNode root = ts_tree_root_node(bs.tree);
  TSNode node = {};

  if (nargs >= 1 && env->is_not_nil(env, args[0]) &&
      env->eq(env, env->type_of(env, args[0]), env->intern(env, "cons"))) {
    emacs_value pget = env->intern(env, "plist-get");
    emacs_value s_args[2] = {args[0], elisp_intern(env, ":start")};
    emacs_value e_args[2] = {args[0], elisp_intern(env, ":end")};
    emacs_value sv = env->funcall(env, pget, 2, s_args);
    emacs_value ev = env->funcall(env, pget, 2, e_args);
    if (env->is_not_nil(env, sv) && env->is_not_nil(env, ev)) {
      uint32_t sb = bs.pos_to_byte(env->extract_integer(env, sv));
      uint32_t eb = bs.pos_to_byte(env->extract_integer(env, ev));
      node = ts_node_named_descendant_for_byte_range(root, sb, eb);
    }
  } else {
    intmax_t pos = (nargs >= 1 && env->is_not_nil(env, args[0]))
                       ? env->extract_integer(env, args[0])
                       : get_point(env);
    uint32_t byte = bs.pos_to_byte(pos);
    if (byte >= bs.text.size() && byte > 0) {
      byte = static_cast<uint32_t>(bs.text.size() - 1);
    }
    node = ts_node_named_descendant_for_byte_range(root, byte, byte);
    while (!ts_node_is_null(node) &&
           (named_only ? ts_node_named_child_count(node)
                       : ts_node_child_count(node)) == 0) {
      node = ts_node_parent(node);
    }
  }
  if (ts_node_is_null(node)) return elisp_nil(env);

  std::vector<emacs_value> children;
  uint32_t count = named_only ? ts_node_named_child_count(node)
                              : ts_node_child_count(node);
  children.reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    TSNode ch = named_only ? ts_node_named_child(node, i)
                           : ts_node_child(node, i);
    children.push_back(make_node_plist(env, bs, ch, false));
  }
  return elisp_list(env, children);
}

emacs_value Fverilog_ts_query(emacs_env *env, ptrdiff_t nargs,
                              emacs_value args[], void *data) EMACS_NOEXCEPT {
  if (nargs < 1 || !env->is_not_nil(env, args[0])) return elisp_nil(env);
  std::string query_str = extract_string(env, args[0]);
  if (query_str.empty()) return elisp_nil(env);

  BufferState &bs = get_buffer_state(env);
  if (!bs.tree) return elisp_nil(env);

  uint32_t err_offset = 0;
  TSQueryError err_type = TSQueryErrorNone;
  TSQuery *query = get_or_compile_query(query_str, &err_offset, &err_type);
  if (!query) {
    emacs_value err_sym = env->intern(env, "error");
    std::string msg = "Invalid tree-sitter query at offset " +
                      std::to_string(err_offset);
    emacs_value err_data = elisp_list(env, {elisp_string(env, msg)});
    env->non_local_exit_signal(env, err_sym, err_data);
    return elisp_nil(env);
  }

  TSQueryCursor *cursor = ts_query_cursor_new();
  if (nargs >= 3 && env->is_not_nil(env, args[1]) &&
      env->is_not_nil(env, args[2])) {
    uint32_t b_start = bs.pos_to_byte(env->extract_integer(env, args[1]));
    uint32_t b_end = bs.pos_to_byte(env->extract_integer(env, args[2]));
    ts_query_cursor_set_byte_range(cursor, b_start, b_end);
  }

  TSNode root = ts_tree_root_node(bs.tree);
  ts_query_cursor_exec(cursor, query, root);

  std::vector<emacs_value> results;
  TSQueryMatch match;
  uint32_t capture_index = 0;
  while (ts_query_cursor_next_capture(cursor, &match, &capture_index)) {
    const TSQueryCapture &cap = match.captures[capture_index];
    uint32_t name_len = 0;
    const char *cap_name =
        ts_query_capture_name_for_id(query, cap.index, &name_len);
    uint32_t sb = ts_node_start_byte(cap.node);
    uint32_t eb = ts_node_end_byte(cap.node);
    std::vector<emacs_value> item = {
        elisp_intern(env, ":capture"),
        elisp_string(env, std::string_view(cap_name, name_len)),
        elisp_intern(env, ":type"),
        elisp_string(env, ts_node_type(cap.node)),
        elisp_intern(env, ":start"),
        elisp_int(env, bs.byte_to_pos(sb)),
        elisp_intern(env, ":end"),
        elisp_int(env, bs.byte_to_pos(eb)),
        elisp_intern(env, ":text"),
        elisp_string(env, bs.slice(sb, eb))};
    results.push_back(elisp_list(env, item));
  }
  ts_query_cursor_delete(cursor);
  return elisp_list(env, results);
}

} // namespace
} // namespace verilog_ts

extern "C" int emacs_module_init(struct emacs_runtime *ert) EMACS_NOEXCEPT {
  if (static_cast<size_t>(ert->size) < sizeof(*ert)) {
    return 1;
  }
  emacs_env *env = ert->get_environment(ert);
  if (static_cast<size_t>(env->size) < sizeof(*env)) {
    return 2;
  }

  using namespace verilog_ts;

  bind_function(env, "verilog-ts-version", 0, 0, Fverilog_ts_version,
                "Return the verilog-ts-core module version string.");
  bind_function(env, "verilog-ts-parse-buffer", 0, 0, Fverilog_ts_parse_buffer,
                "Parse or update the current buffer's tree-sitter AST cache.\n"
                "Return t if clean, or `has-errors' if the AST contains ERROR nodes.");
  bind_function(env, "verilog-ts-cache-flush", 0, 1, Fverilog_ts_cache_flush,
                "Flush the tree-sitter AST cache for current buffer (or all if ALL is non-nil).");
  bind_function(env, "verilog-ts-root-sexp", 0, 0, Fverilog_ts_root_sexp,
                "Return the full tree-sitter S-expression string of current buffer.");
  bind_function(env, "verilog-ts-node-at", 0, 2, Fverilog_ts_node_at,
                "Return AST node plist at POS (default point).\n"
                "If NAMED-ONLY is non-nil, return the smallest named node.");
  bind_function(env, "verilog-ts-enclosing-node", 1, 2,
                Fverilog_ts_enclosing_node,
                "Return nearest enclosing AST node matching NODE-TYPES at POS (default point).");
  bind_function(env, "verilog-ts-children", 0, 2, Fverilog_ts_children,
                "Return child node plists of the named AST node at POS (default point).");
  bind_function(env, "verilog-ts-query", 1, 3, Fverilog_ts_query,
                "Execute tree-sitter S-expression QUERY-STR on current buffer (optional BEG END).");

  register_scan_functions(env);
  register_nav_functions(env);
  register_indent_functions(env);
  register_decls_functions(env);

  emacs_value feat = env->intern(env, "verilog-ts-core");
  env->funcall(env, env->intern(env, "provide"), 1, &feat);
  return 0;
}

#ifndef VERILOG_TS_COMMON_HPP
#define VERILOG_TS_COMMON_HPP

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <list>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <emacs-module.h>
#include <tree_sitter/api.h>

extern "C" {
const TSLanguage *tree_sitter_systemverilog(void);
}

namespace verilog_ts {

enum class SpanKind : uint8_t {
  LINE_COMMENT = 0,
  BLOCK_COMMENT = 1,
  STRING_LITERAL = 2,
  ATTRIBUTE = 3,
  PROTECTED_REGION = 4
};

struct CommentStringSpan {
  uint32_t start_byte;
  uint32_t end_byte;
  SpanKind kind;
};

struct DefunInfo {
  std::string type;       // e.g. "module", "interface", "package", "program", "class", "function", "task", "connectmodule", "primitive", "checker", "config"
  std::string name;       // identifier or macro name
  uint32_t start_byte;    // 0-based start byte of construct (or keyword)
  uint32_t kwd_byte;      // 0-based start byte of the keyword (module/function/task/etc.)
  uint32_t header_end_byte; // 0-based byte after header ';' or '('
  uint32_t end_byte;      // 0-based end byte after endmodule/endfunction/etc.
};

struct BlockInfo {
  std::string type;       // e.g. "begin", "fork", "case", "class", "function", "task", "generate", "covergroup", "property", "sequence", "clocking", "specify", "table", "`ifdef", etc.
  std::string name;       // optional block label or identifier
  uint32_t start_byte;    // 0-based byte of opening keyword
  uint32_t end_kwd_byte;  // 0-based byte of closing keyword (end, endcase, join*, etc.)
  uint32_t end_byte;      // 0-based byte after closing keyword (and optional : label)
};

struct SignalInfo {
  std::string name;
  std::optional<std::string> bits;      // packed dimension (nth 1)
  std::optional<std::string> comment;   // comment (nth 2)
  std::optional<std::string> memory;    // unpacked dimension (nth 3)
  std::optional<std::string> enum_name; // auto/synopsys enum (nth 4)
  std::optional<std::string> signed_str;// "signed" / "unsigned" (nth 5)
  std::optional<std::string> type_str;  // data/net/user type (nth 6)
  std::vector<std::string> multidim;    // extra packed dims (nth 7)
  std::optional<std::string> modport;   // interface modport (nth 8)
};

struct ModportInfo {
  std::string name;
  std::vector<std::string> clockings;
  std::vector<SignalInfo> outs;
  std::vector<SignalInfo> inouts;
  std::vector<SignalInfo> ins;
};

struct ModuleDecls {
  std::vector<SignalInfo> outs;
  std::vector<SignalInfo> inouts;
  std::vector<SignalInfo> ins;
  std::vector<SignalInfo> vars;
  std::vector<ModportInfo> modports;
  std::vector<SignalInfo> assigns;
  std::vector<SignalInfo> consts;
  std::vector<SignalInfo> gparams;
  std::vector<SignalInfo> interfaces;
};

struct InstInfo {
  std::string module_name;
  uint32_t module_start_byte = 0;
  uint32_t module_end_byte = 0;
  std::string inst_name;
  uint32_t inst_start_byte = 0;
  uint32_t inst_end_byte = 0;
  uint32_t paren_start_byte = 0;
  uint32_t paren_end_byte = 0;
  uint32_t start_byte = 0;
  uint32_t end_byte = 0;
  std::vector<std::pair<std::string, std::string>> params;
  std::vector<std::pair<std::string, std::string>> ports;
  bool valid = false;
};

struct AlwaysSignals {
  std::vector<std::string> out_delayed;
  std::vector<std::string> out_immediate;
  std::vector<std::string> temps;
  std::vector<std::string> inputs;
};

struct BufferState {
  intmax_t chars_modified_tick = -1;
  intmax_t point_min = 1;
  intmax_t point_max = 1;
  int tab_width = 8;
  std::string text;
  bool is_ascii = true;
  std::vector<uint32_t> multibyte_cont_bytes;
  TSTree *tree = nullptr;
  std::vector<CommentStringSpan> spans;
  std::vector<CommentStringSpan> attr_spans;
  std::vector<CommentStringSpan> protected_spans;
  std::vector<uint32_t> line_starts;

  BufferState() = default;
  ~BufferState() {
    if (tree) {
      ts_tree_delete(tree);
      tree = nullptr;
    }
  }
  BufferState(const BufferState &) = delete;
  BufferState &operator=(const BufferState &) = delete;

  intmax_t byte_to_pos(uint32_t byte) const {
    if (byte > text.size()) byte = static_cast<uint32_t>(text.size());
    if (is_ascii) {
      return point_min + static_cast<intmax_t>(byte);
    }
    auto it = std::lower_bound(multibyte_cont_bytes.begin(),
                               multibyte_cont_bytes.end(), byte);
    ptrdiff_t cont_before = it - multibyte_cont_bytes.begin();
    return point_min + static_cast<intmax_t>(byte) - cont_before;
  }

  uint32_t pos_to_byte(intmax_t pos) const {
    if (pos <= point_min) return 0;
    if (pos >= point_max) return static_cast<uint32_t>(text.size());
    if (is_ascii) {
      return static_cast<uint32_t>(pos - point_min);
    }
    // Binary search byte offset such that byte_to_pos(byte) == pos
    uint32_t lo = static_cast<uint32_t>(pos - point_min);
    uint32_t hi = std::min<uint32_t>(
        static_cast<uint32_t>(text.size()),
        lo + static_cast<uint32_t>(multibyte_cont_bytes.size()));
    while (lo < hi) {
      uint32_t mid = lo + (hi - lo) / 2;
      if (byte_to_pos(mid) < pos) {
        lo = mid + 1;
      } else {
        hi = mid;
      }
    }
    return lo;
  }

  uint32_t byte_to_line(uint32_t byte) const {
    if (line_starts.empty()) return 1;
    auto it = std::upper_bound(line_starts.begin(), line_starts.end(), byte);
    return static_cast<uint32_t>(it - line_starts.begin());
  }

  uint32_t line_start_byte(uint32_t byte) const {
    if (line_starts.empty()) return 0;
    auto it = std::upper_bound(line_starts.begin(), line_starts.end(), byte);
    if (it == line_starts.begin()) return 0;
    return *(--it);
  }

  uint32_t line_end_byte(uint32_t byte) const {
    if (byte >= text.size()) return static_cast<uint32_t>(text.size());
    if (line_starts.empty()) return static_cast<uint32_t>(text.size());
    auto it = std::upper_bound(line_starts.begin(), line_starts.end(), byte);
    if (it == line_starts.end()) return static_cast<uint32_t>(text.size());
    uint32_t next_start = *it;
    if (next_start > 0 && text[next_start - 1] == '\n') {
      return next_start - 1;
    }
    return next_start;
  }

  uint32_t byte_to_column(uint32_t byte) const {
    uint32_t ls = line_start_byte(byte);
    uint32_t limit = std::min<uint32_t>(byte, static_cast<uint32_t>(text.size()));
    uint32_t col = 0;
    uint32_t tw = (tab_width > 0) ? static_cast<uint32_t>(tab_width) : 8;
    for (uint32_t i = ls; i < limit; ++i) {
      unsigned char c = static_cast<unsigned char>(text[i]);
      if (c == '\t') {
        col = (col / tw + 1) * tw;
      } else if ((c & 0xC0) != 0x80) {
        col += 1;
      }
    }
    return col;
  }

  std::string_view slice(uint32_t start, uint32_t end) const {
    if (start >= text.size() || start >= end) return {};
    end = std::min<uint32_t>(end, static_cast<uint32_t>(text.size()));
    return std::string_view(text.data() + start, end - start);
  }

  std::string_view node_text(TSNode node) const {
    if (ts_node_is_null(node)) return {};
    return slice(ts_node_start_byte(node), ts_node_end_byte(node));
  }

  const CommentStringSpan *find_span_at_byte(uint32_t byte,
                                             bool v_cmts_semantics = true) const {
    auto it = std::upper_bound(
        spans.begin(), spans.end(), byte,
        [](uint32_t b, const CommentStringSpan &s) { return b < s.start_byte; });
    if (it == spans.begin()) return nullptr;
    --it;
    uint32_t min_start = v_cmts_semantics ? (it->start_byte + 1) : it->start_byte;
    if (byte >= min_start && byte < it->end_byte) {
      return &(*it);
    }
    return nullptr;
  }

  bool is_inside_comment_or_string_byte(uint32_t byte,
                                        bool v_cmts_semantics = true) const {
    return find_span_at_byte(byte, v_cmts_semantics) != nullptr;
  }

  bool is_inside_attribute_byte(uint32_t byte) const {
    auto it = std::upper_bound(
        attr_spans.begin(), attr_spans.end(), byte,
        [](uint32_t b, const CommentStringSpan &s) { return b < s.start_byte; });
    if (it == attr_spans.begin()) return false;
    --it;
    return byte >= it->start_byte + 2 && byte + 2 < it->end_byte;
  }

  bool is_inside_protected_byte(uint32_t byte) const {
    auto it = std::upper_bound(
        protected_spans.begin(), protected_spans.end(), byte,
        [](uint32_t b, const CommentStringSpan &s) { return b < s.start_byte; });
    if (it == protected_spans.begin()) return false;
    --it;
    return byte >= it->start_byte && byte < it->end_byte;
  }
};

// Emacs environment helpers (implemented in verilog_ts_core.cpp)
emacs_value elisp_nil(emacs_env *env);
emacs_value elisp_t(emacs_env *env);
emacs_value elisp_intern(emacs_env *env, const char *name);
emacs_value elisp_string(emacs_env *env, std::string_view sv);
emacs_value elisp_int(emacs_env *env, intmax_t val);
emacs_value elisp_cons(emacs_env *env, emacs_value car, emacs_value cdr);
emacs_value elisp_list(emacs_env *env, const std::vector<emacs_value> &elems);
emacs_value elisp_vector(emacs_env *env, const std::vector<emacs_value> &elems);
std::string extract_string(emacs_env *env, emacs_value val);
intmax_t get_point(emacs_env *env);
intmax_t get_point_min(emacs_env *env);
intmax_t get_point_max(emacs_env *env);
BufferState &get_buffer_state(emacs_env *env, bool need_tree = true);
TSQuery *get_or_compile_query(const std::string &query_str,
                              uint32_t *err_offset,
                              TSQueryError *err_type);
bool is_verilog_keyword(std::string_view word);
emacs_value make_node_plist(emacs_env *env, const BufferState &bs, TSNode node,
                            bool include_sexp = false);

// Scanner & syntactic whitespace helpers (implemented in verilog_ts_scan.cpp)
void build_buffer_spans(BufferState &bs);
uint32_t forward_syntactic_ws_bytes(const BufferState &bs, uint32_t byte,
                                    uint32_t bound_byte,
                                    bool skip_directives = false);
uint32_t backward_syntactic_ws_bytes(const BufferState &bs, uint32_t byte,
                                     uint32_t bound_byte,
                                     bool skip_directives = false);
void register_scan_functions(emacs_env *env);

// Navigation & context functions (implemented in verilog_ts_nav.cpp)
std::optional<DefunInfo> find_enclosing_defun(const BufferState &bs,
                                              uint32_t byte,
                                              bool module_only = false);
std::vector<DefunInfo> find_all_defuns(const BufferState &bs,
                                       bool modules_only = false);
std::optional<BlockInfo> find_block_at_byte(const BufferState &bs,
                                            uint32_t byte);
std::optional<uint32_t> leap_to_head_byte(const BufferState &bs, uint32_t byte);
std::optional<uint32_t> forward_sexp_byte(const BufferState &bs, uint32_t byte);
std::optional<uint32_t> backward_sexp_byte(const BufferState &bs, uint32_t byte);
std::pair<uint32_t, uint32_t> find_statement_bounds_bytes(const BufferState &bs,
                                                          uint32_t byte);
bool check_in_context(const BufferState &bs, std::string_view context,
                      uint32_t byte, intmax_t *nested_col_out = nullptr);
void register_nav_functions(emacs_env *env);

// Indentation, alignment & endcomment functions (implemented in verilog_ts_indent.cpp)
void register_indent_functions(emacs_env *env);

// Declarations, instantiations, always blocks, defines, font-lock & imenu (implemented in verilog_ts_decls.cpp)
ModuleDecls extract_module_decls(const BufferState &bs, uint32_t start_byte,
                                 uint32_t end_byte,
                                 const std::string &typedef_regexp,
                                 bool case_fold);
InstInfo extract_inst_at_byte(const BufferState &bs, uint32_t byte);
std::vector<InstInfo> extract_all_insts(const BufferState &bs,
                                        uint32_t start_byte,
                                        uint32_t end_byte);
AlwaysSignals extract_always_signals(const BufferState &bs, uint32_t start_byte,
                                     uint32_t end_byte);
void register_decls_functions(emacs_env *env);

// Helper to bind an Emacs function with docstring
void bind_function(
    emacs_env *env, const char *name, ptrdiff_t min_arity, ptrdiff_t max_arity,
    emacs_value (*func)(emacs_env *env, ptrdiff_t nargs, emacs_value args[],
                        void *data) EMACS_NOEXCEPT,
    const char *doc);

} // namespace verilog_ts

#endif // VERILOG_TS_COMMON_HPP

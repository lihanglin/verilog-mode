# `verilog-mode-ng` — Tree-Sitter Powered SystemVerilog Mode for GNU Emacs

`verilog-mode-ng` is a drop-in, next-generation replacement for GNU Emacs's `verilog-mode` powered by a native C++17 Tree-Sitter dynamic module (`verilog-ts-core.so`) and the IEEE 1800-2023 SystemVerilog grammar (`tree-sitter-systemverilog`).

It preserves **100% backward compatibility** with all original `verilog-mode` `/*AUTO...*/` expansions, indentation rules, and user customizations, while adding Tree-Sitter AST accuracy and new multi-column tabular alignment features for SystemVerilog.

---

## 1. Quick Start

```bash
# 1. Build static Tree-Sitter libraries:
make -C tree-sitter libtree-sitter.a
make -C tree-sitter-systemverilog libtree-sitter-systemverilog.a

# 2. Build verilog-ts-core.so and byte-compile verilog-mode.elc:
make -C verilog-mode compile
```

For full installation instructions (user `~/.emacs.d`, build-tree, or system-wide replacement), see [INSTALL.md](INSTALL.md).

---

## 2. How to Configure `verilog-mode`

All configurable settings below can be set in any of three ways:

1. **Global Configuration (`~/.emacs` or `~/.emacs.d/init.el`)**:
   ```elisp
   (setq verilog-indent-level             3
         verilog-indent-level-module      3
         verilog-indent-level-declaration 3
         verilog-indent-level-behavioral  3
         verilog-auto-newline             nil
         verilog-auto-lineup              'declarations
         verilog-auto-lineup-on-semi      t
         verilog-align-decl-dimension     t
         verilog-align-decl-sub-dimension t)
   ```
2. **Interactive Emacs Customization UI**:
   ```
   M-x customize-group RET verilog-mode RET
   ```
   Sub-groups:
   - `verilog-mode-indent` — Indentation, `<TAB>` / `;` alignment, and syntax highlighting
   - `verilog-mode-auto` — `/*AUTO...*/` expansion behavior, library search paths, and hooks
   - `verilog-mode-actions` — External tools (linter, compiler, simulator, preprocessor) and save/edit actions
3. **Per-File Local Variables (at the bottom of any `.v` / `.sv` / `.svh` file)**:
   ```systemverilog
   // Local Variables:
   // verilog-indent-level: 4
   // verilog-auto-newline: nil
   // verilog-auto-lineup-on-semi: t
   // verilog-align-decl-dimension: t
   // verilog-align-decl-sub-dimension: t
   // verilog-library-flags: ("-y ../rtl +libext+.sv+.v")
   // End:
   ```

---

## 3. Complete Reference of Configurable Settings

`verilog-mode` provides **92 `defcustom` variables** (83 option variables + 9 hook variables), **4 `defface` faces**, and **8 user-configurable `defvar` variables**.

### 3.1 Indentation & Block Structure (`verilog-mode-indent`)

| Variable | Default | Type | Description |
| :--- | :--- | :--- | :--- |
| `verilog-indent-level` | `3` | `integer` | Indentation of Verilog statements with respect to their containing block (`begin`..`end`, `if`, `for`, etc.). |
| `verilog-indent-level-module` | `3` | `integer` | Indentation of module-level statements (e.g., `always`, `initial`, `assign`). Set to `0` to start module items at column 0. |
| `verilog-indent-level-declaration` | `3` | `integer` | Indentation of declarations (`input`, `output`, `inout`, `reg`, `wire`, `logic`, `parameter`, `localparam`) relative to the containing block. |
| `verilog-indent-level-behavioral` | `3` | `integer` | Absolute indentation of the first `begin` in a `task` or `function` block. |
| `verilog-indent-level-directive` | `1` | `integer` | Indentation added to each nested level of `` `ifdef `` / `` `ifndef `` / `` `else `` / `` `elsif `` preprocessor directives. |
| `verilog-case-indent` | `2` | `integer` | Indentation for `case` items inside `case`, `casex`, `casez`, and `randcase` statements. |
| `verilog-cexp-indent` | `2` | `integer` | Indentation of Verilog expressions and statements split across multiple lines. |
| `verilog-indent-lists` | `t` | `boolean` | How to indent items inside parenthesized lists (`t` aligns continuations under the first item after `(`; `nil` indents relative to the containing block). |
| `verilog-indent-begin-after-if` | `t` | `boolean` | Non-nil indents `begin` statements following `if`, `else`, `while`, `for`, `always`, etc.; `nil` lines `begin` up with the enclosing keyword. |
| `verilog-indent-class-inside-pkg` | `t` | `boolean` | Non-nil indents `class` declarations inside `package` blocks; `nil` indents `class` at column 0. |
| `verilog-indent-declaration-macros` | `nil` | `boolean` | How to treat macro expansions inside declarations (`nil` aligns macro after the `[msb:lsb]` width column; `t` treats the macro as part of the type column). |
| `verilog-indent-ignore-multiline-defines` | `t` | `boolean` | Non-nil ignores indentation on continuation lines inside a multiline `` `define `` macro. |
| `verilog-indent-ignore-regexp` | `nil` | `regexp` / `nil` | Regexp matching lines that should be skipped during indentation. |
| `verilog-align-ifelse` | `nil` | `boolean` | Non-nil aligns `else` directly under its matching `if`; `nil` aligns `else` with the first character of the line holding the matching `if` (e.g., `end else if`). |
| `verilog-minimum-comment-distance` | `10` | `integer` | Minimum distance (in lines) between `begin` and `end` required before `verilog-auto-endcomments` inserts a trailing comment on `end` (`0` labels every `end`). |

---

### 3.2 `<TAB>`, Semicolon (`;`), and Tabular Alignment (`verilog-mode-indent` / `verilog-mode-actions`)

| Variable | Default | Type | Description |
| :--- | :--- | :--- | :--- |
| `verilog-tab-always-indent` | `t` | `boolean` | Non-nil means pressing `<TAB>` always re-indents the current line regardless of point position; `nil` re-indents only when point is at the beginning of the line. |
| `verilog-tab-to-comment` | `nil` | `boolean` | Non-nil means pressing `<TAB>` moves point to the right-hand comment column in preparation for a comment. |
| `verilog-auto-newline` | `t` | `boolean` | Non-nil automatically inserts a newline after typing a semicolon (`;`). Set `(setq verilog-auto-newline nil)` to disable automatic newlines on `;`. |
| `verilog-auto-indent-on-newline` | `t` | `boolean` | Non-nil automatically indents the line when pressing `<RET>` (`electric-verilog-terminate-line`). |
| `verilog-auto-lineup` | `'declarations` | `symbol` / `function` | Statements to line up across multiple lines: `'declarations` (declarations only), `'assignments` (assignments only), `'all` (both declarations and assignments), or `nil` (none). |
| `verilog-auto-lineup-on-semi` *(New)* | `t` | `boolean` | Controls whether typing `;` (`electric-verilog-semi`) runs the same multi-line declaration/block alignment as pressing `<TAB>` (`electric-verilog-tab`). When `t`, typing `;` and pressing `<TAB>` share the exact same formatting flow. When `nil`, typing `;` only indents the current line and pressing `<TAB>` triggers block alignment. |
| `verilog-align-decl-dimension` *(New)* | `nil` | `boolean` | Master switch for Tree-Sitter multi-column alignment across **ANSI port lists**, **packed struct/union members**, **module instantiation port connections** (`.port_name (signal)`), and **signal declarations**. When `t` and `verilog-align-decl-sub-dimension` is `nil`, consecutive packed dimensions are normalized with no spaces between brackets (`[dim0][dim1]`) and aligned as a single column after `type`. |
| `verilog-align-decl-sub-dimension` *(New)* | `nil` | `boolean` | When both `verilog-align-decl-dimension` and `verilog-align-decl-sub-dimension` are `t` (ignored when `verilog-align-decl-dimension` is `nil`), aligns each packed dimension (`[dim0] [dim1] ...`) into its own sub-column **right-to-left** (`col_idx = (max_k - m) + j`), left-aligning `[` inside each sub-column with 1 space between columns. |
| `verilog-align-decl-expr-comments` | `t` | `boolean` | Non-nil aligns trailing end-of-line comments across declarations and expressions. |
| `verilog-align-comment-distance` | `1` | `integer` | Minimum distance (in spaces) between the longest declaration/expression and aligned trailing comments when `verilog-align-decl-expr-comments` is non-nil. |
| `verilog-align-assign-expr` | `nil` | `boolean` | Non-nil aligns the expressions of continuous `assign` statements across consecutive lines. |
| `verilog-align-typedef-regexp` | `nil` | `regexp` / `nil` | Regexp matching user-defined typedef names for legacy declaration alignment (e.g., `"_t$"`). |
| `verilog-align-typedef-words` | `nil` | `list of strings` | List of literal type names matching user-defined typedefs for declaration alignment. |
| `verilog-auto-inst-column` | `40` | `integer` | Target column number for the connected signal `(net)` part of `/*AUTOINST*/` generated pins. |

#### Examples of New Alignment Settings (`verilog-align-decl-dimension` & `verilog-align-decl-sub-dimension`)

- **With `(setq verilog-align-decl-dimension t verilog-align-decl-sub-dimension nil)`**:
  ```systemverilog
  module my_mod (
      input  logic               clk,
      input  logic               rst_n,
      input  pkg_t               req_in,
      output logic [WIDTH-1:0]   data_out,
      output logic [3:0][7:0]    byte_en
  );
     typedef struct packed {
        logic               valid;
        logic [3:0]         id;
        my_pkg::custom_t    payload;
        logic [1:0][15:0]   matrix;
     } packet_t;

     logic                   a;
     logic [7:0]             b;
     logic [3:0][7:0]        c;
     logic [1:0][15:0][7:0]  d;

     sub_mod u_sub (
         .clk       (clk),
         .rst_n     (rst_n),
         .req_valid (valid && ready),
         .data_o    (dout[31:0])
     );
  endmodule
  ```

- **With `(setq verilog-align-decl-dimension t verilog-align-decl-sub-dimension t)`** (right-to-left sub-dimension alignment):
  ```systemverilog
  logic                    a;
  logic              [7:0] b;
  logic        [3:0] [7:0] c;
  logic [1:0] [15:0] [7:0] d;
  ```

---

### 3.3 Syntax Highlighting, Font-Lock & Faces (`verilog-mode-indent` / `verilog-mode-actions` / `font-lock-highlighting-faces`)

| Variable / Face | Default | Type | Description |
| :--- | :--- | :--- | :--- |
| `verilog-fontify-variables` | `t` | `boolean` | Non-nil fontifies declared variable names in declarations using `font-lock-variable-name-face`. |
| `verilog-highlight-translate-off` | `nil` | `boolean` | Non-nil background-highlights regions between `// synopsys translate_off` and `// synopsys translate_on` using `verilog-font-lock-translate-off-face`. |
| `verilog-highlight-grouping-keywords` | `nil` | `boolean` | Non-nil highlights structural grouping keywords (`begin`..`end`) using `verilog-font-lock-grouping-keywords-face` instead of `font-lock-type-face`. |
| `verilog-highlight-includes` | `t` | `boolean` | Non-nil highlights `` `include `` filenames so clicking or pressing `RET` (`verilog-load-file-at-point`) opens the included file. |
| `verilog-highlight-modules` | `nil` | `boolean` | Non-nil highlights instantiated module names for mouse/`RET` navigation to the submodule definition. |
| `verilog-highlight-max-lookahead` | `10000` | `integer` | Maximum character lookahead in a declaration statement for variable fontification (`0` = unlimited). |
| `verilog-highlight-p1800-keywords` | `nil` | `boolean` | Obsolete (all buffers are now highlighted with full IEEE 1800 SystemVerilog support). |
| `verilog-font-lock-translate-off-face` | *(face)* | `face` | Face used to background-highlight `translate_off` regions when `verilog-highlight-translate-off` is non-nil. |
| `verilog-font-lock-grouping-keywords-face` | *(face)* | `face` | Face used to highlight grouping keywords (`begin`..`end`) when `verilog-highlight-grouping-keywords` is non-nil. |
| `verilog-font-lock-ams-face` | *(face)* | `face` | Face used to highlight Verilog-AMS (Analog/Mixed-Signal) keywords. |
| `verilog-font-lock-p1800-face` | *(face)* | `face` | Obsolete face formerly used for IEEE 1800 keywords. |

---

### 3.4 Library & Submodule Search Paths (`verilog-mode-auto` / `verilog-mode`)

| Variable | Default | Type | Description |
| :--- | :--- | :--- | :--- |
| `verilog-library-flags` | `'("")` | `list of strings` | List of simulator-style command-line flags (`-y dir`, `+incdir+dir`, `-Idir`, `+libext+.v+.sv`, `-v file`, `-f file`, `-F file`) used to locate submodules and includes for `/*AUTOINST*/` and `verilog-auto`. |
| `verilog-library-directories` | `'(".")` | `list of dirs` | List of directories searched when resolving submodule definitions for `/*AUTOINST*/`. |
| `verilog-library-files` | `'()` | `list of files` | List of specific library/technology files searched for module definitions. |
| `verilog-library-extensions` | `'(".v" ".va" ".sv")` | `list of strings` | List of file extensions tried when searching `verilog-library-directories` for a module name. |
| `verilog-case-fold` | `t` | `boolean` | Non-nil means `verilog-mode` searches ignore case (`nil` is recommended for strict SystemVerilog). |
| `verilog-typedef-regexp` | `nil` | `regexp` / `nil` | Regexp matching user-defined Verilog-2001 / SystemVerilog typedef names (e.g., `"_t$"`) during `/*AUTO...*/` expansion. |

---

### 3.5 `/*AUTO...*/` Expansion & Buffer Actions (`verilog-mode-auto` / `verilog-mode-actions`)

| Variable | Default | Type | Description |
| :--- | :--- | :--- | :--- |
| `verilog-auto-endcomments` | `t` | `boolean` | Non-nil inserts descriptive comments (e.g., `// case: ...`, `// block: ...`) after `end` keywords when the block spans at least `verilog-minimum-comment-distance` lines. |
| `verilog-auto-wire-type` | `nil` | `string` / `nil` | Data type used by `/*AUTOWIRE*/` (e.g., `"logic"` for SystemVerilog, or `"wire"`). |
| `verilog-auto-wire-comment` | `t` | `boolean` | Non-nil inserts `// To/From <inst> of <module>` comments on `/*AUTOWIRE*/` declarations. |
| `verilog-auto-declare-nettype` | `nil` | `string` / `nil` | Data type used with `/*AUTOINPUT*/` / `/*AUTOOUTPUT*/` when `` `default_nettype none `` is active (e.g., `"wire"`). |
| `verilog-auto-delete-trailing-whitespace` | `nil` | `boolean` | Non-nil strips trailing whitespace from the buffer during `M-x verilog-auto`. |
| `verilog-auto-ignore-concat` | `nil` | `boolean` | Non-nil excludes signals referenced inside `{...}` concatenations from `/*AUTOWIRE*/`, `/*AUTOOUTPUT*/`, etc. |
| `verilog-auto-read-includes` | `nil` | `boolean` | Non-nil automatically runs `verilog-read-defines` and `verilog-read-includes` before expanding AUTOs. |
| `verilog-auto-save-policy` | `nil` | `symbol` | Action when saving a buffer containing AUTOs: `nil` (do nothing), `'ask` (prompt user), `'detect` (update when needed), or `'force` (always run `verilog-auto` on save). |
| `verilog-auto-star-expand` | `t` | `boolean` | Non-nil expands SystemVerilog `.*` implicit port connections during `M-x verilog-auto`. |
| `verilog-auto-star-save` | `nil` | `boolean` | Non-nil keeps expanded `.*` port connections in the file when saving to disk (`nil` collapses them back to `.*` on save). |
| `verilog-active-low-regexp` | `nil` | `regexp` / `nil` | Regexp matching active-low signal names (e.g., `"_n$"`), used by `/*AUTORESET*/` and `/*AUTOTIEOFF*/` to reset/tie off signals to `1` instead of `0`. |
| `verilog-auto-sense-include-inputs` | `nil` | `boolean` | Non-nil includes all read signals in `/*AUTOSENSE*/` even if they are also written in the same `always` block. |
| `verilog-auto-sense-defines-constant` | `nil` | `boolean` | Non-nil assumes all `` `define `` macro references inside `always` blocks represent constants and excludes them from `/*AUTOSENSE*/` sensitivity lists. |
| `verilog-auto-simplify-expressions` | `t` | `boolean` | Non-nil simplifies constant arithmetic expressions when computing bus bit ranges in AUTOs. |
| `verilog-auto-reset-blocking-in-non` | `t` | `boolean` | Non-nil resets signals assigned with blocking `=` inside `/*AUTORESET*/` blocks that also use nonblocking `<=`. |
| `verilog-auto-reset-widths` | `t` | `symbol` / `boolean` | Controls width formatting of zero literals in `/*AUTORESET*/`: `t` (sized literal like `32'h0`), `nil` (unsized `0`), or `'unbased` (SystemVerilog `'0`). |
| `verilog-assignment-delay` | `""` | `string` | Delay text inserted in `/*AUTORESET*/` nonblocking assignments (e.g., `"#1 "`). |
| `verilog-auto-arg-format` | `'packed` | `symbol` | Formatting for `/*AUTOARG*/` port lists: `'packed` (multiple ports per line up to `fill-column`) or `'single` (one port per line). |
| `verilog-auto-arg-sort` | `nil` | `boolean` | Non-nil sorts `/*AUTOARG*/` port names alphabetically instead of preserving declaration order. |
| `verilog-auto-inst-dot-name` | `nil` | `boolean` | Non-nil uses SystemVerilog `.port` syntax instead of `.port(port)` in `/*AUTOINST*/` when the net and port names match. |
| `verilog-auto-inst-param-value` | `nil` | `boolean` | Non-nil substitutes `#(...)` instance parameter values into `/*AUTOINST*/` port widths and types instead of keeping symbolic parameter names. |
| `verilog-auto-inst-param-value-type` | `t` | `boolean` | Non-nil expands `parameter type` overrides in `/*AUTOINST*/` when `verilog-auto-inst-param-value` is enabled. |
| `verilog-auto-inst-sort` | `nil` | `boolean` | Non-nil sorts `/*AUTOINST*/` and `/*AUTOINSTPARAM*/` pins alphabetically within input/inout/output groups. |
| `verilog-auto-inst-vector` | `t` | `symbol` / `boolean` | Controls bus subscripts on `/*AUTOINST*/` pins: `t` (always include `[msb:lsb]`), `nil` (omit when matching full bus width), or `'unsigned`. |
| `verilog-auto-inst-template-numbers` | `nil` | `symbol` / `boolean` | Annotates `/*AUTOINST*/` pins matched by `AUTO_TEMPLATE`: `nil` (no annotation), `t` (template line number), or `'lhs` (matched rule pattern). |
| `verilog-auto-inst-template-required` | `nil` | `boolean` | Non-nil omits any `/*AUTOINST*/` port that does not match an explicit `AUTO_TEMPLATE` rule. |
| `verilog-auto-inst-interfaced-ports` | `nil` | `boolean` | Non-nil includes SystemVerilog interface ports in `/*AUTOINST*/` expansions. |
| `verilog-auto-input-ignore-regexp` | `nil` | `regexp` / `nil` | Regexp of signal names to exclude from `/*AUTOINPUT*/`. |
| `verilog-auto-reg-input-assigned-ignore-regexp` | `nil` | `regexp` / `nil` | Regexp of signal names to exclude from `/*AUTOINPUT*/` / `/*AUTOREGINPUT*/`. |
| `verilog-auto-inout-ignore-regexp` | `nil` | `regexp` / `nil` | Regexp of signal names to exclude from `/*AUTOINOUT*/`. |
| `verilog-auto-output-ignore-regexp` | `nil` | `regexp` / `nil` | Regexp of signal names to exclude from `/*AUTOOUTPUT*/`. |
| `verilog-auto-tieoff-ignore-regexp` | `nil` | `regexp` / `nil` | Regexp of signal names to exclude from `/*AUTOTIEOFF*/`. |
| `verilog-auto-unused-ignore-regexp` | `nil` | `regexp` / `nil` | Regexp of signal names to exclude from `/*AUTOUNUSED*/`. |
| `verilog-auto-tieoff-declaration` | `"wire"` | `string` | Declaration/statement style for `/*AUTOTIEOFF*/` (`"wire"`, `"assign"`, or a data type like `"logic"`). |
| `verilog-auto-template-warn-unused` | `nil` | `boolean` | Non-nil warns when an `AUTO_TEMPLATE` rule is not matched by any instantiation. |
| `verilog-warn-fatal` | `nil` | `boolean` | Non-nil turns `verilog-warn-error` warnings during AUTO expansion into fatal Emacs errors. |

---

### 3.6 External Compilation, Linting & Simulation Tools (`verilog-mode-actions`)

| Variable | Default | Type | Description |
| :--- | :--- | :--- | :--- |
| `verilog-linter` | `"echo 'No verilog-linter set...'"` | `string` | Shell command invoked by `M-x compile` when `verilog-tool` is `'verilog-linter`. Supports `__FLAGS__`, `__FILE__`, and `%s` placeholders. |
| `verilog-compiler` | `"echo 'No verilog-compiler set...'"` | `string` | Shell command invoked by `M-x compile` when `verilog-tool` is `'verilog-compiler`. |
| `verilog-simulator` | `"echo 'No verilog-simulator set...'"` | `string` | Shell command invoked by `M-x compile` when `verilog-tool` is `'verilog-simulator`. |
| `verilog-coverage` | `"echo 'No verilog-coverage set...'"` | `string` | Shell command invoked by `M-x compile` when `verilog-tool` is `'verilog-coverage`. |
| `verilog-preprocessor` | `"verilator -E __FLAGS__ __FILE__"` | `string` | Shell command invoked by `M-x verilog-preprocess` (or `M-x compile` when `verilog-tool` is `'verilog-preprocessor`). |

---

### 3.7 Hooks (`verilog-mode` / `verilog-mode-auto`)

| Hook Variable | Default | Type | Description |
| :--- | :--- | :--- | :--- |
| `verilog-mode-hook` | `(list #'verilog-set-compile-command)` | `hook` | Hook run when entering `verilog-mode`. |
| `verilog-before-auto-hook` | `nil` | `hook` | Hook run before `M-x verilog-auto` updates AUTOs. |
| `verilog-auto-hook` | `nil` | `hook` | Hook run after `M-x verilog-auto` updates AUTOs. |
| `verilog-before-delete-auto-hook` | `nil` | `hook` | Hook run before `M-x verilog-delete-auto` deletes AUTOs. |
| `verilog-delete-auto-hook` | `nil` | `hook` | Hook run after `M-x verilog-delete-auto` deletes AUTOs. |
| `verilog-before-getopt-flags-hook` | `nil` | `hook` | Hook run before `verilog-getopt-flags` computes library flags. |
| `verilog-getopt-flags-hook` | `nil` | `hook` | Hook run after `verilog-getopt-flags` computes library flags. |
| `verilog-before-save-font-hook` | `nil` | `hook` | Hook run before temporarily removing fontification during batch/AUTO operations. |
| `verilog-after-save-font-hook` | `nil` | `hook` | Hook run after restoring fontification following batch/AUTO operations. |

---

### 3.8 Additional User-Configurable `defvar` Variables

In addition to the `defcustom` variables above, the following `defvar` variables can be configured with `(setq ...)` in `~/.emacs`:

| Variable | Default | Description |
| :--- | :--- | :--- |
| `verilog-tool` | `'verilog-linter` | Active tool symbol used to build `compile-command`: `nil`, `'verilog-linter`, `'verilog-compiler`, `'verilog-coverage`, `'verilog-preprocessor`, or `'verilog-simulator`. |
| `verilog-company` | `nil` | Default company name inserted by `M-x verilog-header` / `M-x verilog-sk-header`. |
| `verilog-project` | `nil` | Default project name inserted by `M-x verilog-header` / `M-x verilog-sk-header`. |
| `verilog-date-scientific-format` | `nil` | Non-nil formats dates in file headers as `YYYY/MM/DD`; `nil` uses `DD.MM.YYYY`. |
| `verilog-diff-function` | `#'verilog-diff-report` | Function called when `M-x verilog-diff-auto` detects differences in AUTO expansions. |
| `verilog-diff-ignore-regexp` | `nil` | Optional regexp of differences for `M-x verilog-diff-auto` to ignore. |
| `verilog-cache-enabled` | `t` | Non-nil enables caching of parsed module/signal declarations and directory lookups during `verilog-auto`. |
| `verilog-debug` | `nil` | Non-nil enables verbose debug messages for `verilog-mode` internals. |

---

## 4. Running the Test Suites

```bash
# 1. Tree-Sitter & Phase 3 ERT unit tests:
make -C verilog-mode test_ts

# 2. Batch tests (original batch tests + 50 new Phase 3 feature tests in tests_new/):
make -C verilog-mode test_batch

# 3. Full 479-file original golden test suite (5 parallel threads):
make -C verilog-mode -j5 test_emacs
```

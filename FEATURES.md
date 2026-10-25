# The configuration for verilog-mode

```elisp
;; 4. All standard `verilog-mode' customization variables remain 100% supported:
   (setq verilog-indent-level             3
         verilog-indent-level-module      3
         verilog-indent-level-declaration 3
         verilog-indent-level-behavioral  3
         verilog-indent-level-directive   1
         verilog-case-indent              2
         verilog-cexp-indent              2
         verilog-auto-newline             t
         verilog-auto-indent-on-newline   t
         verilog-tab-always-indent        t
         verilog-auto-endcomments         t
         verilog-minimum-comment-distance 40
         verilog-indent-begin-after-if    t
         verilog-auto-lineup              'declarations)
```

New features settings

```elisp
(setq verilog-align-decl-dimension nil)
(setq verilog-align-decl-sub-dimension nil)
(setq verilog-auto-newline nil)
(setq verilog-auto-lineup-on-semi nil)
(setq verilog-highlight-uvm nil)
```

## 5. Complete Reference of Configurable Settings

All 93 `defcustom` variables (84 option variables + 9 hook variables) and 5 `defface` faces from `verilog-mode` are preserved and can be configured via `(setq ...)` in `~/.emacs` / `~/.emacs.d/init.el`, interactively via `M-x customize-group RET verilog-mode`, or per-file via `// Local Variables:` at the bottom of a Verilog/SystemVerilog file.

### 5.1 Indentation & `<TAB>` Alignment (`verilog-mode-indent`)

| Variable | Default | Description |
| :--- | :--- | :--- |
| `verilog-indent-level` | `3` | Indentation of Verilog statements with respect to containing block. |
| `verilog-indent-level-module` | `3` | Indentation of module-level statements (e.g., `always`, `initial`). Set to `0` to align at column 0. |
| `verilog-indent-level-declaration` | `3` | Indentation of declarations (`input`, `output`, `reg`, `wire`, `logic`) with respect to containing block. |
| `verilog-indent-level-behavioral` | `3` | Absolute indentation of first `begin` in a `task` or `function` block. |
| `verilog-indent-level-directive` | `1` | Indentation added to each level of `` `ifdef `` / `` `ifndef `` directives. |
| `verilog-case-indent` | `2` | Indentation for `case` items inside `case` / `casex` / `casez` / `randcase` statements. |
| `verilog-cexp-indent` | `2` | Indentation of Verilog statements split across multiple lines. |
| `verilog-indent-lists` | `t` | How to indent items inside parenthesized lists (`t` aligns under opening item; `nil` indents relative to block). |
| `verilog-indent-begin-after-if` | `t` | Non-nil indents `begin` statements following `if`, `else`, `while`, etc.; `nil` lines them up. |
| `verilog-indent-class-inside-pkg` | `t` | Non-nil indents `class` declarations inside `package` blocks; `nil` uses zero indentation. |
| `verilog-indent-declaration-macros` | `nil` | How to align macro expansions inside declarations (`nil` aligns macro after width column; `t` treats macro as type). |
| `verilog-indent-ignore-multiline-defines` | `t` | Non-nil ignores indentation on lines that are part of a multiline `` `define ``. |
| `verilog-indent-ignore-regexp` | `nil` | Regexp matching lines that should be ignored during indentation. |
| `verilog-tab-always-indent` | `t` | Non-nil means `<TAB>` always re-indents the current line regardless of point position. |
| `verilog-tab-to-comment` | `nil` | Non-nil means `<TAB>` moves to the right-hand column in preparation for a comment. |
| `verilog-auto-newline` | `t` | Non-nil automatically inserts a newline after typing semicolons (`;`). |
| `verilog-auto-lineup-on-semi` | `t` | Non-nil means typing `;` runs the same declaration/block lineup as `<TAB>`; `nil` disables auto-lineup on `;` so only `<TAB>` triggers block alignment. |
| `verilog-auto-indent-on-newline` | `t` | Non-nil automatically indents the line after `<RET>` / newline. |
| `verilog-auto-lineup` | `'declarations` | Statements to line up across multiple lines: `'declarations`, `'assignments` (aligns procedural `=` / `<=` and, when `verilog-align-assign-expr` is non-nil, continuous `assign` blocks), `'all`, or `nil`. |
| `verilog-align-decl-dimension` | `nil` | Master switch for Tree-Sitter tabular alignment of ANSI ports, `parameter` / `localparam` port lists and declaration blocks, `struct` / `union` / `enum` members, module instantiation ports, and declaration packed dimensions (`[dim0][dim1]` with no spaces between brackets when `verilog-align-decl-sub-dimension` is `nil`). |
| `verilog-align-decl-sub-dimension` | `nil` | Non-nil aligns each packed sub-dimension into its own right-to-left sub-column (`[dim0] [dim1] ...`) when `verilog-align-decl-dimension` is also `t` (ignored when `verilog-align-decl-dimension` is `nil`). |
| `verilog-align-ifelse` | `nil` | Non-nil aligns `else` under matching `if`; `nil` aligns with the first character on the line holding `if`. |
| `verilog-align-decl-expr-comments` | `t` | Non-nil aligns trailing inline comments (`// ...` and `/* ... */`) into a right-hand column across aligned declaration, port, instantiation, parameter, enum, and assignment blocks. |
| `verilog-align-comment-distance` | `1` | Distance (in spaces) between longest declaration/expression and aligned trailing comments. |
| `verilog-align-assign-expr` | `nil` | Non-nil aligns expressions of continuous `assign` statements when aligning assignment blocks (`verilog-auto-lineup` set to `'assignments` or `'all`, or via `M-x verilog-pretty-expr`). |
| `verilog-align-typedef-regexp` | `nil` | Regexp matching user-defined typedefs for declaration alignment (e.g., `"_t$"`). |
| `verilog-align-typedef-words` | `nil` | List of literal type names matching user-defined typedefs for declaration alignment. |
| `verilog-minimum-comment-distance` | `10` | Minimum distance (in lines) between `begin` and `end` required before auto-inserting an end-comment. |
| `verilog-auto-inst-column` | `40` | Indent-to column number for the net/signal connection part `(net)` of `/*AUTOINST*/` pins. |

### 5.2 Syntax Highlighting & Visuals (`verilog-mode-indent` / `verilog-mode-actions`)

| Variable / Face | Default | Description |
| :--- | :--- | :--- |
| `verilog-fontify-variables` | `t` | Non-nil fontifies variable names in declarations. |
| `verilog-highlight-translate-off` | `nil` | Non-nil background-highlights code between `// synopsys translate_off` and `translate_on`. |
| `verilog-highlight-grouping-keywords` | `nil` | Non-nil highlights structural grouping keywords (`begin`..`end`, `case`..`endcase`, `module`..`endmodule`) using `verilog-font-lock-grouping-keywords-face`. |
| `verilog-highlight-includes` | `t` | Non-nil highlights `` `include `` filenames for mouse/RET navigation (`verilog-load-file-at-point`). |
| `verilog-highlight-modules` | `nil` | Non-nil highlights submodule instantiations for mouse/RET navigation (`verilog-load-file-at-point`). |
| `verilog-highlight-uvm` | `nil` | Non-nil highlights UVM class/type names (`uvm_*`) using `verilog-font-lock-uvm-face`, UVM constants/enums (`UVM_*`) using `font-lock-constant-face`, and variable names declared with `uvm_*` types. |
| `verilog-highlight-max-lookahead` | `10000` | Maximum character lookahead in a declaration statement for variable fontification (`0` = unlimited). |
| `verilog-highlight-p1800-keywords` | `nil` | Obsolete (all files are highlighted with full SystemVerilog IEEE 1800 support). |
| `verilog-font-lock-translate-off-face` | *(face)* | Face used for `translate_off` regions when `verilog-highlight-translate-off` is non-nil. |
| `verilog-font-lock-grouping-keywords-face` | *(face)* | Face used for grouping keywords when `verilog-highlight-grouping-keywords` is non-nil. |
| `verilog-font-lock-uvm-face` | *(face)* | Face used for UVM class and type names (`uvm_*`) when `verilog-highlight-uvm` is non-nil (inherits from `font-lock-type-face`). |
| `verilog-font-lock-p1800-face` | *(face)* | Face used for IEEE 1800 SystemVerilog keywords. |
| `verilog-font-lock-ams-face` | *(face)* | Face used for Verilog-AMS keywords. |

* Note
 Currently in verilog-mode.el, lowercase uvm_* types use verilog-font-lock-uvm-face while uppercase UVM_* constants use font-lock-constant-face.

### 5.3 Library & Submodule Search Paths (`verilog-mode-auto`)

| Variable | Default | Description |
| :--- | :--- | :--- |
| `verilog-library-flags` | `'("")` | List of simulator-style flags (`-y dir`, `+incdir+dir`, `-Idir`, `+libext+.v+.sv`, `-v file`, `-f file`, `-F file`) used to resolve submodules and includes for `/*AUTOINST*/`. |
| `verilog-library-directories` | `'( "." )` | List of directories searched when resolving submodule definitions for `/*AUTOINST*/`. |
| `verilog-library-files` | `'()` | List of specific library/technology files searched for module definitions. |
| `verilog-library-extensions` | `'(".v" ".va" ".sv")` | List of file extensions tried when searching `verilog-library-directories` for a module name. |
| `verilog-case-fold` | `t` | Non-nil means `verilog-mode` searches ignore case (`nil` recommended for strict SystemVerilog). |
| `verilog-typedef-regexp` | `nil` | Regexp matching user-defined Verilog-2001/SystemVerilog typedef names (e.g., `"_t$"`) during AUTO parsing. |

### 5.4 AUTO Expansion (`/*AUTO...*/`) & Actions (`verilog-mode-auto` / `verilog-mode-actions`)

| Variable | Default | Description |
| :--- | :--- | :--- |
| `verilog-auto-endcomments` | `t` | Non-nil inserts trailing comments (e.g., `// case: ...`, `// block: ...`) after `end` keywords. |
| `verilog-auto-wire-type` | `nil` | Data type used by `/*AUTOWIRE*/` (e.g., `"logic"` or `"wire"`). |
| `verilog-auto-wire-comment` | `t` | Non-nil inserts `// To/From ... of ...` comments on `/*AUTOWIRE*/` declarations. |
| `verilog-auto-declare-nettype` | `nil` | Data type used with `/*AUTOINPUT*/` / `/*AUTOOUTPUT*/` when `` `default_nettype none `` is active (e.g., `"wire"`). |
| `verilog-auto-delete-trailing-whitespace` | `nil` | Non-nil strips trailing whitespace during `M-x verilog-auto`. |
| `verilog-auto-ignore-concat` | `nil` | Non-nil excludes signals inside `{...}` concatenations from `/*AUTOWIRE*/`, `/*AUTOOUTPUT*/`, etc. |
| `verilog-auto-read-includes` | `nil` | Non-nil automatically runs `verilog-read-defines` and `verilog-read-includes` before expanding AUTOs. |
| `verilog-auto-save-policy` | `nil` | Action when saving a buffer with AUTOs: `nil`, `'ask`, `'detect`, or `'force`. |
| `verilog-auto-star-expand` | `t` | Non-nil expands SystemVerilog `.*` implicit ports during `verilog-auto`. |
| `verilog-auto-star-save` | `nil` | Non-nil keeps expanded `.*` port connections in the buffer when saving to disk. |
| `verilog-active-low-regexp` | `nil` | Regexp matching active-low signal names (e.g., `"_n$"`), used by `/*AUTORESET*/` and `/*AUTOTIEOFF*/`. |
| `verilog-auto-sense-include-inputs` | `nil` | Non-nil includes all read signals in `/*AUTOSENSE*/` even if also written in the same `always` block. |
| `verilog-auto-sense-defines-constant` | `nil` | Non-nil assumes all `` `define `` macro references inside `always` blocks are constants for `/*AUTOSENSE*/`. |
| `verilog-auto-simplify-expressions` | `t` | Non-nil simplifies constant arithmetic expressions when computing bus bit ranges in AUTOs. |
| `verilog-auto-reset-blocking-in-non` | `t` | Non-nil resets signals assigned with `=` inside `/*AUTORESET*/` blocks that also use `<=`. |
| `verilog-auto-reset-widths` | `t` | Controls width formatting of zero literals in `/*AUTORESET*/`: `t` (sized `32'h0`), `nil` (`0`), or `'unbased` (`'0`). |
| `verilog-assignment-delay` | `""` | Delay text inserted in `/*AUTORESET*/` nonblocking assignments (e.g., `"#1 "`). |
| `verilog-auto-arg-format` | `'packed` | Formatting for `/*AUTOARG*/` port lists: `'packed` (multiple per line up to `fill-column`) or `'single` (one per line). |
| `verilog-auto-arg-sort` | `nil` | Non-nil sorts `/*AUTOARG*/` port names alphabetically instead of declaration order. |
| `verilog-auto-inst-dot-name` | `nil` | Non-nil uses SystemVerilog `.port` syntax instead of `.port(port)` in `/*AUTOINST*/` when net and port names match. |
| `verilog-auto-inst-param-value` | `nil` | Non-nil substitutes `#(...)` instance parameter values into `/*AUTOINST*/` port widths and types. |
| `verilog-auto-inst-param-value-type` | `t` | Non-nil expands `parameter type` overrides in `/*AUTOINST*/` when `verilog-auto-inst-param-value` is enabled. |
| `verilog-auto-inst-sort` | `nil` | Non-nil sorts `/*AUTOINST*/` and `/*AUTOINSTPARAM*/` pins alphabetically within input/inout/output groups. |
| `verilog-auto-inst-vector` | `t` | Controls bus subscripts on `/*AUTOINST*/` pins: `t` (always include `[msb:lsb]`), `nil` (omit when matching full bus), or `'unsigned`. |
| `verilog-auto-inst-template-numbers` | `nil` | Annotates `/*AUTOINST*/` pins matched by `AUTO_TEMPLATE`: `nil`, `t` (line number), or `'lhs` (matched rule pattern). |
| `verilog-auto-inst-template-required` | `nil` | Non-nil omits any `/*AUTOINST*/` port that does not match an explicit `AUTO_TEMPLATE` rule. |
| `verilog-auto-inst-interfaced-ports` | `nil` | Non-nil includes SystemVerilog interface ports in `/*AUTOINST*/` expansions. |
| `verilog-auto-input-ignore-regexp` | `nil` | Regexp of signal names to exclude from `/*AUTOINPUT*/`. |
| `verilog-auto-reg-input-assigned-ignore-regexp` | `nil` | Regexp of signal names to exclude from `/*AUTOINPUT*/` / `/*AUTOREGINPUT*/`. |
| `verilog-auto-inout-ignore-regexp` | `nil` | Regexp of signal names to exclude from `/*AUTOINOUT*/`. |
| `verilog-auto-output-ignore-regexp` | `nil` | Regexp of signal names to exclude from `/*AUTOOUTPUT*/`. |
| `verilog-auto-tieoff-ignore-regexp` | `nil` | Regexp of signal names to exclude from `/*AUTOTIEOFF*/`. |
| `verilog-auto-unused-ignore-regexp` | `nil` | Regexp of signal names to exclude from `/*AUTOUNUSED*/`. |
| `verilog-auto-tieoff-declaration` | `"wire"` | Declaration/statement style for `/*AUTOTIEOFF*/` (`"wire"`, `"assign"`, or a data type like `"logic"`). |
| `verilog-auto-template-warn-unused` | `nil` | Non-nil warns when an `AUTO_TEMPLATE` rule is not used by any instantiation. |
| `verilog-warn-fatal` | `nil` | Non-nil turns `verilog-warn-error` warnings during AUTO expansion into fatal errors. |

### 5.5 External Compilation, Linting & Simulation Tools (`verilog-mode-actions`)

| Variable | Default | Description |
| :--- | :--- | :--- |
| `verilog-linter` | `"echo 'No verilog-linter set...'"` | Shell command invoked by `M-x compile` when `verilog-tool` is `'verilog-linter`. |
| `verilog-compiler` | `"echo 'No verilog-compiler set...'"` | Shell command invoked by `M-x compile` when `verilog-tool` is `'verilog-compiler`. |
| `verilog-simulator` | `"echo 'No verilog-simulator set...'"` | Shell command invoked by `M-x compile` when `verilog-tool` is `'verilog-simulator`. |
| `verilog-coverage` | `"echo 'No verilog-coverage set...'"` | Shell command invoked by `M-x compile` when `verilog-tool` is `'verilog-coverage`. |
| `verilog-preprocessor` | `"verilator -E __FLAGS__ __FILE__"` | Shell command invoked by `M-x verilog-preprocess` or `M-x compile`. |

### 5.6 Hooks (`verilog-mode` / `verilog-mode-auto`)

| Hook Variable | Default | Description |
| :--- | :--- | :--- |
| `verilog-mode-hook` | `(list #'verilog-set-compile-command)` | Hook run when entering `verilog-mode`. |
| `verilog-before-auto-hook` | `nil` | Hook run before `M-x verilog-auto` updates AUTOs. |
| `verilog-auto-hook` | `nil` | Hook run after `M-x verilog-auto` updates AUTOs. |
| `verilog-before-delete-auto-hook` | `nil` | Hook run before `M-x verilog-delete-auto` deletes AUTOs. |
| `verilog-delete-auto-hook` | `nil` | Hook run after `M-x verilog-delete-auto` deletes AUTOs. |
| `verilog-before-getopt-flags-hook` | `nil` | Hook run before `verilog-getopt-flags` computes library flags. |
| `verilog-getopt-flags-hook` | `nil` | Hook run after `verilog-getopt-flags` computes library flags. |
| `verilog-before-save-font-hook` | `nil` | Hook run before removing fontification during batch/AUTO operations. |
| `verilog-after-save-font-hook` | `nil` | Hook run after restoring fontification following batch/AUTO operations. |

# Building verilog-mode.el from source

## 1. Prerequisites

1. **GNU Emacs $\ge$ 26.1 with Dynamic Module Support**:
   - Emacs must be compiled with `--with-modules` (enabled by default in standard distributions and in the bundled `opt/bin/emacs-26.1`).
   - Verify inside Emacs with `M-: (functionp 'module-load)` — it must return `t`.
2. **C/C++ Toolchain**:
   - `gcc` (C11) and `g++` **$\ge$ 8.5** with `-std=c++17` support, plus `make`, `ar`, and `perl`.


## Building the Libraries, Dynamic Module, and Byte-Compiled Elisp

From the root of this repository (`/path/to/verilog-mode-ng`),
Simple enough in one stop and it will build src/verilog-ts-core.so and byte-compile e/verilog-mode.elc:

```bash
make compile [EMACS=/path/to/emacs]
```

The option `EMACS` could be set as different emacs.

### Option: Build verilog-ts-core.so manually.

If you want to build verilog-ts-core.so manually, do the following commands.
The verilog-ts-core.so is a core engine which used by verilog-mode.el.

It also build two things

1. Build the Tree-Sitter C runtime static archive:
2. Build the IEEE 1800-2023 SystemVerilog grammar static archive:

```bash
# Build tree-sitter core library, systemverilog parser and our emacs binding.
make ts_module
```

## Option: Testing

Testing

```bash
# Test the tree-sitter library
make test_ts

# Test bunch of tests
make test_batch
```

# Installing Tree-Sitter Powered `verilog-mode` (`verilog-mode-ng`)

GNU Emacs ships with an older built-in `verilog-mode.elc` under `<emacs-prefix>/share/emacs/<version>/lisp/progmodes/verilog-mode.elc`. This guide explains how to build the new Tree-Sitter-powered `verilog-mode` (`verilog-mode.el` + `verilog-ts-core.so`) and configure Emacs so it overrides the built-in version.

After building, the two artifacts needed at runtime are:
- `e/verilog-mode.el` and `e/verilog-mode.elc` (version-stamped Elisp and byte-compiled `.elc`)
- `src/verilog-ts-core.so` (self-contained C++17 Tree-Sitter dynamic module with `libtree-sitter.a` and `libtree-sitter-systemverilog.a` statically linked)

---

## Replacing the Default Built-In `verilog-mode`

Emacs resolves libraries by searching `load-path` in order from front to back. Because the built-in `progmodes/verilog-mode.elc` is near the end of `load-path`, **prepending** the new `verilog-mode` directory to `load-path` via `add-to-list` shadows the built-in version automatically.

Choose **one** of the three installation methods below:

---

### Option A: Per-User Installation into `~/.emacs.d/lisp/verilog-mode/` (Recommended)

1. **Copy the built files to `~/.emacs.d/lisp/verilog-mode/`**:
   ```bash
   mkdir -p ~/.emacs.d/lisp/verilog-mode
   cp e/verilog-mode.el  ~/.emacs.d/lisp/verilog-mode/
   cp e/verilog-mode.elc ~/.emacs.d/lisp/verilog-mode/
   cp src/verilog-ts-core.so ~/.emacs.d/lisp/verilog-mode/
   ```

2. **Add the following to your `~/.emacs` or `~/.emacs.d/init.el`** (place this **before** any other config that requires `verilog-mode`):
   ```elisp
   ;; 1. Prepend the new verilog-mode directory to `load-path' so it shadows
   ;;    Emacs's built-in lisp/progmodes/verilog-mode.elc:
   (add-to-list 'load-path (expand-file-name "lisp/verilog-mode" user-emacs-directory))

   ;; 2. If the built-in verilog-mode was already loaded earlier in init, unload it:
   (when (featurep 'verilog-mode)
     (unload-feature 'verilog-mode t))

   ;; 3. Register autoload and file associations for Verilog / SystemVerilog:
   (autoload 'verilog-mode "verilog-mode" "Verilog and SystemVerilog mode (Tree-Sitter)." t)
   (add-to-list 'auto-mode-alist '("\\.[ds]?va?h?\\'" . verilog-mode))
   (add-to-list 'auto-mode-alist '("\\.svi\\'"        . verilog-mode))
   ```

---

### Option B: Load Directly from the Build Tree (For Active Development)

If you want Emacs to load directly from your `verilog-mode-ng` checkout without copying files:

Add the following to your `~/.emacs` or `~/.emacs.d/init.el`:
```elisp
;; Prepend `e' (which automatically finds `./src/verilog-ts-core.so'):
(add-to-list 'load-path "/path/to/verilog-mode-ng/e")

(when (featurep 'verilog-mode)
  (unload-feature 'verilog-mode t))

(autoload 'verilog-mode "verilog-mode" "Verilog and SystemVerilog mode (Tree-Sitter)." t)
(add-to-list 'auto-mode-alist '("\\.[ds]?va?h?\\'" . verilog-mode))
```

---

### Option C: System-Wide `site-lisp` or Direct Built-In Replacement

#### 1. System-Wide `site-lisp` (All Users)
`site-lisp` is searched before Emacs's built-in `lisp/progmodes/` directory:
```bash
sudo install -d /usr/share/emacs/site-lisp/verilog-mode
sudo install -m 644 e/verilog-mode.el  /usr/share/emacs/site-lisp/verilog-mode/
sudo install -m 644 e/verilog-mode.elc /usr/share/emacs/site-lisp/verilog-mode/
sudo install -m 755 src/verilog-ts-core.so /usr/share/emacs/site-lisp/verilog-mode/
```
Then add `(add-to-list 'load-path "/usr/share/emacs/site-lisp/verilog-mode")` to `/etc/emacs/site-start.el` (or `~/.emacs`).

#### 2. Directly Overwriting the Built-In Files in an Emacs Installation (e.g., `opt/`)
If you use a dedicated Emacs prefix such as `/path/to/verilog-mode-ng/opt` and want to replace the built-in files in place:
```bash
EMACS_PROGMODES=/path/to/verilog-mode-ng/opt/share/emacs/26.1/lisp/progmodes

# Back up the original built-in files:
cp "${EMACS_PROGMODES}/verilog-mode.el.gz" "${EMACS_PROGMODES}/verilog-mode.el.gz.bak" 2>/dev/null || true
cp "${EMACS_PROGMODES}/verilog-mode.elc"   "${EMACS_PROGMODES}/verilog-mode.elc.bak"

# Remove the old gzipped source so Emacs doesn't warn about shadowing:
rm -f "${EMACS_PROGMODES}/verilog-mode.el.gz"

# Copy the new Elisp, byte-compiled .elc, and Tree-Sitter dynamic module:
cp verilog-mode/e/verilog-mode.el      "${EMACS_PROGMODES}/verilog-mode.el"
cp verilog-mode/e/verilog-mode.elc     "${EMACS_PROGMODES}/verilog-mode.elc"
cp verilog-mode/src/verilog-ts-core.so "${EMACS_PROGMODES}/verilog-ts-core.so"
```
With this method, no `~/.emacs` changes are needed—every `emacs` invocation from that prefix will automatically load the new `verilog-mode.elc` and `verilog-ts-core.so`.

---

## 4. Verifying the Installation

Open any `.v` or `.sv` file in Emacs and run the following checks:

1. **Verify which `verilog-mode` file is loaded**:
   ```
   M-x locate-library RET verilog-mode RET
   ```
   Confirm the path points to your new `verilog-mode.elc` (and **not** the old built-in `progmodes/verilog-mode.elc`, unless you used Option C.2).

2. **Verify `verilog-ts-core.so` is active**:
   ```
   M-: (verilog-ts-version) RET
   ```
   Expected output in the echo area:
   ```
   "verilog-ts-core 1.0 (tree-sitter IEEE 1800-2023)"
   ```

3. **Test Interactive Tree-Sitter AST Inspection**:
   - Move point onto any SystemVerilog construct and run:
     ```
     M-x verilog-ts-show-node-at-point RET
     ```
     It will display the Tree-Sitter node type, byte/character range, and S-expression at point.
   - To view the full S-expression syntax tree for the current buffer:
     ```
     M-x verilog-ts-show-ast RET
     ```

---

---

## 6. Troubleshooting

- **`Cannot find verilog-ts-core.so; run 'make -C verilog-mode/src'`**:
  `verilog-mode.el` searches for `verilog-ts-core.so` in:
  1. The same directory as `verilog-mode.el` / `verilog-mode.elc` (`./verilog-ts-core.so`)
  2. `./src/verilog-ts-core.so`
  3. `../src/verilog-ts-core.so`
  4. Any directory in Emacs's `load-path` (`(locate-library "verilog-ts-core.so")`)
  Make sure `verilog-ts-core.so` is placed in one of those locations and has read/execute permissions (`chmod 755 verilog-ts-core.so`).

- **`Symbol's function definition is void: module-load`**:
  Your Emacs binary was compiled without dynamic module support. Recompile Emacs with `./configure --with-modules` (or use `opt/bin/emacs-26.1`).

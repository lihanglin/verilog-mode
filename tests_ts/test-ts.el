;;; test-ts.el --- Phase 1 standalone tests for verilog-ts-core.so -*- lexical-binding: t -*-

(require 'ert)
(require 'cl-lib)

(defvar verilog-ts-test-dir
  (file-name-directory (or load-file-name buffer-file-name))
  "Directory containing `test-ts.el' and sample `.sv' fixtures.")

(defvar verilog-ts-module-path
  (let ((main-so (expand-file-name "../src/verilog-ts-core.so" verilog-ts-test-dir))
        (local-so (expand-file-name "src/verilog-ts-core.so" verilog-ts-test-dir)))
    (if (file-exists-p main-so) main-so local-so))
  "Path to the compiled `verilog-ts-core.so' dynamic module.")

(defvar verilog-ts-sample-file
  (expand-file-name "sample_counter.sv" verilog-ts-test-dir)
  "Path to the sample SystemVerilog fixture file.")

;; Load the dynamic module before running tests
(unless (featurep 'verilog-ts-core)
  (module-load verilog-ts-module-path))

(defmacro with-verilog-ts-sample-buffer (&rest body)
  "Execute BODY in a temporary buffer populated with `sample_counter.sv'."
  (declare (indent 0) (debug t))
  `(with-temp-buffer
     (insert-file-contents verilog-ts-sample-file)
     (goto-char (point-min))
     ,@body))

;;; 1. Module Loading & Core AST Primitives

(ert-deftest verilog-ts-test-01-module-loaded ()
  "Verify `verilog-ts-core.so' loads and reports its version string."
  (should (featurep 'verilog-ts-core))
  (let ((ver (verilog-ts-version)))
    (should (stringp ver))
    (should (string-match-p "verilog-ts-core" ver))
    (should (string-match-p "IEEE 1800-2023" ver))))

(ert-deftest verilog-ts-test-02-parse-and-ast ()
  "Verify parsing `sample_counter.sv' and inspecting AST nodes."
  (with-verilog-ts-sample-buffer
    (let ((status (verilog-ts-parse-buffer)))
      (should (eq status t)))
    (let ((sexp (verilog-ts-root-sexp)))
      (should (stringp sexp))
      (should (string-match-p "module_declaration" sexp))
      (should (string-match-p "package_declaration" sexp)))
    ;; Inspect node at `module sample_counter'
    (goto-char (point-min))
    (search-forward "module sample_counter")
    (goto-char (match-beginning 0))
    (let ((node (verilog-ts-node-at (point) nil))
          (hdr-node (verilog-ts-enclosing-node
                     '("module_declaration" "module_ansi_header") (point)))
          (mod-node (verilog-ts-enclosing-node
                     '("module_declaration") (point))))
      (should node)
      (should (equal (plist-get node :type) "module"))
      (should hdr-node)
      (should (equal (plist-get hdr-node :type) "module_ansi_header"))
      (should mod-node)
      (should (equal (plist-get mod-node :type) "module_declaration"))
      (let ((children (verilog-ts-children (plist-get mod-node :start) t)))
        (should (listp children))
        (should (> (length children) 0))))
    ;; Test S-expression query
    (let ((matches (verilog-ts-query "(module_keyword) @kw")))
      (should (= (length matches) 2))
      (should (equal (plist-get (car matches) :capture) "kw")))
    ;; Test cache flush
    (should (eq (verilog-ts-cache-flush) t))))

;;; 2. Syntactic Scanning, Comments, Strings & Text Properties

(ert-deftest verilog-ts-test-03-scan-region-and-comments ()
  "Verify `verilog-ts-scan-region', comment/string detection, and syntactic ws."
  (with-verilog-ts-sample-buffer
    (verilog-ts-scan-region (point-min) (point-max))
    ;; Check line 1 comment: "// Sample SystemVerilog..."
    ;; First '/' at pos 1 has no v-cmts; second '/' through '\n' has v-cmts = t
    (should-not (get-text-property 1 'v-cmts))
    (should (eq (get-text-property 2 'v-cmts) t))
    (goto-char (point-min))
    (end-of-line)
    ;; Even the newline character at the end of a `//' comment has v-cmts = t
    (should (eq (get-text-property (point) 'v-cmts) t))

    ;; Check block comment detection
    (goto-char (point-min))
    (search-forward "containing keywords like module")
    (should (eq (verilog-ts-inside-comment-or-string-p (point)) 'block-comment))
    (let ((bounds (verilog-ts-comment-or-string-bounds (point))))
      (should bounds)
      (should (eq (plist-get bounds :type) 'block-comment)))

    ;; Check string literal detection
    (goto-char (point-min))
    (search-forward "module inside string")
    (should (eq (verilog-ts-inside-comment-or-string-p (point)) 'string))
    ;; Inside the string, v-cmts property should be t
    (should (eq (get-text-property (point) 'v-cmts) t))
    (let ((bounds (verilog-ts-comment-or-string-bounds (point))))
      (should bounds)
      (should (eq (plist-get bounds :type) 'string)))

    ;; Check code outside comment/string returns nil
    (goto-char (point-min))
    (search-forward "assign count_out")
    (should-not (verilog-ts-inside-comment-or-string-p (point)))
    (should-not (verilog-ts-comment-or-string-bounds (point)))

    ;; Check comment spans list
    (let ((spans (verilog-ts-comment-spans)))
      (should (> (length spans) 5))
      (should (cl-some (lambda (s) (eq (car s) 'line-comment)) spans))
      (should (cl-some (lambda (s) (eq (car s) 'block-comment)) spans))
      (should (cl-some (lambda (s) (eq (car s) 'string)) spans)))

    ;; Check forward/backward syntactic whitespace skipping
    (goto-char (point-min))
    (goto-char (verilog-ts-forward-syntactic-ws))
    (should (looking-at "`define COUNTER_WIDTH"))
    (goto-char (point-min))
    (search-forward "package sample_pkg;")
    (goto-char (match-beginning 0))
    (let ((prev-pos (verilog-ts-backward-syntactic-ws)))
      (should (< prev-pos (point))))))

;;; 3. Structural Navigation & Context Queries

(ert-deftest verilog-ts-test-04-navigation-and-modules ()
  "Verify module discovery, block bounds, leap-to-head, and context queries."
  (with-verilog-ts-sample-buffer
    ;; Find all modules/packages/defuns in buffer (ignoring "module" in comments/strings)
    (let ((mods (verilog-ts-find-modules-in-buffer))
          (all-defuns (verilog-ts-find-modules-in-buffer t)))
      (should (equal (mapcar (lambda (d) (plist-get d :name)) mods)
                     '("sample_pkg" "sub_block" "sample_counter")))
      (should (equal (mapcar (lambda (d) (plist-get d :name)) all-defuns)
                     '("sample_pkg" "sub_block" "sample_counter"
                       "calc_next" "reset_task"))))

    ;; Check module-at-point and defun-bounds inside `sample_counter'
    (goto-char (point-min))
    (search-forward "assign count_out")
    (let ((mod (verilog-ts-module-at-point))
          (bounds (verilog-ts-defun-bounds (point))))
      (should mod)
      (should (equal (plist-get mod :name) "sample_counter"))
      (should (equal (plist-get mod :type) "module"))
      (should bounds)
      (should (= (plist-get bounds :start) (plist-get mod :start)))
      (should (= (plist-get bounds :end) (plist-get mod :end))))

    ;; Check block bounds and leap-to-head on `begin : seq_proc ... end : seq_proc'
    (goto-char (point-min))
    (search-forward "begin : seq_proc")
    (goto-char (match-beginning 0))
    (let ((bpos (point))
          (blk (verilog-ts-block-bounds (point))))
      (should blk)
      (should (equal (plist-get blk :type) "begin"))
      ;; Jump from `begin' to matching `end'
      (let ((epos (verilog-ts-forward-sexp)))
        (should epos)
        ;; Jump from `end' back to matching `begin' via leap-to-head
        (goto-char (plist-get blk :end-kwd))
        (let ((head (verilog-ts-leap-to-head)))
          (should (= head bpos)))))

    ;; Check statement bounds
    (goto-char (point-min))
    (search-forward "assign count_out = sub_out;")
    (goto-char (match-beginning 0))
    (let ((sbounds (verilog-ts-statement-bounds (point))))
      (should (consp sbounds))
      (should (<= (car sbounds) (point)))
      (should (> (cdr sbounds) (point))))

    ;; Check context predicates
    (goto-char (point-min))
    (search-forward "parameter int WIDTH = `COUNTER_WIDTH")
    (should (verilog-ts-in-context-p 'parameter (point)))
    (goto-char (point-min))
    (search-forward "`define COUNTER_WIDTH")
    (should (verilog-ts-in-context-p 'directive (point)))))

;;; 4. Indentation, Alignment & End-Comment Calculation

(ert-deftest verilog-ts-test-05-indent-align-and-endcomment ()
  "Verify indentation calculation, alignment info, and endcomment info."
  (with-verilog-ts-sample-buffer
    ;; Top-level module header line has indent `(cpp 0)'
    (goto-char (point-min))
    (search-forward "module sample_counter")
    (beginning-of-line)
    (should (equal (verilog-ts-calculate-indent) '(cpp 0)))

    ;; Module-level item (`always_ff ...') has indent `(defun 0)'
    (goto-char (point-min))
    (search-forward "always_ff @(posedge clk or negedge rst_n) begin : seq_proc")
    (beginning-of-line)
    (should (equal (verilog-ts-calculate-indent) '(defun 0)))

    ;; Statement inside nested `else begin' (at col 6) has governing `(block 6)'
    (goto-char (point-min))
    (search-forward "count_q <= count_d;")
    (beginning-of-line)
    (should (equal (verilog-ts-calculate-indent) '(block 6)))

    ;; Closing `end : seq_proc' of `always_ff ... begin : seq_proc' (at col 3) has `(block 3)'
    (goto-char (point-min))
    (search-forward "end : seq_proc")
    (beginning-of-line)
    (should (equal (verilog-ts-calculate-indent) '(block 3)))

    ;; Test declaration alignment info on a port line of sample_counter
    (goto-char (point-min))
    (search-forward "input  logic             clk,")
    (beginning-of-line)
    (let ((dinfo (verilog-ts-decl-align-info (point))))
      (should dinfo)
      (should (eq (plist-get dinfo :is-decl) t))
      (should (equal (plist-get dinfo :node-type) "ansi_port_declaration")))

    ;; Test expression assignment alignment info
    (goto-char (point-min))
    (search-forward "count_q <= count_d;")
    (beginning-of-line)
    (let ((ainfo (verilog-ts-expr-align-info (point))))
      (should ainfo)
      (should (eq (plist-get ainfo :is-assign) t))
      (should (equal (plist-get ainfo :op) "<=")))

    ;; Test end-comment info on `endmodule'
    (goto-char (point-max))
    (re-search-backward "^endmodule")
    (let ((ec (verilog-ts-endcomment-info (point))))
      (should ec)
      (should (equal (plist-get ec :closer) "endmodule"))
      (should (eq (plist-get ec :matched) t))
      (should (equal (plist-get ec :label) "sample_counter")))))

;;; 5. AUTO Declaration, Instantiation, Sensitivity & Imenu Extraction

(ert-deftest verilog-ts-test-06-decls-insts-always-and-imenu ()
  "Verify `verilog-ts-read-decls', instantiations, always blocks, defines, and imenu."
  (with-verilog-ts-sample-buffer
    ;; Locate sample_counter module range
    (goto-char (point-min))
    (search-forward "module sample_counter")
    (let* ((mod (verilog-ts-module-at-point))
           (mstart (plist-get mod :start))
           (mend (plist-get mod :end))
           (decls (verilog-ts-read-decls mstart mend))
           (ast-decls (verilog-ts-ast-decls mstart mend)))
      ;; 9-element vector: [outputs inouts inputs vars modports assigns consts gparams interfaces]
      (should (= (length decls) 9))
      (let ((outputs (aref decls 0))
            (inputs  (aref decls 2)))
        (should (assoc "count_out" outputs))
        (should (assoc "overflow" outputs))
        (should (assoc "clk" inputs))
        (should (assoc "rst_n" inputs))
        (should (assoc "enable" inputs))
        (should (assoc "load_val" inputs)))
      (should (plist-get ast-decls :outputs))
      (should (plist-get ast-decls :inputs))
      (should (plist-get ast-decls :vars))

      ;; Verify submodule instantiation extraction (`sub_block u_sub')
      (let ((insts (verilog-ts-ast-instantiations mstart mend)))
        (should (= (length insts) 1))
        (let ((inst (car insts)))
          (should (equal (plist-get inst :module) "sub_block"))
          (should (equal (plist-get inst :instance) "u_sub"))
          (should (= (length (plist-get inst :ports)) 4))))

      ;; Verify `verilog-ts-read-inst-info' at instantiation
      (goto-char mstart)
      (search-forward "u_sub")
      (let ((iinfo (verilog-ts-read-inst-info (point))))
        (should iinfo)
        (should (equal (plist-get iinfo :module) "sub_block"))
        (should (equal (plist-get iinfo :instance) "u_sub")))

      ;; Verify always block RHS/LHS signal extraction for AUTOSENSE / AUTORESET
      ;; Vector: [outputs-delayed outputs-immediate temps inputs]
      (goto-char mstart)
      (search-forward "always @* begin")
      (goto-char (match-beginning 0))
      (let ((asigs (verilog-ts-read-always-signals (point))))
        (should (= (length asigs) 4))
        (let ((out-imm (aref asigs 1))
              (inputs  (aref asigs 3)))
          (should (assoc "count_d" out-imm))
          (should (assoc "overflow" out-imm))
          (should (assoc "count_q" inputs))
          (should (assoc "enable" inputs))
          (should (assoc "load_val" inputs))))

      ;; Verify `verilog-ts-ast-always-blocks'
      (let ((ablks (verilog-ts-ast-always-blocks)))
        (should (= (length ablks) 3)))

      ;; Verify `verilog-ts-read-signals'
      (let ((sigs (verilog-ts-read-signals mstart mend)))
        (should (member "count_q" sigs))
        (should (member "count_d" sigs))))

    ;; Verify `verilog-ts-read-defines'
    (let ((defs (verilog-ts-read-defines)))
      (should (assoc "COUNTER_WIDTH" (plist-get defs :defines)))
      (should (assoc "STEP_VAL" (plist-get defs :defines))))

    ;; Verify `verilog-ts-imenu-create-index'
    (let ((idx (verilog-ts-imenu-create-index)))
      (should idx)
      (should (assoc "Modules" idx))
      (should (assoc "Packages" idx))
      (should (assoc "Instances" idx))
      (should (assoc "Tasks & Functions" idx)))))

;;; 6. Phase 2 `verilog-mode.el' Full Integration

(ert-deftest verilog-ts-test-07-verilog-mode-integration ()
  "Verify Phase 2 `verilog-mode' integration with `verilog-ts-core'."
  (let ((vm-elc (expand-file-name "../e/verilog-mode.elc" verilog-ts-test-dir))
        (vm-el  (expand-file-name "../verilog-mode.el" verilog-ts-test-dir)))
    (unless (featurep 'verilog-mode)
      (load (if (file-exists-p vm-elc) vm-elc vm-el) nil t))
    (with-verilog-ts-sample-buffer
      (verilog-mode)
      (should (eq major-mode 'verilog-mode))
      ;; Verify defun movement via Tree-Sitter
      (goto-char (point-min))
      (search-forward "assign count_out")
      (verilog-beg-of-defun)
      (should (looking-at "module\\s-+sample_counter"))
      (verilog-end-of-defun)
      (should (looking-back "endmodule" (- (point) 12)))
      ;; Verify parameter and attribute context predicates via Tree-Sitter
      (goto-char (point-min))
      (search-forward "parameter int WIDTH")
      (should (verilog-in-parameter-p))
      ;; Verify `<TAB>' indentation via `electric-verilog-tab'
      (goto-char (point-min))
      (search-forward "count_q <= count_d;")
      (beginning-of-line)
      (delete-horizontal-space)
      (electric-verilog-tab)
      (should (> (current-indentation) 0))
      ;; Verify interactive AST inspection helper
      (verilog-ts-show-node-at-point))))

;;; 7. Phase 3 Tabular Alignment & Lineup Settings (PLAN.md Sections 1.1 - 1.5)

(ert-deftest verilog-ts-test-08-phase3-tabular-alignment-and-settings ()
  "Verify ANSI ports (1.1), struct/union (1.2), inst ports (1.3), decl dims (1.4), and semi/TAB settings (1.5)."
  (let ((vm-elc (expand-file-name "../e/verilog-mode.elc" verilog-ts-test-dir))
        (vm-el  (expand-file-name "../verilog-mode.el" verilog-ts-test-dir)))
    (unless (featurep 'verilog-mode)
      (load (if (file-exists-p vm-elc) vm-elc vm-el) nil t))

    ;; 1.1 ANSI Port List Tabular Alignment
    (with-temp-buffer
      (verilog-mode)
      (setq-local verilog-align-decl-dimension t)
      (setq-local verilog-align-decl-sub-dimension t)
      (insert "module my_unit (\n"
              "    input logic clk,\n"
              "    input idu_opcode_e [3:0] opcode_i,\n"
              "    output logic [NSvdq-1:0] [63:0] payload_o\n"
              ");\n"
              "endmodule\n")
      (goto-char (point-min))
      (search-forward "input logic clk")
      (electric-verilog-tab)
      (should (equal (buffer-string)
                     (concat "module my_unit (\n"
                             "    input  logic                           clk,\n"
                             "    input  idu_opcode_e             [3:0]  opcode_i,\n"
                             "    output logic        [NSvdq-1:0] [63:0] payload_o\n"
                             ");\n"
                             "endmodule\n"))))

    ;; 1.2 Packed Struct and Union Member Alignment
    (with-temp-buffer
      (verilog-mode)
      (setq-local verilog-align-decl-dimension t)
      (insert "typedef struct packed {\n"
              "    logic [3:0] tag;\n"
              "    idu_opcode_e [10:0] opcode;\n"
              "    logic valid;\n"
              "} idu_instr_pkt_t;\n")
      (goto-char (point-min))
      (search-forward "logic [3:0] tag")
      (electric-verilog-tab)
      (should (equal (buffer-string)
                     (concat "typedef struct packed {\n"
                             "    logic        [3:0]  tag;\n"
                             "    idu_opcode_e [10:0] opcode;\n"
                             "    logic               valid;\n"
                             "} idu_instr_pkt_t;\n"))))

    ;; 1.3 Module Instantiation Port Connection Alignment
    (with-temp-buffer
      (verilog-mode)
      (setq-local verilog-align-decl-dimension t)
      (insert "my_submodule u_sub (\n"
              "    .clk(sys_clk),\n"
              "    .rst_n(sys_rst_n),\n"
              "    .data_in_dp(wide_data_bus[31:0]),\n"
              "    .ready_o(sub_ready)\n"
              ");\n")
      (goto-char (point-min))
      (search-forward ".clk")
      (electric-verilog-tab)
      (should (equal (buffer-string)
                     (concat "my_submodule u_sub (\n"
                             "    .clk        (sys_clk),\n"
                             "    .rst_n      (sys_rst_n),\n"
                             "    .data_in_dp (wide_data_bus[31:0]),\n"
                             "    .ready_o    (sub_ready)\n"
                             ");\n"))))

    ;; 1.4a Data Type & Combined Packed Dimension Alignment (`verilog-align-decl-dimension' = t, sub = nil)
    (with-temp-buffer
      (verilog-mode)
      (setq-local verilog-align-decl-dimension t)
      (setq-local verilog-align-decl-sub-dimension nil)
      (insert "\tlogic  [3:0] [NSvdqEnqXuops-1:0] frontend_svdq_vld_dp;\n"
              "    idu_svdq_pkt_t [NSvdqEnqXuops-1:0] frontend_svdq_pkt_dp;\n"
              "    logic  [1:0][3:0]      [NSvdqEnqXuops-1:0] svdq_frontend_rdy_dp;\n"
              "    logic   svdq_frontend_rdy_dp2;\n")
      (goto-char (point-min))
      (electric-verilog-tab)
      (should (equal (buffer-string)
                     (concat "    logic          [3:0][NSvdqEnqXuops-1:0]      frontend_svdq_vld_dp;\n"
                             "    idu_svdq_pkt_t [NSvdqEnqXuops-1:0]           frontend_svdq_pkt_dp;\n"
                             "    logic          [1:0][3:0][NSvdqEnqXuops-1:0] svdq_frontend_rdy_dp;\n"
                             "    logic                                        svdq_frontend_rdy_dp2;\n"))))

    ;; 1.4b Data Type & Sub-Dimension Alignment (`verilog-align-decl-dimension' = t, sub = t)
    (with-temp-buffer
      (verilog-mode)
      (setq-local verilog-align-decl-dimension t)
      (setq-local verilog-align-decl-sub-dimension t)
      (insert "\tlogic  [3:0] [NSvdqEnqXuops-1:0] frontend_svdq_vld_dp;\n"
              "    idu_svdq_pkt_t [NSvdqEnqXuops-1:0] frontend_svdq_pkt_dp;\n"
              "    logic  [1:0][3:0]      [NSvdqEnqXuops-1:0] svdq_frontend_rdy_dp;\n"
              "    logic   svdq_frontend_rdy_dp2;\n")
      (goto-char (point-min))
      (electric-verilog-tab)
      (should (equal (buffer-string)
                     (concat "    logic                [3:0] [NSvdqEnqXuops-1:0] frontend_svdq_vld_dp;\n"
                             "    idu_svdq_pkt_t             [NSvdqEnqXuops-1:0] frontend_svdq_pkt_dp;\n"
                             "    logic          [1:0] [3:0] [NSvdqEnqXuops-1:0] svdq_frontend_rdy_dp;\n"
                             "    logic                                          svdq_frontend_rdy_dp2;\n"))))

    ;; 1.4c Verify `verilog-align-decl-sub-dimension' is ignored when `verilog-align-decl-dimension' is nil
    (with-temp-buffer
      (verilog-mode)
      (setq-local verilog-align-decl-dimension nil)
      (setq-local verilog-align-decl-sub-dimension t)
      (setq-local verilog-auto-lineup-on-semi nil)
      (insert "    logic [3:0] [7:0] sig_a;\n"
              "    idu_svdq_pkt_t [7:0] sig_b;\n")
      (goto-char (point-min))
      (electric-verilog-tab)
      ;; Sub-dimensions [3:0] and [7:0] must NOT be split into separate sub-columns when verilog-align-decl-dimension is nil
      (should (string-match-p "\\[3:0\\]\\[7:0\\] sig_a;" (buffer-string))))

    ;; 1.5 Verify `verilog-auto-newline' = nil and `verilog-auto-lineup-on-semi' = nil
    (with-temp-buffer
      (verilog-mode)
      (setq-local verilog-auto-newline nil)
      (setq-local verilog-auto-lineup-on-semi nil)
      (setq-local verilog-align-decl-dimension t)
      (insert "module m;\n"
              "   logic [7:0] first_sig;\n"
              "   logic second_sig")
      (let ((last-command-event ?\;))
        (electric-verilog-semi))
      ;; Should not have inserted a newline after ';', and should not have aligned `second_sig' yet
      (should (looking-back "logic second_sig;" (- (point) 20)))
      ;; Now pressing TAB kicks off alignment of the declaration block
      (electric-verilog-tab)
      (should (string-match-p "logic \\[7:0\\] first_sig;\n   logic       second_sig;" (buffer-string))))

    ;; 1.6 Verify declaration inside nested scope (`function') indents relative to enclosing scope
    (with-temp-buffer
      (let ((indent-tabs-mode nil)
            (verilog-indent-lists nil)
            (verilog-indent-level 4)
            (verilog-indent-level-module 4)
            (verilog-indent-level-declaration 4)
            (verilog-indent-level-behavioral 4)
            (verilog-cexp-indent 4)
            (verilog-case-indent 4)
            (verilog-align-decl-dimension t)
            (verilog-align-decl-sub-dimension t)
            (verilog-auto-lineup-on-semi nil))
        (verilog-mode)
        (setq-local indent-tabs-mode nil)
        (insert "module foo;\n"
                "    function automatic logic fn_check_thermal (\n"
                "        input [3:0] in\n"
                "    );\n"
                "    logic [3:0] in_p1;\n"
                "        in_p1 = in + 1'b1;\n"
                "        return (in & in_p1) == 4'b0000;\n"
                "    endfunction\n"
                "endmodule\n")
        (indent-region (point-min) (point-max) nil)
        (should (string-match-p "^        logic \\[3:0\\] in_p1;$" (buffer-string)))))))

(provide 'test-ts)
;;; test-ts.el ends here

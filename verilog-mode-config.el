;;; verilog-mode-config.el --- Custom settings for verilog-mode -*- lexical-binding: t; -*-

;; Disable list-style indentation:
;; When nil, verilog-mode will not align lines inside parentheses to the column
;; after '(', but will instead use standard block indentation.
;; This formats assertions, property blocks, and port lists cleanly:
;;
;;   ast_idu_arb_req_done_pulse: assert property (
;;       idu_biu__smu_arb_req_done |=> !idu_biu__smu_arb_req_done
;;   );
(setq verilog-indent-lists nil)

;; Indentation width (adjust to 4 or 3 as desired)
(setq verilog-indent-level 4)
(setq verilog-indent-level-module 4)
(setq verilog-indent-level-declaration 4)
(setq verilog-indent-level-behavioral 4)
(setq verilog-cexp-indent 4)
(setq verilog-case-indent 4)

;; Indent with spaces rather than tabs
(setq-default indent-tabs-mode nil)

;; Enable packed dimension alignment across declaration blocks
(setq verilog-align-decl-dimension t)
;; Enable sub-dimensions aligned separately
(setq verilog-align-decl-sub-dimension t)

;; Semicolon behavior:
;; 1. Do not automatically insert a newline when typing ';'
(setq verilog-auto-newline nil)
;; 2. Do not auto-align declarations when typing ';'; kick off alignment by pressing <TAB>
(setq verilog-auto-lineup-on-semi nil)


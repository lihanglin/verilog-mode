;; DESCRIPTION: Common Emacs flags needed for batch_test.pl
;;
;; Copyright 2008-2024 by Michael McNamara and Wilson Snyder.  This package
;; is free software; you can redistribute it and/or modify it under the
;; terms of either the GNU Lesser General Public License or the Perl
;; Artistic License.

(setq verilog-library-flags "-I. -I../../tests")

(defun verilog-test-new-buffer ()
  "Format or simulate interactive `;'/<TAB> in current buffer for `tests_new/'."
  (verilog-mode)
  (verilog-auto-reeval-locals t)
  (goto-char (point-min))
  (if (save-excursion (re-search-forward "/\\*\\(SEMI\\|TAB\\)\\*/" nil t))
      (while (re-search-forward "/\\*\\(SEMI\\|TAB\\)\\*/" nil t)
        (let ((act (match-string 1)))
          (replace-match "" t t)
          (if (string= act "SEMI")
              (let ((last-command-event ?\;))
                (electric-verilog-semi))
            (electric-verilog-tab))))
    (indent-region (point-min) (point-max) nil)))

(defun verilog-batch-test-new ()
  "Batch test runner for Phase 3 new features in `tests_new/'."
  (verilog-batch-execute-func 'verilog-test-new-buffer))

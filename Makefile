# DESCRIPTION: Run verilog-mode tests, as part of "make test"
#
# Copyright 2008-2024 by Michael McNamara and Wilson Snyder.  This package
# is free software; you can redistribute it and/or modify it under the
# terms of either the GNU Lesser General Public License or the Perl
# Artistic License.
# 
######################################################################
# Common targets:
#   make             # Compile C++17 Tree-Sitter module and GNU Emacs elisp
#   make emacs       # Compile source for GNU Emacs
#   make test        # Run self tests for GNU Emacs and Tree-Sitter
#   make test_emacs  # Run golden self tests for GNU Emacs
#   make test_ts     # Run Tree-Sitter ERT self tests
######################################################################

# the directory where the .elc files will be installed
ifneq ($(wildcard $(abspath ../opt/bin/emacs-26.1)),)
EMACS   ?= $(abspath ../opt/bin/emacs-26.1)
else
EMACS   ?= emacs
endif
EMACS_DEST = /usr/share/emacs/site-lisp/
ELC	= -batch -q -f batch-byte-compile
MAKECHANGELOG = perl makechangelog
export EMACS

default : src/verilog-ts-core.so e/verilog-mode.elc
emacs : src/verilog-ts-core.so e/verilog-mode.elc
compile : src/verilog-ts-core.so e/verilog-mode.elc

.PHONY: ts_module test_ts
ts_module: src/verilog-ts-core.so
src/verilog-ts-core.so: $(wildcard src/*.cpp src/*.hpp src/Makefile)
	$(MAKE) -C src all

test_ts: src/verilog-ts-core.so e/verilog-mode.elc
	$(EMACS) --batch -Q -l ../phase1_exp/test-ts.el -f ert-run-tests-batch-and-exit

release : .timestamps
install : .timestamps
	@echo "Installation not supported by this makefile"
	false

.timestamps:
	@mkdir -p $@

test:	.timestamps/test
.timestamps/test: src/verilog-ts-core.so e/verilog-mode.elc verilog.info 0test.el .timestamps
ifeq ($(VERILOG_MODE_TEST_FILE),)
	$(MAKE) test_ts test_batch test_errors test_emacs
	@touch $@
	@echo ======= ALL TESTS PASSED
else
	$(MAKE) test_emacs
endif

#Usage: $(call test-emacs_sub,label,threading)
define test_emacs_sub
test_emacs:: $(1)
$(1): e/verilog-mode.elc
	@echo
	@echo == $(1)
	@mkdir -p e/t
	VERILOG_MODE_THREAD=$(2) time $(EMACS)  --batch -q --no-site-file -l e/verilog-mode.elc -l 0test.el
endef

$(eval $(call test_emacs_sub,test_emacs_1,1of5))
ifeq ($(VERILOG_MODE_TEST_FILE),)
$(eval $(call test_emacs_sub,test_emacs_2,2of5))
$(eval $(call test_emacs_sub,test_emacs_3,3of5))
$(eval $(call test_emacs_sub,test_emacs_4,4of5))
$(eval $(call test_emacs_sub,test_emacs_5,5of5))
endif

test_errors:
	@echo
	@echo == test_errors
	-# The multiline errors must be in one read()s output or the comint may not match it
	-cat error_msgs.out

test_batch: e/verilog-mode.elc
	@echo == test_batch
	mkdir -p e/b
	time ./batch_test.pl

local:	.timestamps/local
.timestamps/local:  verilog-mode.el
	cp verilog-mode.el $(EMACS_DEST)verilog-mode.el
	rm -f $(EMACS_DEST)verilog-mode.elc
	$(EMACS) $(ELC) $(EMACS_DEST)verilog-mode.el
	@touch $@

ChangeLog.txt emacs-version.h : make_log.pl verilog-mode.el
ifneq ($(VERILOGMODE_SKIP_MAKELOG),1)
	./make_log.pl	
endif

ChangeLog : verilog-mode.el makechangelog
	$(MAKECHANGELOG) --git verilog-mode.el > $@

e/verilog-mode.el : verilog-mode.el ./config_rev.pl
	-rm -rf e
	-mkdir e
	./config_rev.pl . . <verilog-mode.el >e/verilog-mode.el

e/verilog-mode.el.gz : e/verilog-mode.el
	gzip --best $< --stdout > $@

e/verilog-mode.elc : e/verilog-mode.el src/verilog-ts-core.so
	$(EMACS) $(ELC) e/verilog-mode.el

verilog.info : verilog.texinfo
	makeinfo verilog.texinfo > verilog.info

######################################################################
# GNU GIT version

gnutrunk:
	@echo "gnutrunk needs to be a symlink to a emacs/trunk git checkout"
	false

.PHONY: gnu-update gnu-update-trunk
gnu-update: gnu-update-trunk
gnu-update-trunk: gnutrunk
	echo "NOT DOING: cd gnutrunk && git pull"

.PHONY: gnu-diff-trunk gnu-diff
gnu-diff: gnu-diff-trunk
gnu-diff-trunk: gnu-update-trunk verilog-mode-tognu.el
	diff -u gnutrunk/lisp/progmodes/verilog-mode.el verilog-mode-tognu.el 
verilog-mode.patch: gnu-update verilog-mode-tognu.el
	diff -u gnutrunk/lisp/progmodes/verilog-mode.el verilog-mode-tognu.el > $@
# Before install, clean to make sure we get proper git ID
gnu-cp:
	$(MAKE) clean
	$(MAKE) verilog-mode-tognu.el
	cp verilog-mode-tognu.el gnutrunk/lisp/progmodes/verilog-mode.el

verilog-mode-tognu.el: e/verilog-mode.el Makefile
	cat e/verilog-mode.el \
	 | sed 's/verilog-mode-version "\(.*\)"/verilog-mode-version "\1-GNU"/g' \
	 | sed 's/verilog-mode-release-emacs nil/verilog-mode-release-emacs t/g' \
	 > verilog-mode-tognu.el

######################################################################
# Clean

clean::
	make -C src clean
	make -C tree-sitter clean
	make -C tree-sitter-systemverilog clean
	/bin/rm -rf .timestamps e x test_dir verilog-mode.patch verilog-mode-tognu.el *.info

# Makefile for verilog-ts-core.so (Emacs 26.1+ Dynamic Module, C++17)

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -fPIC -Wall -Wextra -Wno-unused-parameter

TOP_DIR     := $(abspath ../..)
EMACS_INC   := $(TOP_DIR)/emacs/src
TS_DIR      := $(TOP_DIR)/tree-sitter
TSSV_DIR    := $(TOP_DIR)/tree-sitter-systemverilog

INCLUDES := \
	-I. \
	-I$(EMACS_INC) \
	-I$(TS_DIR)/lib/include \
	-I$(TSSV_DIR)/bindings/c \
	-I$(TSSV_DIR)/src

TS_LIB   := $(TS_DIR)/libtree-sitter.a
TSSV_LIB := $(TSSV_DIR)/libtree-sitter-systemverilog.a

SRCS := \
	verilog_ts_core.cpp \
	verilog_ts_scan.cpp \
	verilog_ts_nav.cpp \
	verilog_ts_indent.cpp \
	verilog_ts_decls.cpp

OBJS := $(SRCS:.cpp=.o)
TARGET := verilog-ts-core.so

.PHONY: all clean

all: $(TARGET)

$(TS_LIB):
	$(MAKE) -C $(TS_DIR) CFLAGS="-O3 -Wall -Wextra -std=c11 -fPIC" libtree-sitter.a

$(TSSV_LIB):
	$(MAKE) -C $(TSSV_DIR) CFLAGS="-O3 -Wall -Wextra -std=c11 -fPIC" libtree-sitter-systemverilog.a

%.o: %.cpp verilog_ts_common.hpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(TARGET): $(OBJS) $(TS_LIB) $(TSSV_LIB)
	$(CXX) $(CXXFLAGS) -shared -o $@ $(OBJS) $(TSSV_LIB) $(TS_LIB)

clean:
	$(RM) $(OBJS) $(TARGET)

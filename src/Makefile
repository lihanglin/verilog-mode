# Makefile for verilog-ts-core.so (Emacs 26.1+ Dynamic Module, C++17)
CC          ?= gcc
CFLAGS      ?= -O2 -Wall -Wextra -std=gnu11 -fPIC
CXX         ?= g++
CXXFLAGS    ?= -std=gnu++17 -O2 -fPIC -Wall -Wextra -Wno-unused-parameter

TOP_DIR     := $(abspath ..)
EMACS_INC   := $(TOP_DIR)/src/include/emacs/v26.1
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
	$(MAKE) -C $(TS_DIR) CC="$(CC)" CFLAGS="$(CFLAGS)" libtree-sitter.a

$(TSSV_LIB):
	$(MAKE) -C $(TSSV_DIR) CC="$(CC)" CFLAGS="$(CFLAGS)" libtree-sitter-systemverilog.a

%.o: %.cpp verilog_ts_common.hpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(TARGET): $(TS_LIB) $(TSSV_LIB) $(OBJS)
	$(CXX) $(CXXFLAGS) -shared -o $@ $(OBJS) $(TSSV_LIB) $(TS_LIB)

clean:
	$(RM) $(OBJS) $(TARGET)

OUT_SYS_DIR := scons-out/opt-linux-x86-64/staging
OUT_RYOAN_DIR := scons-out/opt-bivy-x86-64/staging
FILES := ./SConstruct $(shell find ./src -name '*.c' -o -name '*.h' -o -name '*.cpp')
J:=1 # set J in environment for parallel build

export RYOAN_BUILD_PATH=../root

all: ropt rdbg opt dbg
.PHONY: all

ropt: scons-out/opt-ryoan-x86-64/staging/sel_ldr
.PHONY: ropt

opt: scons-out/opt-linux-x86-64/staging/sel_ldr
.PHONY: opt

dbg: scons-out/dbg-linux-x86-64/staging/sel_ldr
.PHONY: dbg

rdbg: scons-out/dbg-ryoan-x86-64/staging/sel_ldr
.PHONY: rdbg

scons-out/%-x86-64/staging/sel_ldr: $(FILES)
	./scons MODE=$*,nacl platform=x86_64 -j$(J)

scons-out/%-ryoan-x86-64/staging/sel_ldr: libc_exists $(FILES)
	./scons MODE=$*-ryoan,nacl platform=x86_64 -j$(J)

clean:
	rm -rf scons-out

# Use zig as my build compiler. If you want to use it everywhere export it.
CC=clang
CXX=clang++


# These warnings only exist in clang (gcc/musl-gcc reject them as unknown options)
CLANG_WARN := $(shell $(CC) --version 2>/dev/null | grep -qi clang && echo -Wmacro-redefined -Wempty-translation-unit)
CFLAGS  := -std=c99 -Wall -Wextra -Wpedantic $(CLANG_WARN) -O2 -g -O0
LDFLAGS := -static -lyaml
INCLUDES:= -Iinclude

# Where a libyaml built for the libc you are linking against lives, ie. the musl
# one from tools/build-libyaml-musl.sh. Leave it empty to use the system's.
YAML_PREFIX ?=
ifneq ($(YAML_PREFIX),)
INCLUDES += -I$(YAML_PREFIX)/include
LDFLAGS  := -static -L$(YAML_PREFIX)/lib -lyaml
endif

SRC := $(wildcard src/*.c)
BIN := bin/mylfs

all: $(BIN)


# make musl YAML_PREFIX=$PWD/deps/libyaml
# A fully static musl binary from any host that has zig. It runs on the glibc LFS
# system and on a musl one. Use a native musl toolchain instead with:
#   make CC=musl-gcc YAML_PREFIX=/usr/local/musl
MUSL_TARGET ?= x86_64-linux-musl
musl:
	$(MAKE) clean
	$(MAKE) all CC="zig cc -target $(MUSL_TARGET)"

# prints "not a dynamic executable" when the binary really is static
check: $(BIN)
	@file $(BIN)
	@! ldd $(BIN) >/dev/null 2>&1 || (echo "$(BIN) is NOT static" && false)

.PHONY: all clean musl check

$(BIN): $(SRC) | bin
	$(CC) $(CFLAGS) $(INCLUDES) $(SRC) -o $@ $(LDFLAGS)


bin:
	mkdir -p bin

run:
	bin/main

clean:
	rm -rfv bin/*

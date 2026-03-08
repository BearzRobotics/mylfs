# Use zig as my build compiler. If you want to use it everywhere export it.
CC=clang
CXX=clang++


FLAGS  := -std=c99 -Wall -Wextra -Wpedantic -Wmacro-redefined -Wempty-translation-unit -O2 -g -Wnopointer-integer-compare
LDFLAGS := -static -lyaml
INCLUDES:= -Iinclude

SRC := $(wildcard src/*.c)
BIN := bin/mylfs

all: $(BIN)


.PHONY: all clean

$(BIN): $(SRC) | bin
	$(CC) $(CFLAGS) $(INCLUDES) $(SRC) -o $@ $(LDFLAGS)

bin:
	mkdir -p bin

run:
	bin/main

clean:
	rm -rfv bin/*

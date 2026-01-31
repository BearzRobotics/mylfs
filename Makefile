CC      := zig cc #clang # musl-clang
CFLAGS  := -std=c99 -Wall -Wextra -Wpedantic -Wempty-translation-unit -O2 -g 
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

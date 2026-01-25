CC      := clang # musl-clang
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -O2 -g -Ivendor/libyaml/include
LDFLAGS := -static
INCLUDES:= -Ilib

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

# mylfs

mylfs builds a [Linux From Scratch](https://www.linuxfromscratch.org/) system
from a directory of recipes, then keeps going and builds any extra packages you
add on top. It is written in C and is inspired by
[MyLFS](https://github.com/TheKingKerellos/MyLFS). I forked that to my GitHub
and used it to build a working X Windows stack.

* [doc/mylfs.md](doc/mylfs.md) - command line options and config
* [doc/phases.md](doc/phases.md) - what each of the 5 phases does
* [doc/recipes.md](doc/recipes.md) - how recipes are laid out
* [doc/template.md](doc/template.md) - every `template.yml` key
* [doc/installation.md](doc/installation.md) - getting the result onto a disk or
  image and booting it

## Recipe sets

| Set | What it is |
|-----|------------|
| `recipes/LFS13_1` | LFS 13.1 (systemd). The default in config.yml. |
| `recipes/LFS12_3` | LFS 12.3 |
| `dkrecipes` | Batteries-included distro: everything I or my uncle might want to use. A huge build, around 35G. |
| `cmrecipes` | LFS with musl, clang as a secondary compiler and dinit |

Select one with `recipes_path` in config.yml or `--recipes-path`.

Notes on cmrecipes: musl's documentation is a bit thin. This page links to two
standards: https://musl.libc.org/doc/1.1.24/manual.txt. I downloaded them and
copy them to `/usr/share/doc/musl/` (one PDF, one HTML, plus plain text copies
of both). The musl patches come from
https://github.com/richfelker/musl-cross-make.

## Requirements

Build mylfs:

* GNU make
* libyaml
* a working C compiler

Run mylfs (on the build host):

* rsync, tar, unzip, curl
* GNU make, autotools, coreutils
* qemu, only if you want to try the result in a VM

## Build

The Makefile defaults to `clang`; gcc works too (`make CC=gcc`).

```sh
make
sudo ./mylfs
```

Run it from the directory that holds `config.yml`, `recipes/` and `logs/`. Paths
in config.yml are relative to where you run it.

### Static / musl build
mylfs should be a fully static binary so it runs on the build host, on the
system it builds, and on a musl one. It never calls `getpwnam()`/NSS (it reads
`/etc/passwd` itself), so it doesn't depend on the libc it was built against.
With zig installed:

```sh
tools/build-libyaml-musl.sh            # static libyaml against musl -> deps/libyaml
make musl YAML_PREFIX=$PWD/deps/libyaml
make check                             # confirms bin/mylfs is static
```

Or with a native musl toolchain: `make CC=musl-gcc YAML_PREFIX=/usr/local/musl`.

On the built system the `musl`, `libyaml` and `mylfs` phase 5 recipes build it
natively with musl-gcc.

## Using it

```sh
sudo ./mylfs                       # all 5 phases
sudo ./mylfs --bootstrap           # phases 1-4 only (base system)
sudo ./mylfs --start-phase 5       # resume at a phase
sudo ./mylfs --start-phase 5 --start-package nwipe
```

If a build crashes, restart that phase with `--start-phase <number>`. This is
handy in phase 5 after you add packages to your recipes directory. Anything
already built is recorded in `recipes/build_state.db` in the build dir and shown
as `[SKIP]`. A package whose dependency failed, or has no recipe, is skipped as
well.

To see what is going on inside a build:

```sh
ps aux | grep mylfs
```

and read the files in `logs/`. All options are in [doc/mylfs.md](doc/mylfs.md).

### Example output
This is from the Python edition, the C edition prints the same kind of
thing.

```
$ ./mylfs --start-phase 5 --start-package nwipe

Phase 5 - Building final system
========================================
    [SKIP] make                              already built
    [SKIP] glibc                             already built
    [SKIP] ncurses                           already built
    [SKIP] libpcap                           already built
    [SKIP] parted                            already built
    [SKIP] nwipe                             already built
```

## Downloads

* Packages from git.kernel.org may fail to download because of their bot
  protection. Download the file in a browser, put it in the recipe's directory,
  and mylfs uses it as if it had already downloaded it.
* Some packages fetch things from the internet while building. If they fail,
  restarting usually fixes it.

## Why C?
I feel like in this day and age I need to put this here so people don't ask why
it's not Rust. First, I don't like programming in Rust, it gives me a major
migraine. Preferably I would choose Zig, but two factors are against it. My
primary user prefers C, which is the driving factor. And with Zig's IO changes
changing the way programs are written, it's not worth putting a lot of effort
into Zig until that settles.

## Things I hate about C
* glibc is a nightmare to program with. Unix features only work with
  `_GNU_SOURCE`, even though there are other options like `_POSIX_C_SOURCE`.
* Scoping: when including `dprint.h` for my print wrappers, or any other header,
  I can call `passed()` with no reference to where it's from. What if glibc or
  another dependency made a function named `passed()`?
* short, int, long, long long, double, etc.
* Header guards so the compiler only includes one copy of a header.
* No proper build system.
* Prototyping: functions must be declared in .h files before use.
* No built-in bool. `stdbool.h` shouldn't be required.
* Errors as return numbers.
* C switch statements are ugly compared to Zig's switch or Rust's match.

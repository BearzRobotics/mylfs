#!/bin/sh
# Builds a static libyaml against musl with zig into deps/libyaml, for:
#   make musl YAML_PREFIX=$PWD/deps/libyaml
set -eu

VERSION=0.2.5
ROOT=$(cd "$(dirname "$0")/.." && pwd)
WORK=$ROOT/deps/build
PREFIX=$ROOT/deps/libyaml
TARGET=${MUSL_TARGET:-x86_64-linux-musl}

mkdir -p "$WORK"
cd "$WORK"
[ -f yaml-$VERSION.tar.gz ] || curl -fsSL -O https://pyyaml.org/download/libyaml/yaml-$VERSION.tar.gz
rm -rf yaml-$VERSION
tar -xf yaml-$VERSION.tar.gz
cd yaml-$VERSION

CC="zig cc -target $TARGET" AR="zig ar" RANLIB="zig ranlib" \
    ./configure --prefix="$PREFIX" --disable-shared --enable-static
make -j"$(nproc)"
make install
echo "libyaml installed in $PREFIX"

#!/bin/sh
# Packs the build into dist/braincurses-<target>.tar.gz (.zip for Windows)
# for av: the binary at the top, with README.md, LICENSE and PATCHES.md.
#
#   asciiverse/package.sh x86_64-unknown-linux-musl
set -eu
cd "$(dirname "$0")/.."
target="$1"
case "$target" in *windows*) exe=.exe ;; *) exe= ;; esac
stage=stage/braincurses
rm -rf "$stage" && mkdir -p "$stage" dist
cp "out/braincurses$exe" README.md LICENSE PATCHES.md "$stage/"
if [ -n "$exe" ]; then
  (cd "$stage" && zip -qr "../../dist/braincurses-$target.zip" .)
else
  tar -czf "dist/braincurses-$target.tar.gz" -C "$stage" .
fi
ls dist

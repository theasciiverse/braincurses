#!/bin/sh
# Builds braincurses into out/. The compiler and curses library come from
# the environment, so one script serves every target (see
# .github/workflows/release.yml):
#
#   asciiverse/build.sh                                          # Linux
#   LIBS="-lncursesw" LDFLAGS="-static -s" asciiverse/build.sh   # Alpine, static
#   CXXFLAGS="-O2 -arch arm64 -D_XOPEN_SOURCE_EXTENDED" LDFLAGS="-arch arm64" \
#     LIBS="-lncurses" asciiverse/build.sh                       # macOS
#   CXX=x86_64-w64-mingw32-g++ CXXFLAGS="-O2 -I$PDC -DPDC_WIDE -DPDC_FORCE_UTF8" \
#     LIBS="$PDC/wincon/pdcurses.a" LDFLAGS="-static -s" EXE=.exe asciiverse/build.sh
set -eu
cd "$(dirname "$0")/.."
CXX="${CXX:-c++}"
CXXFLAGS="${CXXFLAGS:--O2}"
LDFLAGS="${LDFLAGS:-}"
LIBS="${LIBS:-$(pkg-config --libs ncursesw 2>/dev/null || echo -lncursesw)}"
EXE="${EXE:-}"
mkdir -p out
rm -f "out/braincurses$EXE"
# shellcheck disable=SC2086
nice -n 10 $CXX -std=c++11 -Wall $CXXFLAGS main.cpp code.cpp braincurses.cpp $LDFLAGS $LIBS -o "out/braincurses$EXE"
echo "built braincurses$EXE"

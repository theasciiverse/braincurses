# Changes from upstream

This is the Asciiverse packaging of [braincurses](https://github.com/bderrly/braincurses)
by Brian Derr, a code-breaking game for the terminal (C++, ncurses; first
written in 2002, rewritten 2015 to 2016). Upstream is the `upstream`
remote; the `asciiverse` branch carries the changes below.

Built from upstream `master` at `fa2a67e` (2016-09-01), its last commit,
which is also tagged `1.1.0` (the `v1.1.0` tag is one README commit
earlier). The version here is 1.1.0.

## Licence

GPL-2.0-only. LICENSE is the text of the GNU GPL version 2, and the
source files carry only a copyright line ("Copyright © 2002–2015, Brian
Derr"), no licence notice; nothing in the repository or its history
offers "any later version" (the only such wording in the history is in
a removed LGPL test library, liblittletest). With no grant of later
versions, this packaging stays on version 2 only. GitHub reads the
licence as GPL-2.0.

## Debian

Not packaged by Debian (checked 2026-10-18).

## Code

- **A solved code of 5 or 6 colours shows all its markers.** An exact
  guess returned four markers whatever the code length, so with `-c 5`
  or `-c 6` the winning row was drawn from past the end of the marker
  list.
- **A guess is only complete when every field has a colour.** The right
  arrow could move past the last field and end the guess with empty
  fields, which were then scored and drawn as colour -1. It now moves
  only over fields that have a colour, and a guess with an empty field
  goes back to that field instead of ending.
- **Centred, and redrawn on resize.** The board was drawn at the top
  left. It is centred, and a resized terminal gets the board made again
  in the middle with every guess, its markers and the guess being typed;
  a terminal smaller than 60x23 is asked to grow (q quits) instead of
  `newwin()` failing.
- **Wide curses.** Linked with ncursesw after `setlocale()`, so the boxes
  are drawn in the terminal's own character set.
- **The result stays on screen.** The win or lose message was cleared at
  once by the play-again question; it now stays in the information
  panel, which also lists the keys. Backspace also works where the
  terminal sends DEL or ^H.
- **q quits; signals leave the terminal in order.** There was no way to
  leave a game except Ctrl+C, whose handler called `endwin()` and
  `exit()` inside the signal handler. q now quits, and SIGINT, SIGTERM
  and SIGHUP only set a flag that the next key read turns into a normal
  exit; the atexit handler, now registered before the screen opens,
  restores the terminal.
- **Windows build with MinGW and PDCurses**: PDCurses' `curses.h`,
  `std::signal()` where there is no `sigaction()` or SIGHUP, and
  `resize_term(0, 0)` when PDCurses reports a resize.

## Packaging (new files)

- `asciiverse/build.sh`: the three source files in one compiler call; the
  compiler and curses library come from the environment. Upstream's
  Makefile now links ncursesw too.
- `asciiverse/package.sh`: one archive per target with the binary,
  README.md, LICENSE and this file.
- `.github/workflows/release.yml`: static Linux in Alpine (x86_64,
  aarch64), macOS (x86_64, arm64) and Windows x86_64 (MinGW, wide
  PDCurses 3.9), plus SHA256SUMS, on our tags (`*-av.*`). Upstream's unit
  tests need the bandit and json submodules and are not built.
- `asciiverse/record-preview.py`: records `asciiverse/braincurses.cast`,
  games played by a code breaker that reads the markers from the screen
  and always guesses a code consistent with every answer.
- `.gitignore`: the build folders.

av starts braincurses with no arguments: four colours to find, ten
guesses. `-c 5` or `-c 6` makes the code longer, `-g` sets the number of
guesses (1 to 15).

## Tested

Linux: native (glibc) and static (Alpine 3.22, musl) builds; games won
and lost at 100x30 with codes of 4, 5 and 6, play again and quit at the
prompt; resized during a guess and at the prompt, down to 40x10 (the
too-small message) and back; q, Ctrl+C, SIGTERM and SIGHUP exit with
status 0 and the terminal restored. ASan and UBSan: a 90-second soak of
random keys and resizes with random code lengths and guess counts,
clean. Windows: cross-built with MinGW-w64 against wide PDCurses 3.9 and
run under Wine 10: colours typed, Backspace and arrows, a guess scored,
q quits with status 0 (Wine's console in tmux shows the box lines as
question marks). macOS: not built here.

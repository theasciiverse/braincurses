#!/usr/bin/env python3
"""Records asciiverse/braincurses.cast, the catalog preview, at 100x30
(hub/RECORDING.md): games played by a code breaker that reads the red and
white markers from the screen and always guesses a code that agrees with
every answer so far (the first guess is red red green green, after Knuth).
It plays again after each game, and after about 50 seconds it quits with q.

braincurses runs from out/ (asciiverse/build.sh) by name, so the cast's
header holds no local path.

Needs asciinema 3.x and pyte. Usage, from the repo root:
  uv run --with pyte python3 asciiverse/record-preview.py
Any extra argument is used as the cast's path instead (for test runs);
with --dry-run it plays without recording."""
import fcntl, itertools, os, pty, select, struct, sys, termios, time
import pyte

root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
args = [a for a in sys.argv[1:] if a != "--dry-run"]
dry = "--dry-run" in sys.argv
cast = os.path.abspath(args[0]) if args else f"{root}/asciiverse/braincurses.cast"
COLS, ROWS, PLAY = 100, 30, float(os.environ.get("PLAY", "50"))
LETTERS = "rgybmw"
# LENGTH=5 or 6 plays longer codes (dry runs, for testing)
LENGTH = int(os.environ.get("LENGTH", "4"))
POS = {4: (2, 6, 10, 14), 5: (2, 5, 8, 11, 14), 6: (3, 5, 7, 9, 11, 13)}[LENGTH]

env = dict(os.environ, TERM="xterm-256color", COLORTERM="truecolor", LANG="C.UTF-8",
           PATH=f"{root}/out:{os.environ['PATH']}")
for v in ("VTE_VERSION", "TERM_PROGRAM", "TERM_PROGRAM_VERSION", "TMUX"):
    env.pop(v, None)

pid, fd = pty.fork()
if pid == 0:
    if dry:
        os.execvpe("braincurses", ["braincurses", "-c", str(LENGTH)], env)
    os.execvpe("asciinema", ["asciinema", "rec", "--overwrite", "-q", "--idle-time-limit", "2",
        "--window-size", f"{COLS}x{ROWS}", "--capture-env", "TERM,COLORTERM,LANG",
        "--title", "braincurses", "--command", "braincurses", cast], env)
fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", ROWS, COLS, 0, 0))
screen = pyte.Screen(COLS, ROWS)
stream = pyte.ByteStream(screen)
start = time.time()


def pump(seconds):
    end = time.time() + seconds
    while True:
        left = end - time.time()
        r, _, _ = select.select([fd], [], [], max(0.0, min(0.02, left)))
        if r:
            try:
                stream.feed(os.read(fd, 65536))
            except OSError:
                return False
        if left <= 0:
            return True


def key(k, wait=0.3):
    os.write(fd, k)
    pump(wait)


def origin():
    """Top left corner of the board, from the title in the header box."""
    for y, line in enumerate(screen.display):
        x = line.find("BrainCurses")
        if x >= 0:
            return y - 1, x - 5
    return None


def markers(turn):
    """Red and white markers of a guess (turn 0 is the first), read from the
    marker box: red X for right colour and place, white X for colour only."""
    y0, x0 = origin()
    row = y0 + 3 + 15 - turn
    red = white = 0
    for x in POS:
        cell = screen.buffer[row][x0 + x]
        if cell.data == "X":
            if cell.fg == "red":
                red += 1
            else:
                white += 1
    return red, white


def score(guess, code):
    red = sum(g == c for g, c in zip(guess, code))
    common = sum(min(guess.count(ch), code.count(ch)) for ch in set(guess))
    return red, common - red


def text():
    return "\n".join(screen.display)


games = solved = 0
pump(1.5)
while time.time() - start < PLAY:
    candidates = ["".join(p) for p in itertools.product(LETTERS, repeat=LENGTH)]
    guess = ("rrgg" + "by")[:LENGTH]
    turn = 0
    while True:
        for ch in guess:
            key(ch.encode(), 0.25)
        pump(0.9)
        answer = markers(turn)
        turn += 1
        if answer == (LENGTH, 0):
            solved += 1
            break
        if "play again" in text():
            break
        candidates = [c for c in candidates if score(guess, c) == answer]
        if not candidates:
            print("no code agrees with the answers", file=sys.stderr)
            break
        guess = candidates[0]
    games += 1
    pump(3.0)
    if time.time() - start >= PLAY - 10:
        break
    key(b"y", 1.0)
key(b"q", 0.5)
end = time.time() + 5
while time.time() < end:
    if not pump(0.1):
        break
    done, _ = os.waitpid(pid, os.WNOHANG)
    if done:
        break
else:
    os.kill(pid, 9)
try:
    os.waitpid(pid, 0)
except ChildProcessError:
    pass
print(f"{cast}: {time.time() - start:.0f} s, {solved} of {games} games solved", file=sys.stderr)

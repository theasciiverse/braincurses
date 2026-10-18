/* braincurses.cpp
 * Copyright © 2002–2015, Brian Derr <brian@derrclan.com>
 */

#include "braincurses.h"

#include <algorithm>
#include <cstdlib>

#include "code.h"


// Maps the x-coordinates (within a WINDOW) for different values of code_length_.
const std::unordered_map<int, std::vector<int>> codePosition = {
    {4, {2, 6, 10, 14}},
    {5, {2, 5, 8, 11, 14}},
    {6, {3, 5, 7, 9, 11, 13}}
};


Braincurses::Braincurses(int code_length, int guesses) : 
    code_(Code(code_length)), guesses_(guesses), initialized_(false), code_shown_(false) {
  InitializeNcurses();
}

void Braincurses::InitializeNcurses() {
  initscr();

  if (has_colors() == FALSE) {
    // TODO: throw an exception?
  }

  start_color();
  if (COLORS < 8) {
    // TODO: throw an exception?
  }
  use_default_colors();

  // Only using the first eight color pairs.
  for (int i = 0; i < 8; i++) {
    if (i == 6) {
      // COLOR_CYAN is hard to distinguish from COLOR_BLUE; replace it with COLOR_WHITE.
      init_pair(i, i+1, -1);
    } else {
      init_pair(i, i, -1);
    }
  }

  cbreak();
  noecho();
  intrflush(stdscr, FALSE);
  keypad(stdscr, TRUE);
  curs_set(1);

  CreateWindows();

  initialized_ = true;
}

// Creates the windows centred in the terminal. A terminal smaller than the
// board gets a message instead, until it is made big enough.
void Braincurses::CreateWindows() {
  DeleteWindows();

  while (LINES < kBoardHeight || COLS < kBoardWidth) {
    erase();
    mvaddstr(0, 0, "Terminal too small:");
    mvaddstr(1, 0, "BrainCurses needs 60x23.");
    mvaddstr(2, 0, "q quits.");
    refresh();
    int key = GetKey(stdscr);
    if (key == 'q') {
      exit(EXIT_SUCCESS);
    }
  }
  erase();
  refresh();

  int y = (LINES - kBoardHeight) / 2;
  int x = (COLS - kBoardWidth) / 2;

  windows_.emplace(kHeaderWindow, CreateWindow(3, 21, y + 0, x + 0));
  windows_.emplace(kMarkerWindow, CreateWindow(17, 17, y + 3, x + 0));
  windows_.emplace(kGuessWindow, CreateWindow(17, 17, y + 3, x + 21));
  windows_.emplace(kCodeWindow, CreateWindow(3, 17, y + 0, x + 21));
  windows_.emplace(kWatermarkWindow, CreateWindow(17, 4, y + 3, x + 17));
  windows_.emplace(kInputWindow, CreateWindow(3, 60, y + 20, x + 0));
  windows_.emplace(kInfoWindow, CreateWindow(20, 22, y + 0, x + 38));

  for (auto kv : windows_) {
    keypad(kv.second, TRUE);
  }
}

void Braincurses::DeleteWindows() {
  for (auto kv : windows_) {
    delwin(kv.second);
  }
  windows_.clear();
}

// Reads a key; a resized terminal is answered here by drawing the board
// again in the middle.
int Braincurses::GetKey(WINDOW* window) {
  return wgetch(window);
}

// Draws everything that is on the board again, after the windows were made
// anew.
void Braincurses::Redraw() {
  PrepareBoard();
  int i = 0;
  for (auto& turn : history_) {
    DisplayGuess(i, turn.first);
    DisplayMarkers(i, turn.second);
    i++;
  }
  DisplayCode(code_shown_);
}

void Braincurses::Resize() {
  CreateWindows();
  Redraw();
}

void Braincurses::PrepareBoard() {
  mvwaddstr(windows_[kHeaderWindow], 1, 5, kGameName.c_str());
  mvwaddstr(windows_[kInfoWindow], 1, 2, "Colors: ");
  wattron(windows_[kInfoWindow], A_BOLD);

  wattron(windows_[kInfoWindow], COLOR_PAIR(COLOR_RED));
  mvwaddstr(windows_[kInfoWindow], 2, 2, "RED");
  wattroff(windows_[kInfoWindow], COLOR_PAIR(COLOR_RED));

  waddstr(windows_[kInfoWindow], ", ");

  wattron(windows_[kInfoWindow], COLOR_PAIR(COLOR_BLUE));
  waddstr(windows_[kInfoWindow], "BLUE");
  wattroff(windows_[kInfoWindow], COLOR_PAIR(COLOR_BLUE));

  waddstr(windows_[kInfoWindow], ", ");

  mvwaddstr(windows_[kInfoWindow], 3, 2, "WHITE, ");

  wattron(windows_[kInfoWindow], COLOR_PAIR(COLOR_GREEN));
  waddstr(windows_[kInfoWindow], "GREEN");
  wattroff(windows_[kInfoWindow], COLOR_PAIR(COLOR_GREEN));

  waddstr(windows_[kInfoWindow], ", ");

  wattron(windows_[kInfoWindow], COLOR_PAIR(COLOR_YELLOW));
  mvwaddstr(windows_[kInfoWindow], 4, 2, "YELLOW");
  wattroff(windows_[kInfoWindow], COLOR_PAIR(COLOR_YELLOW));

  waddstr(windows_[kInfoWindow], ", ");

  wattron(windows_[kInfoWindow], COLOR_PAIR(COLOR_MAGENTA));
  waddstr(windows_[kInfoWindow], "MAGENTA");
  wattroff(windows_[kInfoWindow], COLOR_PAIR(COLOR_MAGENTA));

  wattroff(windows_[kInfoWindow], A_BOLD);

  mvwaddstr(windows_[kInfoWindow], 6, 2, "Type the first");
  mvwaddstr(windows_[kInfoWindow], 7, 2, "letter of a color.");
  mvwaddstr(windows_[kInfoWindow], 8, 2, "Arrows, Backspace:");
  mvwaddstr(windows_[kInfoWindow], 9, 2, "change the guess.");
  mvwaddstr(windows_[kInfoWindow], 10, 2, "q: quit");

  // The result of the last game, if there is one.
  size_t start = 0;
  int row = 13;
  while (start < message_.size()) {
    size_t end = message_.find('\n', start);
    if (end == std::string::npos) {
      end = message_.size();
    }
    mvwaddstr(windows_[kInfoWindow], row++, 2, message_.substr(start, end - start).c_str());
    start = end + 1;
  }

#ifdef DEBUG
  DisplayCode(true);
#else
  DisplayCode(false);
#endif

  char guessLabel[12];
  for (int i = 1; i <= guesses_; i++) {
    snprintf(guessLabel, sizeof(guessLabel), "%2d", i);
    mvwaddstr(windows_[kWatermarkWindow], 16 - i, 1, guessLabel);
  }

  wmove(windows_[kInputWindow], 1, 15);
  wnoutrefresh(windows_[kHeaderWindow]);
  wnoutrefresh(windows_[kCodeWindow]);
  wnoutrefresh(windows_[kInfoWindow]);
  wnoutrefresh(windows_[kWatermarkWindow]);
  wnoutrefresh(windows_[kMarkerWindow]);
  wnoutrefresh(windows_[kGuessWindow]);
  wnoutrefresh(windows_[kInputWindow]);
  doupdate();
}

WINDOW* Braincurses::CreateWindow(int height, int width, int starty, int startx) {
  WINDOW* window;

  window = newwin(height, width, starty, startx);
  box(window, 0, 0);
  wrefresh(window);

  return window;
}

void Braincurses::CleanUpWindow(WINDOW* window) {
  werase(window);
  box(window, 0, 0);
  wrefresh(window);
}

void Braincurses::WipeBoard() {
  for (auto kv : windows_) {
    CleanUpWindow(kv.second);
  }
}

static const int kColumn[6] = {2, 11, 20, 29, 38, 47};

static const char* ColorName(int color) {
  switch (color) {
    case COLOR_RED: return "red";
    case COLOR_GREEN: return "green";
    case COLOR_YELLOW: return "yellow";
    case COLOR_BLUE: return "blue";
    case COLOR_MAGENTA: return "magenta";
    case COLOR_CYAN: return "white";  // Really COLOR_WHITE
    default: return "";
  }
}

// Shows the colours of the guess being typed.
void Braincurses::DisplayInput(const std::vector<int>& guess) {
  auto local_window = windows_[kInputWindow];
  for (unsigned i = 0; i < guess.size(); i++) {
    mvwaddstr(local_window, 1, kColumn[i], ColorName(guess[i]));
  }
}

std::vector<int> Braincurses::GetInput() {
  std::string delStr (kInputLength, ' ');
  std::vector<int> guessInput (code_.Length(), -1);
  int input;

  int x = 0;
  while (true) {
    auto local_window = windows_[kInputWindow];
    wmove(local_window, 1, kColumn[x]);
    input = GetKey(local_window);
    int color = -1;
    switch (input) {
      case 'r':
        color = COLOR_RED;
        break;
      case 'g':
        color = COLOR_GREEN;
        break;
      case 'y':
        color = COLOR_YELLOW;
        break;
      case 'b':
        color = COLOR_BLUE;
        break;
      case 'm':
        color = COLOR_MAGENTA;
        break;
      case 'w':
        color = COLOR_CYAN;  // Really COLOR_WHITE
        break;
      case KEY_LEFT:
        if (x > 0) {
          x--;
        }
        break;
      case KEY_RIGHT:
        // Only over a colour already chosen, and never past the last field:
        // moving off the end used to end the guess with fields left empty.
        if (x < code_.Length() - 1 && guessInput[x] != -1) {
          x++;
        }
        break;
      case KEY_BACKSPACE:
      case KEY_DC:
      case 127:
      case 8:
        if (x > 0 && x <= code_.Length()) {
          x--;
          mvwaddstr(local_window, 1, kColumn[x], delStr.c_str());
          guessInput[x] = -1;
        }
        break;
      case KEY_RESIZE:
        Resize();
        DisplayInput(guessInput);
        break;
      default:
        break;
    }
    local_window = windows_[kInputWindow];
    if (color != -1) {
      mvwaddstr(local_window, 1, kColumn[x], delStr.c_str());
      mvwaddstr(local_window, 1, kColumn[x], ColorName(color));
      guessInput[x] = color;
      x++;
    }
    if (x == code_.Length()) {
      // The guess is done once every field has a colour; otherwise go back
      // to the first empty one.
      auto empty = std::find(guessInput.begin(), guessInput.end(), -1);
      if (empty == guessInput.end()) {
        break;
      }
      x = empty - guessInput.begin();
    }
    wmove(local_window, 1, kColumn[x]);
    wrefresh(local_window);
  }
  CleanUpWindow(windows_[kInputWindow]);
  return guessInput;
}

void Braincurses::DisplayGuess(int y, std::vector<int> guess) {
  auto local_window = windows_[kGuessWindow];

  y = 15 - y;

  wattron(local_window, A_BOLD);
  int i = 0;
  for (auto x : codePosition.at(guess.size())) {
    mvwaddch(local_window, y, x, 'X' | COLOR_PAIR(guess[i]));
    i++;
  }
  wattroff(local_window, A_BOLD);
  wrefresh(local_window);
}

void Braincurses::DisplayMarkers(int y, std::vector<int> correct) {
  auto local_window = windows_[kMarkerWindow];

  y = 15 - y;

  wattron(local_window, A_BOLD);

  std::vector<int> x_positions = codePosition.at(code_.Length());
  for (int i = 0; i < code_.Length(); i++) {
    int marker = correct[i];
    int x = x_positions[i];

    if (marker == 2) {  // NAILED_IT
      mvwaddch(local_window, y, x, 'X' | COLOR_PAIR(COLOR_RED));
      continue;
    } else if (marker == 1) {  // ALMOST
      mvwaddch(local_window, y, x, 'X' | COLOR_PAIR(COLOR_WHITE));
      continue;
    }
  }
  wattroff(local_window, A_BOLD);
  wrefresh(local_window);
}

bool Braincurses::IsWinner(std::vector<int> correct) {
  bool winner = true;
  for (unsigned i = 0; i < correct.size(); i++) {
    if (correct[i] != 2) {
      winner = false;
      break;
    }
  }
  return winner;
}

// The result stays in the information panel while the input line asks
// whether to play again (it used to be cleared at once by the question).
bool Braincurses::GameOverPlayAgain(bool winner) {
  if (winner) {
    message_ = "You win!\nCongratulations!";
  } else {
    message_ = "You ran out of\nturns; better luck\nnext time.";
  }
  Redraw();
  return PlayAgain();
}

void Braincurses::DisplayCode(bool colored) {
  auto local_window = windows_[kCodeWindow];

  int i = 0;
  for (auto x : codePosition.at(code_.Length())) {
#ifdef DEBUG
    mvwaddch(local_window, 1, x, 'X' | COLOR_PAIR(code_.Get()[i]) | A_BOLD);
#else
    if (colored) {
      mvwaddch(local_window, 1, x, 'X' | COLOR_PAIR(code_.Get()[i]) | A_BOLD);
    } else {
      mvwaddch(local_window, 1, x, 'X');
    }
#endif
    i++;
  }
  wrefresh(local_window);
}

bool Braincurses::PlayAgain() {
  while (true) {
    CleanUpWindow(windows_[kInputWindow]);
    mvwaddstr(windows_[kInputWindow], 1, 1, "Would you like to play again? ([Y]/n) ");
    wrefresh(windows_[kInputWindow]);

    int again = GetKey(windows_[kInputWindow]);
    if (again == KEY_RESIZE) {
      Resize();
      continue;
    }
    again = tolower(again);
    return (again == 'y' || again == '\n' ? true : false);
  }
}

bool Braincurses::PlayGame() {
  code_.Create();
  history_.clear();
  code_shown_ = false;
  message_.clear();
  WipeBoard();
  PrepareBoard();

  bool winner = false;
  std::vector<int> correct;
  for (int i = 0; i < guesses_; i++) {
    std::vector<int> guess = GetInput();
    correct = code_.IsCorrect(guess.begin(), guess.end());
    history_.emplace_back(guess, correct);

    DisplayGuess(i, guess);
    DisplayMarkers(i, correct);
    if (IsWinner(correct)) {
      winner = true;
      break;
    }
  }
  code_shown_ = true;
  DisplayCode(true);
  return winner;
}

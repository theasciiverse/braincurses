/* main.cpp
 * Copyright © 2015, Brian Derr <brian@derrclan.com>
 */

#include <clocale>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <unistd.h>

#include "braincurses.h"


static const int kMinCodeLength = 4;
static const int kMaxCodeLength = 6;

static const int kDefaultGuessCount = 10;
static const int kMinGuessCount = 1;
static const int kMaxGuessCount = 15;


void PrintUsage() {
  std::cerr << "usage: braincurses [-c code_length] [-g guesses]" << std::endl;
}

void ProcessArgs(int argc, char* argv[], int& code_length, int& guesses) {
  int opt;
  while ((opt = getopt(argc, argv, "c:g:h")) != -1) {
    switch (opt) {
      case 'c':
        code_length = atoi(optarg);
        break;
      case 'g':
        guesses = atoi(optarg);
        break;
      case 'h':
      default:
        PrintUsage();
        exit(EXIT_FAILURE);
    }
  }

  if (code_length > kMaxCodeLength) {
    code_length = kMaxCodeLength;
  } else if (code_length < kMinCodeLength) {
    code_length = kMinCodeLength;
  }

  if (guesses > kMaxGuessCount) {
    guesses = kMaxGuessCount;
  } else if (guesses <= 0) {
    guesses = kDefaultGuessCount;
  }
}

void endwin_atexit_handler() {
  endwin();
}

volatile int quit_requested = 0;

// Only sets a flag: the next key read quits, and the atexit handler puts
// the terminal back. No SA_RESTART, so a waiting wgetch() returns at once.
void quit_signal_handler(int signal) {
  quit_requested = 1;
}

int main(int argc, char* argv[]) {
  int code_length = kMinCodeLength;
  int guesses = kDefaultGuessCount;
  ProcessArgs(argc, argv, code_length, guesses);

  int result = std::atexit(endwin_atexit_handler);
  if (result != 0) {
    std::cerr << "atexit registration failed" << std::endl;
    return EXIT_FAILURE;
  }

#ifdef _WIN32
  std::signal(SIGINT, quit_signal_handler);
  std::signal(SIGTERM, quit_signal_handler);
#else
  struct sigaction action = {};
  action.sa_handler = quit_signal_handler;
  sigemptyset(&action.sa_mask);
  sigaction(SIGINT, &action, nullptr);
  sigaction(SIGTERM, &action, nullptr);
  sigaction(SIGHUP, &action, nullptr);
#endif

  // Box drawing and the rest in the terminal's own character set.
  std::setlocale(LC_ALL, "");

  Braincurses bc(code_length, guesses);

  bool winner = false;
  do {
    winner = bc.PlayGame();
  } while (bc.GameOverPlayAgain(winner));

  return EXIT_SUCCESS;
}

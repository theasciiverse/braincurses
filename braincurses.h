/* braincurses.h
 * Copyright © 2002, Brian Derr <brian@derrclan.com>
 */

#ifndef BRAINCURSES_H
#define BRAINCURSES_H

#include <ncurses.h>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "code.h"

const int kInputLength = 7;
const std::string kGameName = "BrainCurses";

const std::string kCodeWindow = "code";
const std::string kGuessWindow = "guess";
const std::string kHeaderWindow = "header";
const std::string kInfoWindow = "info";
const std::string kInputWindow = "input";
const std::string kMarkerWindow = "marker";
const std::string kWatermarkWindow = "watermark";

// The board: 60 columns by 23 rows, centred in the terminal.
const int kBoardWidth = 60;
const int kBoardHeight = 23;


class Braincurses {
  public:
    Braincurses(int code_length, int guesses);
    bool GameOverPlayAgain(bool winner);
    bool Initialized() const { return initialized_; };
    bool PlayGame();

  private:
    void CleanUpWindow(WINDOW* window);
    void CreateWindows();
    WINDOW *CreateWindow(int height, int width, int starty, int startx);
    void DeleteWindows();
    void DisplayCode(bool colored);
    void DisplayGuess(int y, std::vector<int> guess);
    void DisplayInput(const std::vector<int>& guess);
    void DisplayMarkers(int y, std::vector<int> correct);
    std::vector<int> GetInput();
    int GetKey(WINDOW* window);
    void InitializeNcurses();
    bool IsWinner(std::vector<int> guess);
    bool PlayAgain();
    void PrepareBoard();
    void Redraw();
    void Resize();
    void WipeBoard();

    Code code_;
    int guesses_;
    bool initialized_;
    std::unordered_map<std::string, WINDOW*> windows_;

    // What is on the board, so that it can be drawn again after a resize.
    std::vector<std::pair<std::vector<int>, std::vector<int>>> history_;
    bool code_shown_;
    std::string message_;
};

void endwin_handler(void);

// Set by SIGINT, SIGTERM and SIGHUP: the game quits at the next key read.
extern volatile int quit_requested;

#endif  // BRAINCURSES_H

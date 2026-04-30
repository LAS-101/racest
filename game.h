#pragma once
#include "types.h"
#include <vector>
#include <string>

extern vector<string> g_wordBank;
extern vector<Star>        g_stars;
extern vector<Word>        g_words;
extern GState  g_state;
extern float   g_time;       // seconds elapsed in current run
extern int     g_score;
extern int     g_highScore;
extern bool    g_newRecord;
extern int     g_targetIdx;  // index into g_words (-1 = no target)

void initGame();           // load word bank, stars, high score from disk
void startGame();          // reset and enter PLAYING state
void updateGame(float dt); // advance logic by one frame (input + physics)
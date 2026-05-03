#pragma once
#include "types.h"
#include <vector>
#include <string>

extern vector<string> g_wordBank;
extern vector<Star>        g_stars;
extern vector<Word>        g_words;
extern GState  g_state;
extern float   g_time;       
extern int     g_score;
extern int     g_highScore;
extern bool    g_newRecord;
extern int     g_targetIdx;  

void initGame();           
void startGame();          
void updateGame(float dt); 
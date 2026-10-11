#pragma once
#include "raylib.h"
#include <string>
#include <vector>
using namespace std;

inline constexpr int   SW        = 1280;
inline constexpr int   SH        = 720;
inline constexpr float DANGER_X  = 95.0f;
inline constexpr int   FONT_SIZE = 27;
inline constexpr int   MAX_WORDS = 7;

inline const Color COL_BG      = {  0,   0,   0, 255};   
inline const Color COL_UNTYPED = {150, 155, 200, 255};   
inline const Color COL_TYPED   = { 68, 198, 110, 255};   
inline const Color COL_ERROR   = {215,  58,  58, 255};   
inline const Color COL_TARGET  = {235, 240, 255, 255};   
inline const Color COL_SCORE   = {110, 170, 255, 255};   
inline const Color COL_GOLD    = {255, 205,  55, 255};   
inline const Color COL_DANGER  = {190,  25,  25, 210};   

struct Star {
    float x, y, r;
    float phase, twinkSpeed;
};

struct Word {
    string text;
    int   typed;      
    bool  errored;    
    float x, y;
    float speed;      
    bool  targeted;   
};

enum class GState { MENU, PLAYING, GAMEOVER };

// Both the keyboard and the agent submit input through this interface.
struct GameInput {
    std::string text;
    bool backspace = false;
    bool start = false;
};

// Presentation settings for an external agent; contains no decision logic.
struct AgentStatus {
    bool enabled = false;
    bool available = false;
    float wpm = 120.0f;
};

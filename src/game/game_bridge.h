#pragma once
#include "game.h"
#include <string>

// Transport only. All target selection and typing decisions live in policy.py.
std::string gameObservation(const Game& game, float dt, bool enabled, int round);
#if !defined(__EMSCRIPTEN__)
bool requestPythonInput(const Game& game, float dt, bool enabled, int round, GameInput& input);
#endif

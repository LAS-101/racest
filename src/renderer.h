#pragma once
#include "types.h"

class Game;

class Renderer {
public:
	Renderer();
	void drawFrame(const Game& game, float now) const;
};
#pragma once
#include "raylib.h"
#include "types.h"

class Game;

// Plays sound effects by observing game state between frames, so the game
// logic (and headless simulations) stay independent of audio.
class Audio {
public:
	static constexpr int   COIN_SCORE_STEP = 500;
	static constexpr float TITLE_FADE_SECONDS = 0.6f;

	void init();
	void update(const Game& game, float dt);
	void shutdown();

private:
	bool  m_ready = false;
	Sound m_word{}, m_coin{}, m_death{}, m_gameOver{};
	Music m_title{};
	bool  m_titlePlaying = false;
	float m_titleFade = -1.0f; // Seconds left in the fade-out; negative when not fading.

	GState m_lastState = GState::MENU;
	int    m_lastCompleted = 0;
	int    m_lastScore = 0;
	bool   m_gameOverQueued = false;

	void play(const Sound& sound) const;
	void stopRoundEndSounds();
	void updateTitleMusic(float dt);
};

#include "audio.h"
#include "game.h"

static Sound loadSound(const char* path) {
	if (!FileExists(path)) {
		TraceLog(LOG_WARNING, "AUDIO: Missing sound file %s", path);
		return Sound{};
	}
	return LoadSound(path);
}

void Audio::init() {
	InitAudioDevice();
	m_ready = IsAudioDeviceReady();
	if (!m_ready) return;
	m_word     = loadSound("assets/sounds/word.mp3");
	m_coin     = loadSound("assets/sounds/coin.mp3");
	m_death    = loadSound("assets/sounds/death.mp3");
	m_gameOver = loadSound("assets/sounds/gameover.mp3");

	// The title theme loops until the first round starts. Browsers keep audio
	// suspended until the first click or key press, so on the web it starts
	// at the player's first interaction.
	if (FileExists("assets/sounds/title.mp3")) m_title = LoadMusicStream("assets/sounds/title.mp3");
	if (IsMusicValid(m_title)) {
		m_title.looping = true;
		PlayMusicStream(m_title);
		m_titlePlaying = true;
	}
}

void Audio::updateTitleMusic(float dt) {
	if (!m_titlePlaying) return;
	if (m_titleFade >= 0.0f) {
		m_titleFade -= dt;
		if (m_titleFade <= 0.0f) {
			StopMusicStream(m_title);
			m_titlePlaying = false;
			return;
		}
		SetMusicVolume(m_title, m_titleFade / TITLE_FADE_SECONDS);
	}
	UpdateMusicStream(m_title);
}

void Audio::play(const Sound& sound) const {
	if (m_ready && IsSoundValid(sound)) PlaySound(sound);
}

void Audio::stopRoundEndSounds() {
	m_gameOverQueued = false;
	if (!m_ready) return;
	if (IsSoundValid(m_death)) StopSound(m_death);
	if (IsSoundValid(m_gameOver)) StopSound(m_gameOver);
}

void Audio::update(const Game& game, float dt) {
	const GState state = game.state();
	const int completed = game.completedWords();
	const int score = game.score();

	if (state == GState::PLAYING && m_lastState != GState::PLAYING) {
		stopRoundEndSounds();
		m_lastCompleted = m_lastScore = 0;
		if (m_titlePlaying && m_titleFade < 0.0f) m_titleFade = TITLE_FADE_SECONDS;
	}
	updateTitleMusic(dt);

	if (state == GState::PLAYING) {
		if (completed > m_lastCompleted) play(m_word);
		if (score / COIN_SCORE_STEP > m_lastScore / COIN_SCORE_STEP) play(m_coin);
	} else if (state == GState::GAMEOVER && m_lastState == GState::PLAYING) {
		play(m_death);
		m_gameOverQueued = true;
	}

	if (m_gameOverQueued && !(m_ready && IsSoundValid(m_death) && IsSoundPlaying(m_death))) {
		m_gameOverQueued = false;
		play(m_gameOver);
	}

	m_lastState = state;
	m_lastCompleted = completed;
	m_lastScore = score;
}

void Audio::shutdown() {
	if (!m_ready) return;
	for (Sound* sound : {&m_word, &m_coin, &m_death, &m_gameOver}) {
		if (IsSoundValid(*sound)) UnloadSound(*sound);
		*sound = Sound{};
	}
	if (IsMusicValid(m_title)) UnloadMusicStream(m_title);
	m_title = Music{};
	m_titlePlaying = false;
	CloseAudioDevice();
	m_ready = false;
}

#pragma once
#include "types.h"
#include <vector>
#include <string>
#include <random>

class Renderer;

class Game {
public:
	explicit Game(unsigned int seed = std::random_device{}());
	void init(bool persistScores = true, const std::string& wordsPath = "assets/words.txt");
	bool setWordBank(const std::string& text);
	void start();
	void update(float dt, const GameInput& input = {});
	void markAssisted();

	// accessors for renderer
	const std::vector<Star>& stars() const;
	const std::vector<Word>& words() const;
	float timeElapsed() const;
	int score() const;
	int highScore() const;
	bool newRecord() const;
	GState state() const;
	bool assisted() const;
	int completedWords() const;

private:
	// game data
	std::vector<std::string> m_wordBank;
	std::vector<Star>        m_stars;
	std::vector<Word>        m_words;
	GState  m_state;
	float   m_time;
	int     m_score;
	int     m_highScore;
	bool    m_newRecord;
	int     m_targetIdx;
	bool    m_persistScores = true;
	bool    m_assisted = false;
	int     m_completedWords = 0;

	float   m_spawnTimer;
	std::mt19937 m_rng;

	// helpers
	int loadHighScore();
	void saveHighScore(int v);
	std::vector<std::string> loadWordBank(const std::string& wordsPath);
	void initStars();
	float wordSpeed(float elapsed);
	float spawnInterval(float elapsed);
	std::string pickWord(float elapsed);
	void spawnWord();
	void triggerGameOver();
	void handleInput(const GameInput& input);
	void handleCharacter(char key);
	void completeWord(int index);
};

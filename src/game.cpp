#include "game.h"
#include "raylib.h"
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cctype>
#include <chrono>
using namespace std;

static const char* SAVE_PATH  = "highscore.dat";
static const char* WORDS_PATH = "words.txt";

Game::Game()
    : m_state(GState::MENU), m_time(0.0f), m_score(0), m_highScore(0), m_newRecord(false),
      m_targetIdx(-1), m_spawnTimer(0.0f), m_rng((random_device())()) {}

int Game::loadHighScore() {
    ifstream f(SAVE_PATH);
    int v = 0;
    if (f.is_open()) f >> v;
    return v;
}

void Game::saveHighScore(int v) {
    ofstream f(SAVE_PATH);
    if (f.is_open()) f << v;
}

vector<string> Game::loadWordBank() {
    vector<string> bank;
    ifstream f(WORDS_PATH);
    string w;
    while (f >> w) {
        string clean;
        for (char c : w)
            if (isalpha((unsigned char)c))
                clean += (char)tolower((unsigned char)c);
        if (clean.size() >= 3)
            bank.push_back(clean);
    }
    if (bank.empty()) {
        bank = {
            "the","and","for","are","not","you","all","can","day","see",
            "run","sky","star","dark","type","fast","word","game","play",
            "code","space","drift","light","night","black","break","build",
            "about","after","again","bring","could","every","found","great",
            "large","learn","might","never","often","place","power","right",
            "small","sound","still","think","under","where","which","while",
            "already","another","because","between","capture","command",
            "complex","connect","contain","culture","digital","explore",
            "forward","freedom","general","history","include","journey",
            "million","morning","mystery","nothing","outside","pattern",
            "perfect","picture","problem","process","protect","provide",
            "quickly","reading","realize","receive","reflect","release",
            "require","respect","results","running","science","section",
            "several","silence","similar","society","special","station",
            "student","subject","surface","survive","tonight","trouble",
            "beautiful","challenge","character","community","dangerous",
            "discovery","important","knowledge","landscape","necessary",
            "potential","processor","recognize","sometimes","structure",
            "technical","telescope","temporary","therefore","yesterday"
        };
    }
    return bank;
}

void Game::initStars() {
    m_stars.clear();
    m_stars.reserve(260);
    uniform_real_distribution<float> rx(0, SW), ry(0, SH);
    uniform_real_distribution<float> rr(0.4f, 2.5f);
    uniform_real_distribution<float> rp(0.0f, 6.283f);
    uniform_real_distribution<float> rs(0.35f, 2.7f);
    for (int i = 0; i < 260; ++i)
        m_stars.push_back({rx(m_rng), ry(m_rng), rr(m_rng), rp(m_rng), rs(m_rng)});
}

float Game::wordSpeed(float elapsed) {
    float base = 88.0f + min(elapsed * 0.64f, 182.0f);
    uniform_real_distribution<float> jitter(-14.0f, 14.0f);
    return base + jitter(m_rng);
}

float Game::spawnInterval(float elapsed) {
    return max(1.1f, 3.3f - elapsed * 0.014f);
}

string Game::pickWord(float elapsed) {
    float diff   = elapsed / 55.0f;
    int   minLen = 3 + min((int)(diff * 2.0f), 5);
    int   maxLen = minLen + 4;

    vector<string> pool;
    for (const auto& w : m_wordBank) {
        int l = (int)w.size();
        if (l >= minLen && l <= maxLen) pool.push_back(w);
    }
    if (pool.empty()) pool = m_wordBank;

    uniform_int_distribution<int> d(0, (int)pool.size() - 1);
    return pool[d(m_rng)];
}

void Game::spawnWord() {
    if ((int)m_words.size() >= MAX_WORDS) return;

    string txt = pickWord(m_time);

    uniform_real_distribution<float> yd(48.0f, SH - 72.0f);
    float y = yd(m_rng);
    for (int attempt = 0; attempt < 25; ++attempt) {
        float candidate = yd(m_rng);
        bool  ok        = true;
        for (const auto& w : m_words)
            if (abs(w.y - candidate) < 46.0f) { ok = false; break; }
        if (ok) { y = candidate; break; }
    }

    Word w;
    w.text     = txt;
    w.typed    = 0;
    w.errored  = false;
    w.x        = (float)SW + 24.0f;
    w.y        = y;
    w.speed    = wordSpeed(m_time);
    w.targeted = false;
    m_words.push_back(w);
}

void Game::triggerGameOver() {
    if (m_score > m_highScore) {
        m_highScore = m_score;
        m_newRecord = true;
        saveHighScore(m_highScore);
    }
    m_state = GState::GAMEOVER;
}

void Game::start() {
    m_words.clear();
    m_time       = 0.0f;
    m_score      = 0;
    m_newRecord  = false;
    m_spawnTimer = 0.0f;
    m_targetIdx  = -1;
    m_state      = GState::PLAYING;
    spawnWord();
}

void Game::init() {
    m_wordBank  = loadWordBank();
    m_highScore = loadHighScore();
    initStars();
}

void Game::handleInput() {
    if (m_targetIdx >= 0 && m_targetIdx < (int)m_words.size()) {
        Word& tw = m_words[m_targetIdx];

        if (IsKeyPressed(KEY_BACKSPACE)) {
            if (tw.errored) {
                tw.errored = false;
            } else if (tw.typed > 0) {
                tw.typed--;
                if (tw.typed == 0) {
                    tw.targeted = false;
                    m_targetIdx = -1;
                }
            }
        }

        int ch;
        while ((ch = GetCharPressed()) != 0) {
            if (tw.errored) break;
            if (tw.typed >= (int)tw.text.size()) break;

            char key = (char)tolower(ch);
            if (key == tw.text[tw.typed]) {
                tw.typed++;
                if (tw.typed == (int)tw.text.size()) {
                    m_score += 10 + (int)(tw.speed / 14.0f);
                    m_words.erase(m_words.begin() + m_targetIdx);
                    m_targetIdx = -1;
                    break;
                }
            } else {
                tw.errored = true;
                break;
            }
        }

    } else {
        int ch;
        if ((ch = GetCharPressed()) != 0) {
            char key = (char)tolower(ch);

            int   best  = -1;
            float bestX = (float)SW + 1.0f;
            for (int i = 0; i < (int)m_words.size(); ++i) {
                const auto& w = m_words[i];
                if (!w.text.empty() && w.text[0] == key && w.x < bestX) {
                    bestX = w.x;
                    best  = i;
                }
            }
            if (best >= 0) {
                m_targetIdx              = best;
                m_words[best].targeted   = true;
                m_words[best].typed      = 1;
                if (m_words[best].typed == (int)m_words[best].text.size()) {
                    m_score += 10 + (int)(m_words[best].speed / 14.0f);
                    m_words.erase(m_words.begin() + best);
                    m_targetIdx = -1;
                }
            }
        }
    }
}

void Game::update(float dt) {
    if (m_state == GState::PLAYING) {
        m_time += dt;

        for (auto& w : m_words)
            w.x -= w.speed * dt;

        for (const auto& w : m_words) {
            if (w.x < DANGER_X) {
                triggerGameOver();
                return;
            }
        }

        m_spawnTimer += dt;
        if (m_spawnTimer >= spawnInterval(m_time)) {
            m_spawnTimer = 0.0f;
            spawnWord();
        }

        handleInput();

    } else {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
            start();
    }
}

// Accessors
const std::vector<Star>& Game::stars() const { return m_stars; }
const std::vector<Word>& Game::words() const { return m_words; }
float Game::timeElapsed() const { return m_time; }
int Game::score() const { return m_score; }
int Game::highScore() const { return m_highScore; }
bool Game::newRecord() const { return m_newRecord; }
GState Game::state() const { return m_state; }

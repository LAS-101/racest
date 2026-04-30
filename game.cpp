#include "game.h"
#include "raylib.h"
#include <fstream>
#include <algorithm>
#include <random>
#include <cmath>
#include <cstdio>
#include <cctype>
using namespace std;

static const char* SAVE_PATH  = "highscore.dat";
static const char* WORDS_PATH = "words.txt";

// ─── Global definitions ───────────────────────────────────────────────────────
vector<string> g_wordBank;
vector<Star>        g_stars;
vector<Word>        g_words;
GState  g_state      = GState::MENU;
float   g_time       = 0.0f;
int     g_score      = 0;
int     g_highScore  = 0;
bool    g_newRecord  = false;
int     g_targetIdx  = -1;

static float        g_spawnTimer = 0.0f;
static mt19937 g_rng([] { random_device rd; return rd(); }());

// ─── Persistence ─────────────────────────────────────────────────────────────
static int loadHighScore() {
    ifstream f(SAVE_PATH);
    int v = 0;
    if (f.is_open()) f >> v;
    return v;
}

static void saveHighScore(int v) {
    ofstream f(SAVE_PATH);
    if (f.is_open()) f << v;
}

// ─── Word bank ────────────────────────────────────────────────────────────────
static vector<string> loadWordBank() {
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
        // Fallback if words.txt is missing
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

// ─── Stars ────────────────────────────────────────────────────────────────────
static void initStars() {
    g_stars.clear();
    g_stars.reserve(260);
    uniform_real_distribution<float> rx(0, SW), ry(0, SH);
    uniform_real_distribution<float> rr(0.4f, 2.5f);
    uniform_real_distribution<float> rp(0.0f, 6.283f);
    uniform_real_distribution<float> rs(0.35f, 2.7f);
    for (int i = 0; i < 260; ++i)
        g_stars.push_back({rx(g_rng), ry(g_rng), rr(g_rng), rp(g_rng), rs(g_rng)});
}

// ─── Difficulty curves ────────────────────────────────────────────────────────
static float wordSpeed(float elapsed) {
    float base = 88.0f + min(elapsed * 0.64f, 182.0f);
    uniform_real_distribution<float> jitter(-14.0f, 14.0f);
    return base + jitter(g_rng);
}

static float spawnInterval(float elapsed) {
    return max(1.1f, 3.3f - elapsed * 0.014f);
}

static string pickWord(float elapsed) {
    float diff   = elapsed / 55.0f;
    int   minLen = 3 + min((int)(diff * 2.0f), 5);
    int   maxLen = minLen + 4;

    vector<string> pool;
    for (const auto& w : g_wordBank) {
        int l = (int)w.size();
        if (l >= minLen && l <= maxLen) pool.push_back(w);
    }
    if (pool.empty()) pool = g_wordBank;

    uniform_int_distribution<int> d(0, (int)pool.size() - 1);
    return pool[d(g_rng)];
}

// ─── Word spawning ────────────────────────────────────────────────────────────
static void spawnWord() {
    if ((int)g_words.size() >= MAX_WORDS) return;

    string txt = pickWord(g_time);

    uniform_real_distribution<float> yd(48.0f, SH - 72.0f);
    float y = yd(g_rng);
    for (int attempt = 0; attempt < 25; ++attempt) {
        float candidate = yd(g_rng);
        bool  ok        = true;
        for (const auto& w : g_words)
            if (abs(w.y - candidate) < 46.0f) { ok = false; break; }
        if (ok) { y = candidate; break; }
    }

    Word w;
    w.text     = txt;
    w.typed    = 0;
    w.errored  = false;
    w.x        = (float)SW + 24.0f;
    w.y        = y;
    w.speed    = wordSpeed(g_time);
    w.targeted = false;
    g_words.push_back(w);
}

// ─── State transitions ────────────────────────────────────────────────────────
static void triggerGameOver() {
    if (g_score > g_highScore) {
        g_highScore = g_score;
        g_newRecord = true;
        saveHighScore(g_highScore);
    }
    g_state = GState::GAMEOVER;
}

void startGame() {
    g_words.clear();
    g_time       = 0.0f;
    g_score      = 0;
    g_newRecord  = false;
    g_spawnTimer = 0.0f;
    g_targetIdx  = -1;
    g_state      = GState::PLAYING;
    spawnWord();
}

void initGame() {
    g_wordBank  = loadWordBank();
    g_highScore = loadHighScore();
    initStars();
}

// ─── Input handling ───────────────────────────────────────────────────────────
static void handleInput() {
    if (g_targetIdx >= 0 && g_targetIdx < (int)g_words.size()) {
        Word& tw = g_words[g_targetIdx];

        // Backspace — remove last typed char or clear error
        if (IsKeyPressed(KEY_BACKSPACE)) {
            if (tw.errored) {
                tw.errored = false;
            } else if (tw.typed > 0) {
                tw.typed--;
                if (tw.typed == 0) {
                    tw.targeted = false;
                    g_targetIdx = -1;
                }
            }
        }

        // Character input
        int ch;
        while ((ch = GetCharPressed()) != 0) {
            if (tw.errored) break;
            if (tw.typed >= (int)tw.text.size()) break;

            char key = (char)tolower(ch);
            if (key == tw.text[tw.typed]) {
                tw.typed++;
                // Word complete
                if (tw.typed == (int)tw.text.size()) {
                    g_score += 10 + (int)(tw.speed / 14.0f);
                    g_words.erase(g_words.begin() + g_targetIdx);
                    g_targetIdx = -1;
                    break;
                }
            } else {
                tw.errored = true;
                break;
            }
        }

    } else {
        // No target — lock onto a word by typing its first letter
        int ch;
        if ((ch = GetCharPressed()) != 0) {
            char key = (char)tolower(ch);

            int   best  = -1;
            float bestX = (float)SW + 1.0f;
            for (int i = 0; i < (int)g_words.size(); ++i) {
                const auto& w = g_words[i];
                if (!w.text.empty() && w.text[0] == key && w.x < bestX) {
                    bestX = w.x;
                    best  = i;
                }
            }
            if (best >= 0) {
                g_targetIdx              = best;
                g_words[best].targeted   = true;
                g_words[best].typed      = 1;
                // Handle immediate completion (1-char word)
                if (g_words[best].typed == (int)g_words[best].text.size()) {
                    g_score += 10 + (int)(g_words[best].speed / 14.0f);
                    g_words.erase(g_words.begin() + best);
                    g_targetIdx = -1;
                }
            }
        }
    }
}

// ─── Main update ──────────────────────────────────────────────────────────────
void updateGame(float dt) {
    if (g_state == GState::PLAYING) {
        g_time += dt;

        // Move all words leftward
        for (auto& w : g_words)
            w.x -= w.speed * dt;

        // Game over if any word crosses the danger line
        for (const auto& w : g_words) {
            if (w.x < DANGER_X) {
                triggerGameOver();
                return;
            }
        }

        // Spawn timer
        g_spawnTimer += dt;
        if (g_spawnTimer >= spawnInterval(g_time)) {
            g_spawnTimer = 0.0f;
            spawnWord();
        }

        handleInput();

    } else {
        // MENU or GAMEOVER — wait for Enter / Space
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
            startGame();
    }
}
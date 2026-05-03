#include "renderer.h"
#include "game.h"
#include "raylib.h"
#include <cmath>
#include <cstdio>
#include <algorithm>
using namespace std;


static int partWidth(const string& s, int n, int size) {
    if (n <= 0) return 0;
    return MeasureText(s.substr(0, n).c_str(), size);
}

static void drawStars(float now) {
    for (const auto& s : g_stars) {
        float t   = 0.5f + 0.5f * sinf(now * s.twinkSpeed + s.phase);
        Color col = {200, 212, 255, (unsigned char)(72 + 168 * t)};
        DrawCircleV({s.x, s.y}, s.r * (0.55f + 0.45f * t), col);
    }
}

static void drawWord(const Word& w) {
    int px   = (int)w.x;
    int py   = (int)w.y - FONT_SIZE / 2;
    int xOff = 0;

    if (w.typed > 0) {
        DrawText(w.text.substr(0, w.typed).c_str(), px, py, FONT_SIZE, COL_TYPED);
        xOff = partWidth(w.text, w.typed, FONT_SIZE);
    }

    if (w.typed < (int)w.text.size()) {
        if (w.errored) {
           string cur(1, w.text[w.typed]);
            DrawText(cur.c_str(), px + xOff, py, FONT_SIZE, COL_ERROR);
            xOff += MeasureText(cur.c_str(), FONT_SIZE);
            if (w.typed + 1 < (int)w.text.size())
                DrawText(w.text.substr(w.typed + 1).c_str(), px + xOff, py, FONT_SIZE, COL_UNTYPED);
        } else {
            Color col;
            if (w.targeted) {
                col = COL_TARGET;
            } else {
                float prox = 1.0f - min((w.x - DANGER_X) / 360.0f, 1.0f);
                prox = max(0.0f, prox);
                col  = {
                    (unsigned char)min(255.0f, COL_UNTYPED.r + (255 - COL_UNTYPED.r) * prox * 0.75f),
                    (unsigned char)(COL_UNTYPED.g * (1.0f - prox * 0.65f)),
                    (unsigned char)(COL_UNTYPED.b * (1.0f - prox * 0.65f)),
                    255
                };
            }
            DrawText(w.text.substr(w.typed).c_str(), px + xOff, py, FONT_SIZE, col);
        }
    }
}

static void drawHUD() {
    char scoreBuf[64], timeBuf[32], hsBuf[64];
    snprintf(scoreBuf, sizeof(scoreBuf), "SCORE  %d", g_score);
    snprintf(hsBuf,    sizeof(hsBuf),    "BEST  %d",  g_highScore);

    int mins = (int)g_time / 60, secs = (int)g_time % 60;
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", mins, secs);

    DrawText(scoreBuf, SW - MeasureText(scoreBuf, 21) - 18, 18, 21, COL_SCORE);
    DrawText(hsBuf, 18, 18, 19, COL_GOLD);

    int tw = MeasureText(timeBuf, 20);
    DrawText(timeBuf, SW/2 - tw/2, 18, 20, COL_UNTYPED);
}

static void drawDangerZone() {
    DrawRectangle(0, 0, (int)DANGER_X, SH, {160, 15, 15, 22});
    DrawLineEx({DANGER_X, 0}, {DANGER_X, (float)SH}, 1.5f, COL_DANGER);
}

static void drawMenu(float now) {
    const char* title = "RACEST";
    int tw = MeasureText(title, 80);
    DrawText(title, SW/2 - tw/2, SH/2 - 160, 80, COL_TARGET);

    const char* sub = "type words before they reach the red line";
    int sw = MeasureText(sub, 21);
    DrawText(sub, SW/2 - sw/2, SH/2 - 62, 21, COL_UNTYPED);

    const char* hint = "type first letter to lock on   |   backspace to remove a letter";
    int hw = MeasureText(hint, 17);
    DrawText(hint, SW/2 - hw/2, SH/2 - 28, 17, {95, 100, 140, 200});

    float blink = 0.5f + 0.5f * sinf(now * 2.7f);
    Color pc = {COL_SCORE.r, COL_SCORE.g, COL_SCORE.b, (unsigned char)(85 + 170 * blink)};
    const char* prompt = "PRESS  ENTER  TO  START";
    int pw = MeasureText(prompt, 23);
    DrawText(prompt, SW/2 - pw/2, SH/2 + 22, 23, pc);

    if (g_highScore > 0) {
        char hs[64];
        snprintf(hs, sizeof(hs), "High Score:  %d", g_highScore);
        int hw2 = MeasureText(hs, 22);
        DrawText(hs, SW/2 - hw2/2, SH/2 + 80, 22, COL_GOLD);
    }
}

static void drawGameOver(float now) {
    DrawRectangle(0, 0, SW, SH, {0, 0, 0, 160});

    const char* goTxt = "GAME  OVER";
    int gw = MeasureText(goTxt, 68);
    DrawText(goTxt, SW/2 - gw/2, SH/2 - 128, 68, {215, 60, 60, 255});

    char sc[64];
    snprintf(sc, sizeof(sc), "Score: %d", g_score);
    int sw = MeasureText(sc, 34);
    DrawText(sc, SW/2 - sw/2, SH/2 - 40, 34, COL_TARGET);

    if (g_newRecord) {
        float pulse = 0.5f + 0.5f * sinf(now * 4.2f);
        Color nc = {255, (unsigned char)(160 + 95 * pulse), 45, 255};
        const char* nr = "NEW HIGH SCORE!";
        int nw = MeasureText(nr, 30);
        DrawText(nr, SW/2 - nw/2, SH/2 + 18, 30, nc);
    } else if (g_highScore > 0) {
        char hs[64];
        snprintf(hs, sizeof(hs), "High Score: %d", g_highScore);
        int hw = MeasureText(hs, 24);
        DrawText(hs, SW/2 - hw/2, SH/2 + 18, 24, COL_GOLD);
    }

    float blink = 0.5f + 0.5f * sinf(now * 2.7f);
    Color rc = {COL_SCORE.r, COL_SCORE.g, COL_SCORE.b, (unsigned char)(85 + 170 * blink)};
    const char* restart = "PRESS  ENTER  TO  PLAY  AGAIN";
    int rw = MeasureText(restart, 20);
    DrawText(restart, SW/2 - rw/2, SH/2 + 72, 20, rc);
}

void drawFrame(float now) {
    drawStars(now);

    if (g_state == GState::MENU) {
        drawMenu(now);
    }

    if (g_state == GState::PLAYING || g_state == GState::GAMEOVER) {
        drawDangerZone();
        for (const auto& w : g_words)
            drawWord(w);
        drawHUD();
    }

    if (g_state == GState::GAMEOVER) {
        drawGameOver(now);
    }
}
// ═══════════════════════════════════════════════════════════════════
//  WORDRIFT  —  A Space Typing Game
//
//  Build (Linux):
//    g++ main.cpp game.cpp renderer.cpp -o wordrift \
//        -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -std=c++17
//
//  Build (Windows):
//    g++ main.cpp game.cpp renderer.cpp -o wordrift.exe \
//        -lraylib -lopengl32 -lgdi32 -lwinmm -std=c++17
//
//  Or use CMakeLists.txt (auto-downloads raylib if needed).
//  words.txt must be in the same folder as the executable.
// ═══════════════════════════════════════════════════════════════════

#include "raylib.h"
#include "types.h"
#include "game.h"
#include "renderer.h"

int main() {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(SW, SH, "RACEST");
    SetTargetFPS(60);

    initGame();   // load word bank + stars + high score from disk

    while (!WindowShouldClose()) {
        updateGame(GetFrameTime());

        BeginDrawing();
        ClearBackground(COL_BG);
        drawFrame((float)GetTime());
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
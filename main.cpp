#include "raylib.h"
#include "types.h"
#include "game.h"
#include "renderer.h"

int main() {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(SW, SH, "RACEST");
    SetTargetFPS(60);

    initGame();   

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
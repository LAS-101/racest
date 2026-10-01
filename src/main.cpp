#include "raylib.h"
#include "types.h"
#include "game.h"
#include "renderer.h"
#include <algorithm>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>

EM_JS(bool, browserGameFocused, (), {
    return !document.hidden && document.hasFocus() && document.activeElement === Module.canvas;
});
#endif

struct Application {
    Game game;
    Renderer renderer;
};

static void updateFrame(void* context) {
    auto& app = *static_cast<Application*>(context);
#if defined(__EMSCRIPTEN__)
    // Pause when the player switches tabs or focuses another window.
    const float dt = std::min(GetFrameTime(), 0.1f);
    if (browserGameFocused()) app.game.update(dt);
#else
    app.game.update(GetFrameTime());
#endif
    BeginDrawing();
    ClearBackground(COL_BG);
    app.renderer.drawFrame(app.game, (float)GetTime());
    EndDrawing();
}

int main() {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(SW, SH, "RACEST");
    SetTargetFPS(60);

    static Application app;
    app.game.init();

#if defined(__EMSCRIPTEN__)
    // Let the browser schedule frames instead of blocking its event loop.
    emscripten_set_main_loop_arg(updateFrame, &app, 0, true);
#else
    while (!WindowShouldClose()) {
        updateFrame(&app);
    }

    CloseWindow();
#endif
    return 0;
}

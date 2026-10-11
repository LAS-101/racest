#include "raylib.h"
#include "types.h"
#include "game.h"
#include "game_bridge.h"
#include "renderer.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <string>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
EM_JS(bool, browserGameFocused, (), {
    return !document.hidden && document.hasFocus() && document.activeElement === Module.canvas;
});
EM_JS(void, publishBrowserStatus, (int state, bool enabled, float wpm, int score, int completed, bool focused, int round), {
    if (typeof Module.onGameStatus === 'function')
        Module.onGameStatus({state, enabled, wpm, score, completed, focused, round});
});
EM_JS(void, publishBrowserObservation, (const char* json), {
    if (typeof Module.onGameObservation === 'function') Module.onGameObservation(JSON.parse(UTF8ToString(json)));
});
#endif

struct Application {
    Game game;
    Renderer renderer;
    AgentStatus agent;
    GameInput pendingInput;
    int round = 0;
    bool stdioAgent = false;

    Application(unsigned int seed, float wpm, bool stdio) : game(seed), stdioAgent(stdio) {
        agent.wpm = wpm;
#if defined(__EMSCRIPTEN__)
        agent.available = true;
#else
        agent.available = stdio;
#endif
    }
    void startRound() { game.start(); ++round; pendingInput = {}; }
    void setAgentEnabled(bool enabled) {
        agent.enabled = enabled && agent.available;
        pendingInput = {};
        if (agent.enabled) {
            if (game.state() != GState::PLAYING) startRound();
            game.markAssisted();
        }
    }
};

#if defined(__EMSCRIPTEN__)
static Application* browserApp = nullptr;
extern "C" {
EMSCRIPTEN_KEEPALIVE void racest_set_agent(int enabled) {
    if (browserApp) browserApp->setAgentEnabled(enabled != 0);
}
EMSCRIPTEN_KEEPALIVE void racest_set_agent_wpm(float wpm) {
    if (browserApp && std::isfinite(wpm) && wpm >= 1 && wpm <= 600) browserApp->agent.wpm = wpm;
}
EMSCRIPTEN_KEEPALIVE void racest_submit_agent_input(int ch, int backspace) {
    if (!browserApp || !browserApp->agent.enabled || browserApp->game.state() != GState::PLAYING) return;
    if (ch >= 'a' && ch <= 'z') browserApp->pendingInput.text = (char)ch;
    browserApp->pendingInput.backspace = backspace != 0;
}
}
#endif

static GameInput readKeyboard() {
    GameInput input;
    input.start = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    input.backspace = IsKeyPressed(KEY_BACKSPACE);
    int ch;
    while ((ch = GetCharPressed()) != 0) {
        if (ch > 0 && ch < 128) input.text += (char)ch;
    }
    return input;
}

static void updateFrame(void* context) {
    auto& app = *static_cast<Application*>(context);
    const float dt = std::min(GetFrameTime(), 0.1f);
    bool focused = true;
#if defined(__EMSCRIPTEN__)
    focused = browserGameFocused();
#endif
    GameInput keyboard = readKeyboard();
    if (focused) {
        if (IsKeyPressed(KEY_F2)) app.setAgentEnabled(!app.agent.enabled);
        if (app.game.state() != GState::PLAYING && keyboard.start) {
            app.startRound();
            if (app.agent.enabled) app.game.markAssisted();
        }
        GameInput agentInput;
#if !defined(__EMSCRIPTEN__)
        if (app.stdioAgent && !requestPythonInput(app.game, dt, app.agent.enabled, app.round, agentInput)) {
            app.stdioAgent = false;
            app.agent.available = false;
            app.setAgentEnabled(false);
        }
#else
        agentInput = app.pendingInput;
        app.pendingInput = {};
#endif
        if (app.agent.enabled) {
            app.game.markAssisted();
            app.game.update(dt, agentInput);
        } else {
            keyboard.start = false; // Already handled above, including round numbering.
            app.game.update(dt, keyboard);
        }
    }
    BeginDrawing();
    ClearBackground(COL_BG);
    app.renderer.drawFrame(app.game, (float)GetTime(), app.agent);
    EndDrawing();
#if defined(__EMSCRIPTEN__)
    publishBrowserStatus((int)app.game.state(), app.agent.enabled, app.agent.wpm,
                         app.game.score(), app.game.completedWords(), focused, app.round);
    if (focused && app.agent.enabled) {
        const auto observation = gameObservation(app.game, dt, true, app.round);
        publishBrowserObservation(observation.c_str());
    }
#endif
}

struct Options {
    bool stdioAgent = false;
    bool simulate = false;
    bool help = false;
    float wpm = 120.0f;
    double seconds = 300.0;
    unsigned int seed = std::random_device{}();
};

static double number(const std::string& value, const std::string& flag) {
    size_t consumed = 0;
    double result = std::stod(value, &consumed);
    if (consumed != value.size() || !std::isfinite(result)) throw std::invalid_argument("Invalid value for " + flag);
    return result;
}

static Options parseOptions(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string flag = argv[i];
        if (flag == "--agent-stdio") options.stdioAgent = true;
        else if (flag == "--simulate") options.simulate = true;
        else if (flag == "--help" || flag == "-h") options.help = true;
        else if (flag == "--agent-wpm" || flag == "--seconds" || flag == "--seed") {
            if (++i >= argc) throw std::invalid_argument("Missing value for " + flag);
            const std::string value = argv[i];
            if (flag == "--seed") {
                size_t consumed = 0;
                const auto seed = std::stoull(value, &consumed);
                if (value.empty() || value[0] == '-' || consumed != value.size() || seed > std::numeric_limits<unsigned int>::max())
                    throw std::invalid_argument("Seed must be an unsigned 32-bit integer");
                options.seed = (unsigned int)seed;
            } else if (flag == "--agent-wpm") {
                const double wpm = number(value, flag);
                if (wpm < 1 || wpm > 600) throw std::invalid_argument("Agent speed must be between 1 and 600 WPM");
                options.wpm = (float)wpm;
            } else {
                options.seconds = number(value, flag);
                if (options.seconds <= 0 || options.seconds > 86400) throw std::invalid_argument("Invalid simulation duration (0 < seconds <= 86400)");
            }
        } else throw std::invalid_argument("Unknown option: " + flag + ". Run the agent with python3 src/agent/run.py");
    }
    if (options.simulate && !options.stdioAgent) throw std::invalid_argument("Run simulations with python3 src/agent/run.py --simulate");
    return options;
}

#if !defined(__EMSCRIPTEN__)
static int simulate(const Options& options) {
    Game game(options.seed);
    game.init(false);
    game.start();
    game.markAssisted();
    double elapsed = 0;
    int keystrokes = 0;
    while (elapsed < options.seconds && game.state() == GState::PLAYING) {
        const float dt = (float)std::min(1.0 / 60.0, options.seconds - elapsed);
        GameInput input;
        if (!requestPythonInput(game, dt, true, 1, input)) return 1;
        if (!input.text.empty() || input.backspace) ++keystrokes;
        game.update(dt, input);
        elapsed += dt;
    }
    std::printf("{\"kind\":\"result\",\"seed\":%u,\"wpm\":%.2f,\"seconds\":%.3f,\"score\":%d,"
                "\"completed_words\":%d,\"keystrokes\":%d,\"game_over\":%s}\n",
                options.seed, options.wpm, elapsed, game.score(), game.completedWords(),
                keystrokes, game.state() == GState::GAMEOVER ? "true" : "false");
    return 0;
}
#endif

int main(int argc, char** argv) {
    Options options;
    try { options = parseOptions(argc, argv); }
    catch (const std::exception& error) { std::fprintf(stderr, "%s\nUse --help for usage.\n", error.what()); return 1; }
    if (options.help) {
        std::puts("Usage: racest [--seed N]\nAutonomous play: python3 src/agent/run.py [--wpm 120]\n"
                  "Simulation: python3 src/agent/run.py --simulate [--seconds 300] [--seed N]\n"
                  "--agent-stdio is the Python launcher's internal stdin/stdout interface.");
        return 0;
    }
#if !defined(__EMSCRIPTEN__)
    if (options.simulate) return simulate(options);
    if (options.stdioAgent) SetTraceLogLevel(LOG_NONE); // Reserve stdout for JSON observations.
#endif
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(SW, SH, "RACEST");
    SetTargetFPS(60);
    static Application app(options.seed, options.wpm, options.stdioAgent);
    app.game.init();
    app.setAgentEnabled(options.stdioAgent);
#if defined(__EMSCRIPTEN__)
    browserApp = &app;
    emscripten_set_main_loop_arg(updateFrame, &app, 0, true);
#else
    while (!WindowShouldClose()) updateFrame(&app);
    CloseWindow();
#endif
    return 0;
}

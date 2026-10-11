#include "game_bridge.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#if !defined(__EMSCRIPTEN__) && !defined(_WIN32)
#include <poll.h>
#endif

static std::string jsonString(const std::string& value) {
    std::ostringstream out;
    out << '"';
    for (unsigned char ch : value) {
        if (ch == '"' || ch == '\\') out << '\\' << ch;
        else if (ch < 32 || ch >= 127)
            out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)ch << std::dec;
        else out << ch;
    }
    out << '"';
    return out.str();
}

std::string gameObservation(const Game& game, float dt, bool enabled, int round) {
    const char* state = game.state() == GState::PLAYING ? "playing" :
                        game.state() == GState::MENU ? "menu" : "gameover";
    std::ostringstream out;
    out << std::setprecision(9) << std::boolalpha;
    out << "{\"kind\":\"observation\",\"state\":" << jsonString(state)
        << ",\"dt\":" << dt << ",\"enabled\":" << enabled << ",\"round\":" << round
        << ",\"elapsed\":" << game.timeElapsed() << ",\"score\":" << game.score()
        << ",\"completed_words\":" << game.completedWords()
        << ",\"screen\":{\"width\":" << SW << ",\"danger_x\":" << DANGER_X
        << ",\"font_size\":" << FONT_SIZE << "},\"words\":[";
    bool first = true;
    for (const auto& word : game.words()) {
        if (!first) out << ',';
        first = false;
        out << "{\"text\":" << jsonString(word.text) << ",\"typed\":" << word.typed
            << ",\"errored\":" << word.errored << ",\"x\":" << word.x
            << ",\"y\":" << word.y << ",\"speed\":" << word.speed
            << ",\"targeted\":" << word.targeted << '}';
    }
    out << "]}";
    return out.str();
}

#if !defined(__EMSCRIPTEN__)
bool requestPythonInput(const Game& game, float dt, bool enabled, int round, GameInput& input) {
    std::cout << gameObservation(game, dt, enabled, round) << std::endl;
#if !defined(_WIN32)
    pollfd descriptor{0, POLLIN, 0};
    if (poll(&descriptor, 1, 2000) <= 0) {
        std::cerr << "Python agent did not reply within two seconds; returning control to the player.\n";
        return false;
    }
#endif
    std::string command;
    if (!std::getline(std::cin, command)) {
        std::cerr << "Python agent disconnected; returning control to the player.\n";
        return false;
    }
    if (command == "idle") return true;
    if (command == "backspace") { input.backspace = true; return true; }
    if (command.size() == 6 && command.compare(0, 5, "type ") == 0 &&
        command[5] >= 'a' && command[5] <= 'z') {
        input.text = command[5];
        return true;
    }
    std::cerr << "Invalid Python agent command.\n";
    return false;
}
#endif

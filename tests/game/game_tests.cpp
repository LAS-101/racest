#include "game.h"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

static void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

static void inputTests() {
    Game game(42);
    game.init(false);
    GameInput start;
    start.start = true;
    game.update(0, start);
    require(game.state() == GState::PLAYING, "Start input failed");
    game.update(1);
    const auto text = game.words().front().text;
    game.update(0, {text.substr(0, 1)});
    require(game.words().front().targeted && game.words().front().typed == 1, "First letter did not lock target");
    const char wrong = text[1] == 'z' ? 'x' : 'z';
    game.update(0, {std::string(1, wrong)});
    require(game.words().front().errored, "Wrong character did not cause error");
    GameInput recovery;
    recovery.backspace = true;
    game.update(0, recovery);
    require(!game.words().front().errored && game.words().front().typed == 1, "Backspace recovery changed progress");
    game.update(0, {text.substr(1)});
    require(game.words().empty() && game.score() > 0 && game.completedWords() == 1, "Completion failed");

    game.start();
    game.update(1);
    const auto nextText = game.words().front().text;
    game.update(0, {nextText.substr(0, 1)});
    GameInput backspace;
    backspace.backspace = true;
    game.update(0, backspace);
    require(!game.words().front().targeted && game.words().front().typed == 0, "Backspace failed to unlock target");
    game.update(0, {nextText});
    require(game.completedWords() == 1, "Queued keyboard input failed to complete word");
}

static void scoreTests() {
    // Isolate persistence tests from the player's word bank and saved score.
    const auto original = std::filesystem::current_path();
    const auto dir = std::filesystem::temp_directory_path() /
                     ("racest-agent-tests-" + std::to_string(std::random_device{}()));
    std::filesystem::create_directory(dir);
    std::filesystem::current_path(dir);
    try {
        std::filesystem::create_directory("data");
        std::ofstream("data/highscore.dat") << 0;
        for (bool assisted : {true, false}) {
            // Human scoring must recreate the runtime directory on a fresh run.
            if (!assisted) std::filesystem::remove_all("data");
            Game game(42);
            game.init();
            game.start();
            game.update(1);
            const auto text = game.words().front().text;
            game.update(0, {text});
            require(game.score() > 0, "Score fixture failed");
            if (assisted) game.markAssisted();
            game.update(4); // Spawn the next word before letting it cross the line.
            game.update(100);
            require(game.state() == GState::GAMEOVER, "Score fixture failed to end");
            int saved = -1;
            std::ifstream("data/highscore.dat") >> saved;
            require(assisted ? saved == 0 : saved == game.score(), "High score persistence policy failed");
            require(assisted ? !game.newRecord() : game.newRecord(), "Record indicator incorrect");
            game.start();
            require(!game.assisted() && game.completedWords() == 0, "New round failed to reset flags");
        }
    } catch (...) {
        std::filesystem::current_path(original);
        std::filesystem::remove_all(dir);
        throw;
    }
    std::filesystem::current_path(original);
    std::filesystem::remove_all(dir);
}

int main() {
    try {
        inputTests();
        scoreTests();
        std::cout << "Game tests passed: keyboard input, error recovery, and score isolation.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}

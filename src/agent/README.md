# RACEST Python agent

All target selection, urgency calculations, timing, and error correction run
in Python. The existing C++ game remains the source of the rules, physics,
scoring, and rendering. No AI service, API key, or third-party Python package
is required for normal use. This is a deterministic agent, not a trained model.

## Desktop

From the repository root, build the game and launch the Python agent:

```bash
make
python3 src/agent/run.py
# Equivalent shortcut:
make agent
# Adjustable speed, 1–600 standard words per minute:
python3 src/agent/run.py --wpm 180
```

The launcher starts `build/native/racest` automatically and uses
`assets/words.txt`. Desktop scores are saved to `data/highscore.dat`.
Press **F2** to take control or resume the Python agent. Press Enter to restart
after a loss. Close the window or press Escape to exit. Ctrl+C in the terminal
stops the launcher and its child game process. Running `./build/native/racest` alone starts
the normal human game; launch through Python to connect an agent.

## Headless evaluation

```bash
python3 src/agent/run.py --simulate --seconds 600 --wpm 120 --seed 42
# Short default evaluation (300 simulated seconds):
make simulate
```

The simulation advances the actual C++ game at 60 steps per simulated second,
with Python selecting each action through the same interface used for desktop
play. It stops after the specified duration or a loss and prints a JSON result
with score, completed words, survival time, typing speed, seed, and keystrokes.
It runs faster than real time and never reads or writes your high score.
Seeds are reproducible on the same build and word bank. Maximum duration is
86400 seconds. Use `--binary /path/to/racest` to use a CMake-built executable.

## Online vocabulary

By default, agent runs and simulations use the bundled word bank for repeatable
results. Add `--online-words` to download/cache the internet dataset:

```bash
make agent-online
python3 src/agent/run.py --simulate --online-words --seconds 600 --seed 42
python3 src/agent/run.py --online-words --refresh-words
```

The same words are supplied to the real game and observed by the Python agent.
Downloads happen before play and fall back to cached or bundled words offline.

## Browser

Build the WebAssembly game with Emscripten, then start the Python server:

```bash
make web
python3 src/agent/run.py --web --online-words
# Or build and serve in one command:
make serve-web
```

Open **http://localhost:8000** and click **Start agent**. Use the speed selector,
**Take control**, or F2. The game and agent pause when the canvas loses focus.
You can choose another port or initial speed:

```bash
python3 src/agent/run.py --web --online-words --port 8001 --wpm 180
```

The server binds to localhost and serves the game and Python API from the same
origin. Browser JavaScript only exchanges observations and input commands;
target selection runs in `policy.py`. A plain static host supports human play,
but needs this Python server for autonomous play. If the connection fails,
the game returns control to the player. Requests from an earlier round, speed,
or control mode are discarded, so stale replies cannot keep typing after takeover.

## Contents

| File | Purpose |
| --- | --- |
| `policy.py` | Python agent, target selection, pacing, and Backspace recovery |
| `run.py` | Desktop launcher, headless evaluator, and browser-server CLI |
| `server.py` | Local browser host and Python decision API |
| `../game/game_bridge.h/.cpp` | C++ state serialization and native command transport |
| `tests/agent/test_agent.py` (repository root) | Python policy and real-game integration tests |
| `tests/agent/browser_smoke.py` (repository root) | Optional end-to-end Chromium test |

The policy waits until words fit on screen, prioritizes the selectable word
with the tightest completion deadline, respects first-letter locking, finishes
an existing target, and corrects errors. Standard WPM counts five characters
per word. It sends at most one action per frame; frame rate or browser API
latency can limit effective speed. Enabling the agent marks the entire round
as assisted, keeping your personal high score separate.

## Tests

```bash
make test
# Python tests alone (build the native game first):
python3 -m unittest discover -s tests/agent -v
# Optional: requires an existing Playwright + Chromium installation and web build:
python3 tests/agent/browser_smoke.py
```

The suite checks target selection, duplicate initials, cadence, offscreen
words, corrections, three ten-minute runs, normal losses, seed reproducibility,
option validation, disconnected agents, and high-score preservation. Browser
checks cover live Python decisions, speed changes, focus, and takeover, and
save a screenshot to `build/agent-browser.png`.

CMake users can run `ctest --test-dir build/native --output-on-failure` after
building a native configuration. It uses that build's executable for Python tests.

## Native protocol

`run.py` launches the game with the internal `--agent-stdio` flag. The game
writes one JSON observation per frame to stdout and waits for one line on stdin:
`idle`, `type a` (one lowercase letter), or `backspace`. State includes the
current round, enabled flag, timestep, screen constants, words, typing progress,
errors, positions, and speeds. The game applies commands through its existing
input handler. Python cannot edit game state or scores. Simulation finishes
with a `kind: "result"` JSON message. EOF, invalid commands, or a two-second
reply timeout on POSIX disconnect the agent; simulation exits with an error,
and desktop play returns to the human.

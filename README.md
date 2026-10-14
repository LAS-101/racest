# RACEST — A Space Typing Game

A fast-paced typing game where words race across the screen from right to left. Type them before they reach the danger zone on the left!

## Overview

**RACEST** is a real-time typing challenge game built with C++ and [raylib](https://www.raylib.com/). Words spawn randomly on the screen and move leftward at increasing speeds. Players must type each word's first letter to lock onto it, then complete the word by typing the remaining characters before it crosses the danger line on the left.

- **Dynamic difficulty**: Word speed and spawn rate increase over time
- **Scoring system**: Quick completions earn more points
- **High score tracking**: Persistent scores saved locally
- **Progressive challenge**: Word length preferences adapt to player performance time

## Run with Docker

With Docker Engine and the Compose plugin installed (or Docker Desktop), run:

```bash
docker compose up --build
```

Open **http://localhost:8000**. Play with your keyboard or click **Start agent**.
The container builds the WebAssembly game and runs the Python agent and online
word service. You do not need a local C++ compiler, raylib, Python, or Emscripten.
The first build downloads the compiler image and raylib; later builds reuse layers.

The **Dockerfile** builds the web and simulation images, while **Compose**
records the port, command, and persistent
word-cache volume. The browser interface provides the game and agent together
without forwarding a desktop display into a container. Multi-stage builds keep
compilers out of the runtime images. Only the web service starts by default;
the simulation service is enabled explicitly.

Press Ctrl+C to stop, then remove the stopped containers with:

```bash
docker compose down
```

The named `word-cache` volume survives `down`; browser high scores stay in your
browser's local storage. Downloaded words use the existing offline fallback.
The published port is bound to localhost. To change the port or initial agent speed:

```bash
RACEST_PORT=8080 RACEST_WPM=180 docker compose up --build
```

For a headless agent evaluation, including the native build's tests:

```bash
docker compose --profile tools run --build --rm simulation
docker compose --profile tools run --rm simulation --simulate --seconds 600 --wpm 180 --seed 42
docker compose --profile tools run --rm simulation --simulate --online-words --seconds 60
```

The simulation image has no display requirement. Its default run uses the bundled
word bank for reproducibility and prints a JSON result. Add `--online-words` to
use the shared dataset cache. Refresh that cache with:

```bash
docker compose run --rm --entrypoint python game src/words/run.py --download-only --refresh-words
docker compose restart game
```

Reload the browser after refreshing. For bundled browser words only, stop the
regular service and run:

```bash
docker compose run --rm --service-ports game --web --host 0.0.0.0
```

You can use the Dockerfile directly as well:

```bash
docker build --platform linux/amd64 --target web -t racest:web .
docker run --rm --init -p 127.0.0.1:8000:8000 -v racest-words:/app/data racest:web
```

The web service uses `linux/amd64` to match the pinned Emscripten image; Docker
Desktop can emulate it on Apple Silicon, with a slower first build. The simulation
image builds natively for the selected platform. Both runtime images run as a
non-root user. `.dockerignore` excludes local build outputs, cached data, and
machine-specific configuration from the build context.

## Building

### Requirements

- **C++17** compiler (g++, clang)
- **CMake 3.16+** and **Make**
- **raylib 5.5** (CMake uses an installed copy or downloads it)
- **Python 3** for the agent and Python tests
- Linux development libraries: `libX11-dev`, `libxrandr-dev`, `libxi-dev`, `libxcursor-dev`


## Running the Game

### Browser (WebAssembly)

The same C++ game runs in a browser using Emscripten and raylib's web backend.
Install Git, CMake (3.16+), Make, Python 3, and the
[Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html).
For example, install the tested compiler version in a directory of your choice:

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install 4.0.23
./emsdk activate 4.0.23
source ./emsdk_env.sh
```

Then, from the RACEST repository in that same terminal:

```bash
make serve-web
```

Open **http://localhost:8000**. Click the game and press Enter or Space to start.
Use a desktop browser with WebAssembly, WebGL, and a physical keyboard. Switching
tabs/windows pauses gameplay. Press Ctrl+C in the terminal to stop the server.
Source `/path/to/emsdk/emsdk_env.sh` again when opening a new terminal.

To build without starting the server:

```bash
make web
# Equivalent commands (also usable without Make):
emcmake cmake -S . -B build/web -DCMAKE_BUILD_TYPE=Release
cmake --build build/web --parallel
```

The first build downloads raylib **5.5** and compiles it for WebAssembly; a native
raylib installation is not needed for this target. `assets/words.txt` is bundled in the
generated data file. Rebuilding after editing it updates the bundled word list.

To publish, upload **all four files** from `build/web/public/` (`index.html`,
`index.js`, `index.wasm`, and `index.data`) together to a static web host such as
GitHub Pages or itch.io (HTML game). Serve them over HTTP/HTTPS; opening
`index.html` directly with `file://` will not work. The host should serve `.wasm`
as `application/wasm`. Human play requires no backend or cross-origin isolation headers.
The Python agent requires its local server; use `python3 src/agent/run.py --web`.

Browser high scores use `localStorage` for the current site and browser profile,
separately from desktop `data/highscore.dat`. Clearing site data clears the score;
when storage is blocked, the score remains available only for the current session.

### Desktop

From the repository root:

```bash
make run        # Fetch/cache online words and play
make run-local  # Play with the original bundled vocabulary
# Build without opening a window:
make
./build/native/racest
```

Make delegates compilation to CMake. Native executables and intermediate files
stay in `build/native/`; the WebAssembly bundle stays in `build/web/public/`.
For a direct CMake build:

```bash
cmake -S . -B build/native -DCMAKE_BUILD_TYPE=Release
cmake --build build/native --parallel 2
```

`make run` loads a vocabulary from an internet dataset before starting the
game. Downloads are cached in `data/`; gameplay picks words randomly from that
vocabulary without network requests during a round. Cached or bundled words
keep the game playable offline. Use `make run-local` for the original vocabulary.

The game opens at 1280×720 pixels. Run the executable from the repository root to use
`assets/words.txt` and `data/highscore.dat`. CMake also copies the word bank to
`build/native/assets/` for runs from that build directory. If the word bank is
missing, the game uses its built-in list. The game creates `data/` when saving
scores. Saved scores are local runtime data and are ignored by Git.

## Internet Word Dataset

The online source is the [Google 10,000 English words dataset](https://github.com/first20hours/google-10000-english),
using its `google-10000-english-no-swears.txt` list. RACEST keeps unique lowercase
ASCII words with 3–12 letters so they fit the controls and difficulty curve.
The downloaded list is stored separately from your original `assets/words.txt`.

```bash
make run           # Human play with online words
make agent-online  # Python agent with online words
make refresh-words # Force a new download without opening a window
make serve-web     # Browser game + Python agent + online vocabulary
```

A valid cache is reused for 24 hours. If a refresh fails, RACEST uses the previous
cache or the bundled list. Downloads have an eight-second timeout and happen
before play. `data/words.online.json` records the source URL, timestamp, and word
count; all downloaded data is ignored by Git. Simulations use the original
vocabulary unless you explicitly pass `--online-words`. See
[the word-source README](src/words/README.md) for commands and behavior.

## Python Typing Agent

The autonomous agent lives in **`src/agent/`**. Its decisions, pacing, launcher,
browser server, and tests are written in Python using the standard library.
The C++ game provides state observations and accepts normal typing commands.

```bash
make agent                              # Watch the Python agent play
python3 src/agent/run.py --wpm 180        # Choose a typing speed
make simulate                           # Evaluate without opening a window
python3 src/agent/run.py --web            # Serve the built browser game + agent
make test                               # Game and Python agent tests
```

Press **F2** to take control or resume the connected agent. Agent-assisted
rounds do not update personal high scores. Browser automation requires the
Python server; human play still works on a static host.

See [the agent README](src/agent/README.md) for setup, evaluation options, and
the connection protocol.

## How to Play

### Menu Screen

Press **Enter** or **Space** to start a new game.

### Gameplay

- **Type the first letter** of a word to lock onto it (the word highlights in white)
- **Complete the word** by typing all remaining letters
- Successfully typed characters appear in **green**
- **Errors** appear in **red** — press **Backspace** to clear and retry
- **Danger zone**: Words crossing the red line on the left end the game
- Words get faster and closer as time progresses

### After Game Over

Press **Enter** or **Space** to play again.

## Game Features

- **Twinkling starfield** background
- **Color-coded text**:
  - Lavender: untyped words
  - Green: correctly typed characters
  - Red: error state
  - White/cyan: locked target word
  - Gold: high score display
- **Real-time HUD**: Score, time, and best score
- **Adaptive difficulty**: Word selection based on elapsed time
- **Persistent high scores**: Saved to `data/highscore.dat` on desktop or browser local storage on web

## Project Structure

```text
.
├── CMakeLists.txt          # Native and WebAssembly build definitions
├── Dockerfile              # Web and headless simulation images
├── compose.yaml            # Startup, ports, and persistent cache
├── Makefile                # Build, run, test, and serve shortcuts
├── README.md
├── assets/
│   └── words.txt           # Shared game word bank
├── data/
│   ├── highscore.dat       # Local saved score (ignored by Git)
│   ├── words.online.txt    # Downloaded vocabulary (ignored by Git)
│   └── words.online.json   # Dataset provenance (ignored by Git)
├── src/
│   ├── game/               # C++ game and agent transport
│   │   ├── main.cpp
│   │   ├── game.h / game.cpp
│   │   ├── renderer.h / renderer.cpp
│   │   ├── types.h
│   │   └── game_bridge.h / game_bridge.cpp
│   ├── agent/              # Python decisions and launch tools
│   │   ├── policy.py
│   │   ├── run.py
│   │   ├── server.py
│   │   └── README.md
│   └── words/              # Internet dataset download, cache, and game launcher
├── tests/
│   ├── game/               # C++ input and score tests
│   ├── agent/              # Python policy, integration, and browser tests
│   └── words/              # Dataset validation and offline fallback tests
├── web/
│   └── shell.html          # Browser page and controls
└── build/                  # Generated output (ignored by Git)
```

## Technical Details

### Architecture

- **Modular design**: Separated game logic (`src/game/game.cpp`), rendering (`src/game/renderer.cpp`), and types (`src/game/types.h`)
- **Game state**: Owned by the `Game` class; `Renderer` draws it and the Python agent observes it through `game_bridge`.
- **Spawn rate scaling**: Gradually reduces spawn interval from 3.3s to 1.1s
- **Speed curve**: Word speed ramps from ~88 px/s to ~270 px/s
- **Word selection**: Pool filtered by length, preferring longer words as time progresses

### Key Functions

- `Game::init()`: Load word bank and high score
- `Game::update(dt, input)`: Main update loop (input, physics, spawning)
- `Game::start()`: Reset state and begin playing
- `Renderer::drawFrame(game, now, agent)`: Render all visual elements

## Controls

 - **A-Z**: Type letters to lock on and complete words
 - **Backspace**: Remove last typed character or clear error
 - **Enter / Space**: Start game or replay after game over
 - **Esc**: Close window on desktop (raylib default); close the tab to exit on web
 - **F2**: Enable the agent / take control

## Customization

### Colors

Edit the color constants in `src/game/types.h`.

### Difficulty

Adjust these in `src/game/game.cpp`:

- `wordSpeed()`: Change the speed ramp curve
- `spawnInterval()`: Adjust how often new words appear
- `pickWord()`: Modify word length preferences
- `MAX_WORDS`: Maximum simultaneous words on screen (default: 7)

### Screen Size

Modify in `src/game/types.h`:

```cpp
inline constexpr int SW = 1280;  // screen width
inline constexpr int SH = 720;   // screen height
```

## Performance

- Targets 60 FPS with V-Sync enabled
- Uses 4× MSAA antialiasing for smooth graphics
- Efficient text measurement and caching via raylib

## Dependencies

- **raylib**: Graphics and input handling
- **C++ Standard Library**: `<vector>`, `<string>`, `<random>`, `<fstream>`, `<algorithm>`, `<cmath>`

## Building with Your Own Word List

Replace or supplement `assets/words.txt` (one word per line, alphanumeric only). The game automatically filters and normalizes words (3+ characters, lowercase).

## License

This project is provided as-is for educational and personal use.

## Credits

Built with [raylib](https://www.raylib.com/) — a simple and easy-to-use library to enjoy videogames programming.

---

**Enjoy the game and happy typing!**

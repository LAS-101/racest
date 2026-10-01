# RACEST — A Space Typing Game

A fast-paced typing game where words race across the screen from right to left. Type them before they reach the danger zone on the left!

## Overview

**RACEST** is a real-time typing challenge game built with C++ and [raylib](https://www.raylib.com/). Words spawn randomly on the screen and move leftward at increasing speeds. Players must type each word's first letter to lock onto it, then complete the word by typing the remaining characters before it crosses the danger line on the left.

- **Dynamic difficulty**: Word speed and spawn rate increase over time
- **Scoring system**: Quick completions earn more points
- **High score tracking**: Persistent scores saved locally
- **Progressive challenge**: Word length preferences adapt to player performance time

## Building

### Requirements

- **C++17** compiler (g++, clang)
- **raylib 4.x or 5.x**
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
raylib installation is not needed for this target. `Words.txt` is bundled in the
generated data file. Rebuilding after editing it updates the bundled word list.

To publish, upload **all four files** from `build/web/public/` (`index.html`,
`index.js`, `index.wasm`, and `index.data`) together to a static web host such as
GitHub Pages or itch.io (HTML game). Serve them over HTTP/HTTPS; opening
`index.html` directly with `file://` will not work. The host should serve `.wasm`
as `application/wasm`. No backend or cross-origin isolation headers are required.

Browser high scores use `localStorage` for the current site and browser profile,
separately from desktop `highscore.dat`. Clearing site data clears the score;
when storage is blocked, the score remains available only for the current session.

### Desktop

From the repository root, compile the sources in the `src/` directory:

```bash
g++ src/main.cpp src/game.cpp src/renderer.cpp -o racest \
    -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -std=c++17
```

```bash
./racest
```

The game window opens at 1280×720 pixels. A `Words.txt` file (containing one word per line) should be placed next to the executable (the repository contains `Words.txt` at the project root). If missing, the game uses a built-in fallback word list.

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
- **Persistent high scores**: Saved to `highscore.dat` on desktop or browser local storage on web

## Project Structure

```
.
├── Makefile
├── CMakeLists.txt  # Native / Emscripten build, fetches raylib 5.5
├── README.md
├── web/shell.html # Browser page and canvas
├── Words.txt       # Word list (one word per line)
├── highscore.dat   # Saved high score (auto-created when running)
└── src/
    ├── main.cpp        # Entry point, window setup
    ├── game.h          # Game state and declarations
    ├── game.cpp        # Game logic, input handling, physics
    ├── types.h         # Color palette, constants, structs (Star, Word, GState)
    ├── renderer.h      # Rendering declarations
    └── renderer.cpp    # Drawing logic (HUD, words, menu, game over)
```

## Technical Details

### Architecture

- **Modular design**: Separated game logic (`src/game.cpp`), rendering (`src/renderer.cpp`), and types (`src/types.h`)
- **Global game state**: Managed through extern variables (g_words, g_state, g_score, etc.)
- **Spawn rate scaling**: Gradually reduces spawn interval from 3.3s to 1.1s
- **Speed curve**: Word speed ramps from ~88 px/s to ~270 px/s
- **Word selection**: Pool filtered by length, preferring longer words as time progresses

### Key Functions

- `initGame()`: Load word bank and high score
- `updateGame(dt)`: Main update loop (input, physics, spawning)
- `startGame()`: Reset state and begin playing
- `drawFrame(now)`: Render all visual elements

## Controls

 - **A-Z**: Type letters to lock on and complete words
 - **Backspace**: Remove last typed character or clear error
 - **Enter / Space**: Start game or replay after game over
 - **Esc**: Close window on desktop (raylib default); close the tab to exit on web

## Customization

### Colors

Edit the color constants in `src/types.h`.

### Difficulty

Adjust these in `src/game.cpp`:

- `wordSpeed()`: Change the speed ramp curve
- `spawnInterval()`: Adjust how often new words appear
- `pickWord()`: Modify word length preferences
- `MAX_WORDS`: Maximum simultaneous words on screen (default: 7)

### Screen Size

Modify in `src/types.h`:

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

Replace or supplement `Words.txt` (one word per line, alphanumeric only). The game automatically filters and normalizes words (3+ characters, lowercase).

## License

This project is provided as-is for educational and personal use.

## Credits

Built with [raylib](https://www.raylib.com/) — a simple and easy-to-use library to enjoy videogames programming.

---

**Enjoy the game and happy typing!**

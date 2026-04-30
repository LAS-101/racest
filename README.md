# RACEST — A Space Typing Game

A fast-paced typing game where words race across the screen from right to left. Type them before they reach the danger zone on the left!

## Overview

**RACEST** is a real-time typing challenge game built with C++ and [raylib](https://www.raylib.com/). Words spawn randomly on the screen and move leftward at increasing speeds. Players must type each word's first letter to lock onto it, then complete the word by typing the remaining characters before it crosses the danger line.

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
```bash
g++ main.cpp game.cpp renderer.cpp -o racest \
    -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -std=c++17
```

```bash
./racest
```

The game window opens at 1280×720 pixels. A `words.txt` file (containing one word per line) should be in the same directory as the executable. If missing, the game uses a built-in fallback word list.

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
- **Persistent high scores**: Saved to `highscore.dat`

## Project Structure

```
.
├── main.cpp        # Entry point, window setup
├── game.h          # Game state and declarations
├── game.cpp        # Game logic, input handling, physics
├── types.h         # Color palette, constants, structs (Star, Word, GState)
├── renderer.h      # Rendering declarations
├── renderer.cpp    # Drawing logic (HUD, words, menu, game over)
├── words.txt       # Word list (one word per line)
├── highscore.dat   # Saved high score (auto-created)
└── README.md       # This file
```

## Technical Details

### Architecture

- **Modular design**: Separated game logic (game.cpp), rendering (renderer.cpp), and types
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

| Key | Action |
|-----|--------|
| **A-Z** | Type letters to lock on and complete words |
| **Backspace** | Remove last typed character or clear error |
| **Enter / Space** | Start game or replay after game over |
| **Esc** | Close window (raylib default) |

## Customization

### Colors

Edit the color constants in [types.h](types.h):

```cpp
inline const Color COL_BG      = {  0,   0,   0, 255};   // black background
inline const Color COL_TYPED   = { 68, 198, 110, 255};   // green typed
// ... etc
```

### Difficulty

Adjust these in [game.cpp](game.cpp):

- `wordSpeed()`: Change the speed ramp curve
- `spawnInterval()`: Adjust how often new words appear
- `pickWord()`: Modify word length preferences
- `MAX_WORDS`: Maximum simultaneous words on screen (default: 7)

### Screen Size

Modify in [types.h](types.h):

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

Replace or supplement [words.txt](words.txt) (one word per line, alphanumeric only). The game automatically filters and normalizes words (3+ characters, lowercase).

## License

This project is provided as-is for educational and personal use.

## Credits

Built with [raylib](https://www.raylib.com/) — a simple and easy-to-use library to enjoy videogames programming.

---

**Enjoy the game and happy typing!**

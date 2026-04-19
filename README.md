# Medical Deckbuilder

A roguelike deckbuilding game where you play as the immune system, defending the human body against disease and infection.

## Prerequisites

- **CMake** 3.20 or later
- **C++17 compiler** — one of:
  - GCC 7+ (Linux / MinGW on Windows)
  - Clang 5+ (Linux / macOS)
  - MSVC 2019+ (Windows)
- **git** (to clone the repository)
- **Internet connection** on first build (CMake fetches raylib and nlohmann/json automatically)

## Build

```bash
# Clone the repository
git clone <your-repo-url>
cd medical-deckbuilder

# Create and enter the build directory
mkdir build
cd build

# Configure (downloads dependencies automatically)
cmake ..

# Compile
cmake --build .
```

> **Note:** The first configure step downloads raylib 4.5.0 and nlohmann/json 3.11.2 via CMake's FetchContent. No manual installation is required.

## Run

```bash
# Linux / macOS
./medical_deckbuilder

# Windows
.\medical_deckbuilder.exe
```

## Clean Build

```bash
cd build
cmake --build . --clean-first
```

Or delete the `build/` directory and repeat the build steps.

## Project Structure

```
.
├── CMakeLists.txt
├── assets/
│   ├── decks/
│   │   └── player/     # Player card library + starter deck config
│   └── enemies/        # Enemy definitions and their deck data
├── src/
│   ├── config/         # Shared constants and asset paths
│   ├── content/        # JSON-driven content loaders / factories
│   ├── core/           # Cards, decks, player, enemy, game state
│   ├── gameplay/       # Card effects, combat resolution, statuses
│   ├── ui/             # Rendering, UI state, input wrappers, art cache
│   └── main.cpp        # Application entry point
└── tests/              # Lightweight gameplay regression tests
```

## Architecture Notes

- Cards are data-driven and resolve through an effect list instead of hardcoded `attack/block` branches.
- Enemies load from JSON definitions, so adding a new enemy no longer requires new constructor constants in code.
- Shared gameplay and layout constants live in [`src/config/Defines.h`](src/config/Defines.h).
- Card art is cached in the UI layer instead of being copied around with gameplay data.

## Dependencies (auto-fetched by CMake)

| Library | Version | Purpose |
|---------|---------|---------|
| [raylib](https://github.com/raysan5/raylib) | 4.5.0 | Graphics, input, audio |
| [nlohmann/json](https://github.com/nlohmann/json) | 3.11.2 | JSON data loading |

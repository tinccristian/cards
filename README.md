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
├── CMakeLists.txt      # Build configuration (self-contained, fetches all deps)
├── README.md
├── src/
│   ├── main.cpp        # Entry point — opens the raylib window
│   └── ui/
│       └── Colors.h    # Central color palette (raylib Color constants)
├── assets/             # JSON data files (cards, enemies, locations)
└── tests/              # Unit tests
```

## Dependencies (auto-fetched by CMake)

| Library | Version | Purpose |
|---------|---------|---------|
| [raylib](https://github.com/raysan5/raylib) | 4.5.0 | Graphics, input, audio |
| [nlohmann/json](https://github.com/nlohmann/json) | 3.11.2 | JSON data loading |

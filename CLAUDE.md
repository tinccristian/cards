# Medical Deckbuilding Game - Project Guide

## Vision

A roguelike deckbuilding game where you play as the immune system defending the human body. Navigate body locations (Heart, Brain, Lungs, etc.), build a deck of medical interventions, unlock synergies between treatments.

**Inspiration**: Slay the Spire (run progression, shop system) + medical theme

**Core Loop**: Enter location → Fight pathogens with cards → Visit shop → Progress through body → Victory

## Tech Stack

- **Language**: C++17
- **Graphics**: raylib
- **Build**: CMake (cross-platform)
- **Data**: JSON (cards, enemies, locations)
- **Code**: Clean architecture, type safety, RAII, smart pointers

## Project Structure

.
├── CMakeLists.txt
├── README.md
├── claude.md
├── src/
│   ├── main.cpp
│   ├── core/
│   │   ├── Card.h / Card.cpp
│   │   ├── Deck.h / Deck.cpp
│   │   ├── Player.h / Player.cpp
│   │   ├── Enemy.h / Enemy.cpp
│   │   ├── GameState.h / GameState.cpp
│   │   ├── Effect.h / Effect.cpp
│   │   └── Combat.h / Combat.cpp
│   ├── ui/
│   │   ├── Renderer.h / Renderer.cpp
│   │   ├── InputHandler.h / InputHandler.cpp
│   │   ├── UIState.h / UIState.cpp
│   │   └── Colors.h
│   ├── shop/
│   │   ├── Shop.h / Shop.cpp
│   │   └── ShopUI.h / ShopUI.cpp
│   ├── world/
│   │   ├── Location.h / Location.cpp
│   │   ├── World.h / World.cpp
│   │   └── Encounter.h / Encounter.cpp
│   └── utils/
│       ├── Config.h / Config.cpp
│       ├── JSON.h / JSON.cpp
│       ├── Random.h / Random.cpp
│       └── Logger.h / Logger.cpp
├── assets/
│   ├── cards.json
│   ├── enemies.json
│   ├── locations.json
│   └── config.json
├── tests/
└── build/ (git-ignored)

## Code Standards

**Modern C++17**: std::optional, std::variant, smart pointers (no raw new/delete)
**Naming**: PascalCase classes, camelCase functions/variables, UPPER_CASE constants
**Const-correctness**: Mark methods const when appropriate
**Memory**: RAII, std::unique_ptr, std::shared_ptr
**Architecture**: Game logic separate from rendering; data-driven (JSON for content)
**Style**: Include guards, .h/.cpp pairs, brief comments on public APIs

## Build

### Prerequisites
- CMake 3.20+
- C++17 compiler (g++, clang, MSVC)
- git

### Commands
```bash
git clone <repo-url>
cd medical-deckbuilder
mkdir build && cd build
cmake ..
cmake --build .
./medical_deckbuilder  # or .\medical_deckbuilder.exe on Windows
```

## Development Workflow

1. You describe feature to build
2. Claude Code implements with clean C++ code
3. You test locally
4. Commit to git

## Phase 1: Core Engine (No Graphics)

- [ ] Card class (name, cost, power, type, tags, effects)
- [ ] Deck class (shuffle, draw, discard pile)
- [ ] Player class (health, mana, active effects)
- [ ] Enemy class (health, attacks)
- [ ] Combat loop (turn-based, damage resolution)
- [ ] Effect system (buffs, debuffs)
- [ ] Console logging for debugging

## Phase 2: raylib Rendering

- [ ] Window and game loop
- [ ] Card rendering
- [ ] Hand display
- [ ] Enemy and player status
- [ ] Button rendering
- [ ] Scene management

## Phase 3: Shop System

- [ ] Shop inventory generation
- [ ] Shop UI and interaction
- [ ] Gold/currency
- [ ] Card purchase

## Phase 4: World & Locations

- [ ] Location definitions
- [ ] Location-specific encounters
- [ ] Map progression
- [ ] Victory/defeat conditions

## Phase 5: Synergies & Polish

- [ ] Card synergy system
- [ ] Advanced effects
- [ ] Enemy AI
- [ ] Sound/visual polish
- [ ] Save/load

## Notes for Claude Code

1. Always include both .h and .cpp (unless template-only)
2. Use const references for input parameters
3. Document public methods briefly
4. No raw pointers; use smart pointers
5. Initialize all members in constructors
6. Use std::optional<T> for optional values
7. Keep functions small and focused
8. Use assertions for logic errors
9. CMakeLists.txt must be self-contained
10. Add new .cpp files to CMakeLists.txt

## JSON Examples

### cards.json
```json
{
  "cards": [
    {
      "id": "antibiotic",
      "name": "Antibiotic",
      "cost": 1,
      "power": 5,
      "type": "attack",
      "tags": ["medicine"],
      "effect": {"type": "deal_damage", "amount": 5}
    }
  ]
}
```

### enemies.json
```json
{
  "enemies": [
    {
      "id": "bacteria_weak",
      "name": "Weak Bacterium",
      "health": 10,
      "attack": 2,
      "tags": ["bacteria"]
    }
  ]
}
```

### locations.json
```json
{
  "locations": [
    {
      "id": "heart",
      "name": "Heart",
      "difficulty": 1,
      "enemies": ["bacteria_weak"]
    }
  ]
}
```

## Design Questions (Answer Together)

- Player resource? (mana, action points, cells?)
- Deck size cap?
- Card upgrades or new cards?
- Synergy system? (tag bonuses, combos, multipliers?)
- Difficulty curve?
- Win condition?
- Relics/permanent modifiers?

## Success Criteria

✅ Builds on any machine with CMake + C++17 compiler
✅ Clean code, no compiler warnings
✅ Modular; new content via JSON only
✅ Playable core loop
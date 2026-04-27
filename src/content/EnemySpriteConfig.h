#pragma once

#include <string>

// One animation clip within a sprite sheet.
struct AnimClip {
    int   startFrame     = 0;
    int   frameCount     = 1;
    float frameDurationMs = 400.0f;
    bool  looping        = true;
};

// All sprite-sheet data for one enemy, parsed from the "sprite" section of
// an enemy JSON file.  Pure data — no raylib types so core/ can safely include it.
struct EnemySpriteConfig {
    std::string sheetPath;          // path to the horizontal sprite sheet
    std::string backgroundPath;     // optional combat background used for this enemy
    int         frameWidth  = 80;
    int         frameHeight = 80;
    AnimClip    idle;                        // frames played while waiting
    AnimClip    attack;                      // one-shot played when attacking
    AnimClip    death = { 0, 0, 200.0f, false }; // one-shot played when defeated; 0 frames = skip straight to dissolve

    bool hasSprite() const { return !sheetPath.empty(); }
};

#pragma once

#include "raylib.h"

// Central color palette for the Medical Deckbuilder UI.
// All colors use the raylib Color type { r, g, b, a }.
namespace Colors {

    // --- Backgrounds ---
    // Primary dark background for the game screen
    constexpr Color dark_bg  = { 18,  18,  24, 255 };
    // Slightly lighter surface for panels and cards
    constexpr Color light_bg = { 32,  32,  44, 255 };

    // --- Card colors ---
    // Default card face background
    constexpr Color card_bg     = { 40,  44,  60, 255 };
    // Card border / outline
    constexpr Color card_border = { 90,  96, 130, 255 };

    // --- UI / Buttons ---
    // Normal button background
    constexpr Color button_bg    = { 55,  60,  80, 255 };
    // Button background when hovered
    constexpr Color button_hover = { 80,  90, 120, 255 };

    // --- Text ---
    // Primary readable text (near-white)
    constexpr Color text_primary   = { 230, 230, 240, 255 };
    // Secondary / dimmed text for descriptions, costs, etc.
    constexpr Color text_secondary = { 150, 155, 170, 255 };

    // --- Status indicators ---
    // Health / HP bar
    constexpr Color health_color = { 200,  60,  60, 255 };
    // Damage / attack indicator
    constexpr Color damage_color = { 220, 100,  30, 255 };
    // Healing / restore indicator
    constexpr Color heal_color   = {  60, 180,  80, 255 };

} // namespace Colors

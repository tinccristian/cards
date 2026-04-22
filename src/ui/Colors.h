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
    constexpr Color block_color  = {  80, 130, 220, 255 };
    constexpr Color mixed_intent_color = { 200, 180,  40, 255 };
    constexpr Color gold_color   = { 220, 185,  65, 255 };

    // --- Overlay / utility ---
    constexpr Color overlay_bg          = {   0,   0,   0, 200 };
    constexpr Color empty_pile_bg       = {  28,  28,  36, 255 };
    constexpr Color placeholder_art_bg  = {  28,  30,  42, 255 };
    constexpr Color draw_pile_accent    = {  80, 120, 200, 255 };
    constexpr Color discard_pile_accent = { 200, 120,  50, 255 };
    constexpr Color close_button_bg     = { 120,  40,  40, 200 };
    constexpr Color close_button_hover  = { 200,  60,  60, 255 };

} // namespace Colors

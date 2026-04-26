#pragma once

// =============================================================================
// ## Assets ##
// =============================================================================
namespace AssetPaths {
inline constexpr const char* PLAYER_CARD_LIBRARY = "assets/decks/player/cards.json";  // full card database
inline constexpr const char* NOAH_CARD_LIBRARY   = "assets/decks/noah/noah_cards.json";
inline constexpr const char* PLAYER_STARTER_DECK = "assets/decks/player/deck_config.json"; // which cards to start with
inline constexpr const char* DEFAULT_ENEMY       = "assets/enemies/fungus.json";
inline constexpr const char* ENEMY_DIRECTORY     = "assets/enemies";                   // scanned at runtime for enemy JSONs
inline constexpr const char* MAP_NODE_TYPES      = "assets/maps/nodes.json";
inline constexpr const char* LEG_MAP_DATA        = "assets/maps/leg/leg.json";
inline constexpr const char* NOAH_EVENT_BG       = "assets/events/noah/noah.png";
inline constexpr const char* PLAYER_SPRITE       = "assets/player/player.json";        // idle anim config + sheet path
inline constexpr const char* DISSOLVE_SHADER     = "assets/shaders/dissolve.fs";       // wind-dissolve death effect
inline constexpr const char* HIT_SHADER          = "assets/shaders/hit.fs";            // color-cycle flash on damage
inline constexpr const char* TRANSITION_SHADER   = "assets/shaders/transition.fs";     // hex-tile screen wipe
inline constexpr const char* INTENT_FLOAT_SHADER = "assets/shaders/intent_float.vs";   // subtle bobbing motion for intent icons
inline constexpr const char* AMBULANCE_SHADER    = "assets/shaders/ambulance.fs";      // moving red/blue light sweep overlay
inline constexpr const char* BLOCK_ICON          = "assets/common/block.png";          // 16×16 shield sprite
inline constexpr const char* ATTACK_ICON         = "assets/common/attack.png";         // 16×16 sword sprite
inline constexpr const char* BUFF_ICON           = "assets/common/buff.png";
inline constexpr const char* DEBUFF_ICON         = "assets/common/debuff.png";
inline constexpr const char* POISON_ICON         = "assets/common/poison.png";
inline constexpr const char* CARD_BORDER         = "assets/common/card_border.png";    // 120×180 border overlay (alpha)
inline constexpr const char* CARD_FONT           = "assets/fonts/m5x7.ttf";            // m5x7 by Daniel Linssen, CC0
inline constexpr const char* MAP_BATTLE_MARKER   = "assets/common/battle.png";
inline constexpr const char* MAP_BATTLE_DONE_MARKER = "assets/common/battle_finished.png";
inline constexpr const char* MAP_EVENT_MARKER    = "assets/common/event.png";
inline constexpr const char* MAP_EVENT_DONE_MARKER = "assets/common/event_finished.png";
} // namespace AssetPaths

// =============================================================================
// ## Audio ##
// =============================================================================
namespace AudioPaths {
inline constexpr const char* CARD_HOVER_SOUND   = "assets/sounds/cards/card_hover.mp3";
inline constexpr const char* CARD_SHUFFLE_SOUND = "assets/sounds/cards/card_shuffle.mp3";
inline constexpr const char* CARD_PICKED_SOUND  = "assets/sounds/cards/card_picked.mp3";
inline constexpr const char* BONUS_SOUND        = "assets/sounds/common/bonus.mp3";
inline constexpr const char* ARMOR_SOUND        = "assets/sounds/common/armor.mp3";
inline constexpr const char* ARMOR_HIT_SOUND    = "assets/sounds/common/armor_hit.mp3";
inline constexpr const char* DAMAGE_SOUND       = "assets/sounds/common/damage.mp3";
inline constexpr const char* COIN_PICKED_SOUND  = "assets/sounds/common/coin_picked.mp3";
inline constexpr const char* ENEMY_BUFF_SOUND   = "assets/sounds/enemy/enemy_buff.mp3";
inline constexpr const char* ENEMY_HURT_SOUND   = "assets/sounds/enemy/enemy _hurt.mp3";
inline constexpr const char* ENEMY_DEATH_SOUND  = "assets/sounds/enemy/enemy_death.mp3";
inline constexpr const char* PLAYER_DEATH_SOUND = "assets/sounds/player/player_death.mp3";
inline constexpr const char* GAME_OVER_SOUND    = "assets/sounds/common/game_over.mp3";
inline constexpr const char* NEXT_TURN_SOUND    = "assets/sounds/common/next_turn.mp3";
inline constexpr const char* NOAH_EVENT_SOUND   = "assets/sounds/event/noah_event.mp3";
inline constexpr const char* AMBULANCE_SOUND    = "assets/sounds/cards/ambulance.mp3";
} // namespace AudioPaths

namespace AudioConfig {
inline constexpr float MasterVolume        = 50.0f; // default master volume (0–100)
inline constexpr float CardHoverVolume     = 15.0f; // per-voice volume for hover SFX
inline constexpr float CardShuffleVolume   = 25.0f;
inline constexpr float CardPickedVolume    = 28.0f;
inline constexpr float RewardEnterVolume   = 30.0f;
inline constexpr float ArmorVolume         = 24.0f;
inline constexpr float ArmorHitVolume      = 24.0f;
inline constexpr float DamageVolume        = 32.0f;
inline constexpr float CoinPickedVolume    = 32.0f;
inline constexpr float EnemyBuffVolume     = 28.0f;
inline constexpr float EnemyHurtVolume     = 32.0f;
inline constexpr float EnemyDeathVolume    = 35.0f;
inline constexpr float PlayerDeathVolume   = 36.0f;
inline constexpr float GameOverVolume      = 34.0f;
inline constexpr float NextTurnVolume      = 30.0f;
inline constexpr float NoahEventVolume     = 34.0f;
inline constexpr float AmbulanceVolume     = 35.0f;
inline constexpr int   MasterVolumeStep    = 5;     // how much +/- each options arrow click changes master volume
inline constexpr int   CardHoverVoiceCount = 6;     // number of concurrent hover voices (prevents clipping)
} // namespace AudioConfig

// =============================================================================
// ## Settings ##
// =============================================================================
namespace SettingsConfig {
inline constexpr const char* AppFolderName    = "MedicalDeckbuilder"; // OS user-data folder name
inline constexpr const char* SettingsFileName = "settings.json";
} // namespace SettingsConfig

// =============================================================================
// ## Window ##
// =============================================================================
namespace WindowConfig {
inline constexpr int Width     = 1280;
inline constexpr int Height    = 720;
inline constexpr int TargetFps = 60;
} // namespace WindowConfig

// =============================================================================
// ## Combat ##
// =============================================================================
namespace CombatConfig {
inline constexpr int   PlayerMaxHealth       = 100;
inline constexpr int   PlayerBaseMana        = 3;   // mana restored at the start of each player turn
inline constexpr int   OpeningHandSize       = 5;   // cards drawn at combat start
inline constexpr int   StartingTurnNumber    = 1;
inline constexpr int   EnemyCardsPerTurn     = 2;   // how many intent cards the enemy queues per turn
inline constexpr int   DefaultEffectDuration = 1;   // turns a status effect lasts if not specified
inline constexpr float EnemyTurnDelaySecs    = 1.0f; // pause before the enemy executes its turn
} // namespace CombatConfig

// =============================================================================
// ## Luck ##
// =============================================================================
namespace LuckConfig {
inline constexpr float CommonCardDropRate   = 0.65f;
inline constexpr float UncommonCardDropRate = 0.25f;
inline constexpr float RareCardDropRate     = 0.10f;
inline constexpr int   RewardCardChoiceCount = 3;
} // namespace LuckConfig

// =============================================================================
// ## Map Data ##
// =============================================================================
namespace MapConfig {
inline constexpr float SourceWidth  = 2560.0f; // native resolution of the map texture
inline constexpr float SourceHeight = 1536.0f;
} // namespace MapConfig

// =============================================================================
// ## UI  (all sizes are in logical pixels at 1280×720; scaled at runtime)
// =============================================================================
namespace LayoutConfig {

// -----------------------------------------------------------------------------
// Shared
// -----------------------------------------------------------------------------
inline constexpr int   DefaultButtonFontSize = 22;
inline constexpr int   PanelBorderThickness  = 2;  // thick border used on overlay panels
inline constexpr int   ThinBorderThickness   = 1;  // fine border used on art placeholders etc.
inline constexpr float UiMinScale            = 1.0f; // clamp for uiScale() — never shrinks below native
inline constexpr float UiMaxScale            = 1.8f; // clamp for uiScale() — prevents UI overflowing at very high res

// -----------------------------------------------------------------------------
// Menu
// -----------------------------------------------------------------------------
inline constexpr int MenuTitleFontSize = 48;
inline constexpr int MenuButtonWidth   = 200;
inline constexpr int MenuButtonHeight  = 60;
inline constexpr int MenuButtonGap     = 20; // vertical space between buttons

// -----------------------------------------------------------------------------
// Combat Header
// -----------------------------------------------------------------------------
inline constexpr int CombatTurnFontSize = 26; // "Turn N" label at the top
inline constexpr int CombatLogTop       = 58; // Y offset of the combat log below the header
inline constexpr int CombatLogFontSize  = 17;
inline constexpr int TopHudBarHeight      = 44;
inline constexpr int TopHudSidePadding    = 16;
inline constexpr int TopHudTextPaddingY   = 12;
inline constexpr int TopHudGoldFontSize   = 18;
inline constexpr int TopHudCenterFontSize = 20;
inline constexpr int TopHudButtonWidth    = 92;
inline constexpr int TopHudButtonHeight   = 28;
inline constexpr int TopHudButtonGap      = 10;
inline constexpr float TopHudHoverScale   = 1.08f;

// -----------------------------------------------------------------------------
// Cards
// -----------------------------------------------------------------------------

// Logical card size used by in-game card rendering.
inline constexpr int   CardWidth     = 150; // px
inline constexpr int   CardHeight    = 225; // px
inline constexpr float CardGap       = -40.0f; // negative = cards overlap in hand
inline constexpr int   CardArtHeight = 100; // px, art window at the top of the card
inline constexpr int   CardBorderSourceWidth  = 120; // native size of card_border.png
inline constexpr int   CardBorderSourceHeight = 180;

// Hand layout
inline constexpr float HandLeftBoundPercent      = 0.14f; // hand starts this far from the left edge
inline constexpr float HandRightBoundPercent     = 0.86f; // hand ends this far from the left edge
inline constexpr int   HandBottomMargin          = -20;   // how far the hand extends below the screen bottom
inline constexpr int   HandDropZoneTopPadding    = 90;    // extra height added above the hand for the drag drop-zone
inline constexpr float HandBottomOverflowPercent = 0.30f; // fraction of card height allowed to hang off-screen

// Card hover & animation
inline constexpr float HoveredCardScale        = 2.0f;   // scale multiplier when a card is hovered
inline constexpr float HoveredCardLift         = 250.0f; // px the hovered card rises above the hand baseline
inline constexpr float HoverAnimSpeed          = 15.0f;  // lerp speed for the hover grow/shrink animation
inline constexpr float HandArchHeight          = 50.0f;  // peak height of the hand arc (center card vs edges)
inline constexpr float HandMaxTiltDegrees      = 14.0f;  // max rotation applied to cards at the hand edges
inline constexpr float HoveredTiltFactor       = 0.5f;   // reduces tilt on the hovered card (0 = flat, 1 = full)
inline constexpr float CardIdleWiggleDegrees   = 1.0f;   // smaller idle rotation keeps the hand alive without obvious shimmer
inline constexpr float CardIdleWiggleFrequency = 1.8f;   // Hz of the idle wiggle sine wave
inline constexpr float CardIdleWigglePhaseStep = 0.65f;  // phase offset between adjacent cards so they don't sync
inline constexpr float NeighborCardShift       = 15.0f;  // px neighbors spread apart when a card is hovered

// Card text  (sizes in logical px; drawn with a 1-px black outline)
inline constexpr int CardNameFontSize      = 21; // m5x7 is authored for 16/32/48 px sizes
inline constexpr int HoveredCardNameSize   = 21; // enlarged by card scale, not raw font size
inline constexpr int CardStaticFontBonus   = 5;  // pile/reward cards are static and need larger text
inline constexpr int CardHoveredFontBonus  = 3;  // hovered cards get a smaller bump than static grid cards
inline constexpr float CardNameLetterSpacing = 0.0f;
inline constexpr int CardDescriptionSize   = 18;
inline constexpr float CardDescriptionLetterSpacing = 0.0f;
inline constexpr int CardDescriptionGap    = 4;  // px between description lines
inline constexpr float CardAnimatedTextScale = 0.72f; // hand cards are rendered oversized before GPU transforms
inline constexpr float CardStaticTextScale   = 0.62f; // static preview/reward/deck cards should not scale text as aggressively as art
inline constexpr float CardAnimatedMinNameTracking = 0.35f; // explicit glyph tracking before hand card downsampling
inline constexpr float CardAnimatedMinDescriptionTracking = 0.25f; // explicit glyph tracking before hand card downsampling
inline constexpr float CardStaticLetterSpacingScale = 2.0f; // static cards need more tracking after reduced text scaling
inline constexpr float CardStaticMinNameTracking = 0.45f; // explicit glyph tracking for large static previews
inline constexpr float CardStaticMinDescriptionTracking = 0.35f; // explicit glyph tracking for large static previews
inline constexpr int CardDescriptionLines  = 3;  // max lines shown on the card face
inline constexpr int CardFooterSize        = 13; // dmg/block stat in the bottom-right corner
inline constexpr int HoveredCardFooterSize = 15;
inline constexpr int CardFooterMargin      = 24; // px from the card bottom to the footer baseline
inline constexpr int CardRightStatPadding  = 16; // px inset from the card right edge for the stat text
inline constexpr int CardTextPadding       = 12; // horizontal inset for description text
inline constexpr int CardNameSidePadding   = 18; // horizontal safe inset for the title row
inline constexpr int CardTextTopPadding    = 7;  // px between the art/divider line and the card name
inline constexpr int CardTextGap           = 7;  // px between name and description
inline constexpr int CardDividerPadding    = 2;
inline constexpr int CardNameNudgeUp       = 3;  // px the name is pushed above the art/text divider line
inline constexpr float CardFaceRenderScale = 4.0f; // compose full card faces oversized, then let the GPU scale them down smoothly

// Mana badge — position in logical card pixels.
// These boxes are authored against the native 120x180 card border art.
inline constexpr int CardManaBoxLeft   = 3;
inline constexpr int CardManaBoxTop    = 3;
inline constexpr int CardManaBoxRight  = 15;
inline constexpr int CardManaBoxBottom = 14;
inline constexpr int CardArtBoxLeft    = 2;
inline constexpr int CardArtBoxTop     = 2;
inline constexpr int CardArtBoxRight   = 117;
inline constexpr int CardArtBoxBottom  = 81;
inline constexpr int CardNameBoxLeft   = 10;
inline constexpr int CardNameBoxTop    = 81;
inline constexpr int CardNameBoxRight  = 109;
inline constexpr int CardNameBoxBottom = 96;
inline constexpr int CardDescriptionBoxLeft   = 7;
inline constexpr int CardDescriptionBoxTop    = 100;
inline constexpr int CardDescriptionBoxRight  = 112;
inline constexpr int CardDescriptionBoxBottom = 174;
inline constexpr int CardDescriptionInnerInset = 6; // extra safety inset so long lines wrap before the frame edge
inline constexpr int CardDescriptionTopInset = 3; // keeps body text clear of the name plate frame

// -----------------------------------------------------------------------------
// Piles  (draw pile / discard pile widgets in the bottom corners)
// -----------------------------------------------------------------------------
inline constexpr int   PileWidgetWidth         = 86;
inline constexpr int   PileWidgetHeight        = 118;
inline constexpr float PileSideMarginPercent   = 0.02f; // fraction of screen width kept as a margin
inline constexpr float PileBottomMarginPercent = 0.03f; // fraction of screen height kept as a margin
inline constexpr int   PileLabelFontSize       = 12;    // "Draw" / "Discard" label
inline constexpr int   PileLabelOffsetY        = 8;     // px from widget top to the label
inline constexpr int   PileCountFontSize       = 26;    // large card-count number
inline constexpr int   PileEmptyFontSize       = 16;    // shown when pile is empty
inline constexpr int   PileHintFontSize        = 11;    // "click to view" hint text
inline constexpr int   PileHintOffsetY         = 4;     // px from widget bottom to the hint
inline constexpr int   PileShadowLayers        = 3;     // stacked rectangle shadow passes
inline constexpr float PileShadowOffset        = 2.0f;  // px offset per shadow layer
inline constexpr int   PileShadowAlpha         = 180;   // alpha of the darkest shadow layer

// -----------------------------------------------------------------------------
// Turn Controls
// -----------------------------------------------------------------------------
inline constexpr int EndTurnButtonWidth  = 160;
inline constexpr int EndTurnButtonHeight = 50;
inline constexpr int EndTurnButtonMargin = 24; // px from the screen right edge
inline constexpr int EndTurnToPileGap   = 14; // px between the End Turn button and the discard pile widget

// -----------------------------------------------------------------------------
// Entity HUD  (player and enemy name / health bar below their sprite)
// -----------------------------------------------------------------------------

// Sprite placement
inline constexpr float PlayerEntityCenterXPercent = 0.18f; // fraction of screen width for the player sprite center
inline constexpr float EnemyEntityCenterXPercent  = 0.82f; // fraction of screen width for the enemy sprite center
inline constexpr int   EntitySpriteTop            = 150;   // px from the screen top to the sprite top
inline constexpr int   EntitySpriteSize           = 240;   // sprite drawn into a square of this size (px)

// HUD layout
inline constexpr int   EntityHudWidth          = 250; // total HUD width (name + health bar)
inline constexpr int   EntityHudGap            = 12;  // px between sprite bottom and HUD top
inline constexpr int   EntityHudNameGap        = 20;  // px between the name text and the health bar
inline constexpr int   EntityHealthTextGap     = 10;  // (reserved) gap used for additional text rows
inline constexpr int   EntityBlockGap          = 8;   // (reserved) extra space around the block slot
inline constexpr int   EntityMinHealthBarWidth = 24;  // minimum bar width so the bar never disappears

// Text & stats
inline constexpr int EntityNameFontSize = 20;
inline constexpr int EntityTextPadding  = 10;
inline constexpr int HealthBarHeight    = 18; // px; block slot is drawn at 1.5× this height
inline constexpr int EntityStatFontSize = 16; // hp text inside the bar, and block / intent numbers
inline constexpr int EntityStatusGap    = 8;
inline constexpr int EntityStatusIconSize = 18;
inline constexpr int EntityStatusIconGap  = 4;
inline constexpr int EntityStatusValueSize = 12;

// -----------------------------------------------------------------------------
// Mana HUD  (small box above the draw pile showing current / max mana)
// -----------------------------------------------------------------------------
inline constexpr int ManaHudWidth        = 96;
inline constexpr int ManaHudHeight       = 44;
inline constexpr int ManaHudGapToPile    = 10; // px between the HUD bottom and the draw pile widget top
inline constexpr int ManaHudLabelSize    = 11; // "MANA" label
inline constexpr int ManaHudValueSize    = 18; // large "X / Y" value
inline constexpr int ManaHudValueOffsetY = 16; // px from HUD top to the value baseline
inline constexpr int ManaHudLabelTop     = 4;  // px from HUD top to the label

// -----------------------------------------------------------------------------
// Intent  (enemy attack / block icons shown below the enemy HUD)
// -----------------------------------------------------------------------------
inline constexpr int   IntentHeight          = 34;  // (legacy, kept for reference)
inline constexpr int   IntentOffsetY         = 6;   // (legacy, kept for reference)
inline constexpr int   IntentFontSize        = 15;  // (legacy, kept for reference)
inline constexpr int   IntentTooltipWidth    = 200; // (legacy, kept for reference)
inline constexpr int   IntentTooltipHeight   = 42;  // (legacy, kept for reference)
inline constexpr int   IntentTooltipOffsetX  = 4;   // (legacy, kept for reference)
inline constexpr int   IntentTooltipFontSize = 13;  // (legacy, kept for reference)
inline constexpr int   IntentTooltipPadding  = 6;   // (legacy, kept for reference)
inline constexpr float IntentIconGap         = 4.0f;
inline constexpr float IntentFloatAmplitude  = 3.0f;  // vertical bob in logical pixels
inline constexpr float IntentFloatSpeed      = 1.8f;  // slightly quicker bob so the intent reads as alive
inline constexpr float IntentFloatPhaseStep  = 0.9f;  // phase separation between sibling icons
inline constexpr float IntentAttackValueOffsetX = 8.0f; // pushes the attack number farther outside the icon on the left
inline constexpr float IntentAttackValueOffsetY = 12.0f; // keeps attack multipliers clear of adjacent intent icons

// -----------------------------------------------------------------------------
// Overlays  (Pause / Options — both reuse the same centred panel)
// -----------------------------------------------------------------------------
inline constexpr int OverlayTitleFontSize  = 34;
inline constexpr int OverlayPanelWidth     = 760;
inline constexpr int PausePanelHeight      = 380;
inline constexpr int OptionsPanelHeight    = 540;
inline constexpr int OverlayButtonWidth    = 220;
inline constexpr int OverlayButtonHeight   = 48;
inline constexpr int OverlayButtonGap      = 14;  // vertical gap between overlay buttons
inline constexpr int OverlayContentInset   = 36;  // horizontal inset of content inside the panel
inline constexpr int PauseButtonsTopOffset = 92;  // px from panel top to the first pause button

// -----------------------------------------------------------------------------
// Rewards
// -----------------------------------------------------------------------------
inline constexpr int RewardPanelWidth         = 620;
inline constexpr int RewardPanelHeight        = 300;
inline constexpr int RewardListTopOffset      = 86;
inline constexpr int RewardListHeight         = 132;
inline constexpr int RewardEntryHeight        = 56;
inline constexpr int RewardEntryGap           = 8;
inline constexpr int RewardEntryTitleSize     = 22;
inline constexpr int RewardEntryBodySize      = 16;
inline constexpr int RewardEntryInset         = 18;
inline constexpr int RewardContinueWidth      = 180;
inline constexpr int RewardContinueHeight     = 44;
inline constexpr int RewardContinueBottomGap  = 28;
inline constexpr int RewardCardTitleSize      = 34;
inline constexpr int RewardCardChoiceGap      = 36;
inline constexpr int RewardCardChoiceWidth    = 220;
inline constexpr int RewardCardChoiceTop      = 170;
inline constexpr int RewardCardChoiceHeight   = 330;
inline constexpr int RewardHintSize           = 16;
inline constexpr int RewardVignetteAlpha      = 170;
inline constexpr float RewardCardHoverScale   = 1.12f;
inline constexpr int RewardSkipButtonWidth    = 140;
inline constexpr int RewardSkipButtonHeight   = 40;
inline constexpr int RewardSkipButtonTopGap   = 24;

// -----------------------------------------------------------------------------
// Options screen
// -----------------------------------------------------------------------------
inline constexpr int   OptionsTabWidth              = 140; // width of each tab (Audio / Video / etc.)
inline constexpr int   OptionsTabHeight             = 40;
inline constexpr int   OptionsTabGap                = 10;  // gap between tabs
inline constexpr int   OptionsTabTopOffset          = 70;  // px from panel top to the tab row
inline constexpr int   OptionsRowsTopOffset         = 140; // px from panel top to the first settings row
inline constexpr int   OptionsRowHeight             = 58;  // height of each setting row
inline constexpr int   OptionsLabelFontSize         = 22;
inline constexpr int   OptionsValueFontSize         = 20;
inline constexpr int   OptionsArrowButtonSize       = 34;  // size of the < > stepper buttons
inline constexpr int   OptionsCheckboxSize          = 24;
inline constexpr int   OptionsValueAreaWidth        = 280; // right-side area reserved for the value control
inline constexpr int   OptionsValuePadding          = 12;  // inset inside the value area
inline constexpr int   OptionsRowTextOffsetY        = 6;   // vertical nudge to vertically centre label text
inline constexpr int   OptionsValueTextOffsetY      = 2;
inline constexpr int   OptionsCheckboxTickInset     = 4;   // inset of the tick mark inside the checkbox rect
inline constexpr float OptionsCheckboxTickThickness = 3.0f;
inline constexpr int   OptionsFooterOffsetY         = 72;  // px from panel bottom to the Back button

// -----------------------------------------------------------------------------
// Game Over screen
// -----------------------------------------------------------------------------
inline constexpr int GameOverFontSize      = 60;
inline constexpr int GameOverButtonWidth   = 240;
inline constexpr int GameOverButtonHeight  = 60;
inline constexpr int GameOverButtonOffsetY = 20; // px below the "Game Over" title text
inline constexpr int GameOverSubtitleSize  = 22;
inline constexpr int GameOverButtonGap     = 18;
inline constexpr int GameOverPanelWidth    = 560;
inline constexpr int GameOverPanelHeight   = 300;
inline constexpr int GameOverVignetteAlpha = 190;

// -----------------------------------------------------------------------------
// Death presentation
// -----------------------------------------------------------------------------
inline constexpr float DeathSlowMoScale    = 0.24f;
inline constexpr float DeathSlowMoDuration = 0.70f;

// -----------------------------------------------------------------------------
// Turn feedback
// -----------------------------------------------------------------------------
inline constexpr float TurnVignetteAlpha     = 110.0f;
inline constexpr float TurnVignetteDuration  = 0.55f;

// -----------------------------------------------------------------------------
// Death presentation
// -----------------------------------------------------------------------------
inline constexpr float DeathPresentationDuration = 2.0f;
inline constexpr float DeathScreenRedAlpha       = 165.0f;

// -----------------------------------------------------------------------------
// Floating damage numbers
// -----------------------------------------------------------------------------
inline constexpr float DamageNumberLifetime      = 0.80f;
inline constexpr float DamageNumberRiseDistance  = 44.0f;
inline constexpr float DamageNumberFallDistance  = 12.0f;
inline constexpr float DamageNumberSpawnJitterX  = 18.0f;
inline constexpr float DamageNumberStackGap      = 18.0f;
inline constexpr float DamageNumberCenterOffsetX = 72.0f;
inline constexpr float DamageNumberScaleStart    = 0.55f;
inline constexpr float DamageNumberScalePeak     = 1.25f;
inline constexpr float DamageNumberScaleEnd      = 1.0f;
inline constexpr int   DamageNumberFontSize      = 30;
inline constexpr float HitEventDelayStep         = 0.08f;

// -----------------------------------------------------------------------------
// Noah Event
// -----------------------------------------------------------------------------
inline constexpr int   NoahPanelWidth             = 340;
inline constexpr int   NoahPanelHeight            = 330;
inline constexpr int   NoahPanelBottomMargin      = 28;
inline constexpr int   NoahPanelRightMargin       = 28;
inline constexpr int   NoahPanelTitleSize         = 24;
inline constexpr int   NoahPanelTitleTop          = 16;
inline constexpr int   NoahOptionWidth            = 280;
inline constexpr int   NoahOptionHeight           = 74;
inline constexpr int   NoahOptionGap              = 12;
inline constexpr int   NoahOptionTitleSize        = 18;
inline constexpr int   NoahOptionBodySize         = 14;
inline constexpr int   NoahOptionTopOffset        = 58;
inline constexpr int   NoahEventTitleSize         = 36;
inline constexpr int   NoahEventSubtitleSize      = 18;
inline constexpr int   NoahEventLeftMargin        = 58;
inline constexpr int   NoahEventTopMargin         = 68;
inline constexpr int   NoahChoiceCardWidth        = 220;
inline constexpr int   NoahChoiceCardHeight       = 330;
inline constexpr int   NoahChoiceCardGap          = 34;
inline constexpr int   NoahChoiceTop              = 180;
inline constexpr float NoahChoiceHoverScale       = 1.10f;
inline constexpr int   NoahConfirmButtonWidth     = 180;
inline constexpr int   NoahConfirmButtonHeight    = 42;
inline constexpr int   NoahConfirmButtonBottomGap = 36;
inline constexpr int   NoahDeckPanelWidth         = 900;
inline constexpr int   NoahDeckPanelHeight        = 590;
inline constexpr int   NoahDeckPanelTop           = 108;
inline constexpr int   NoahDeckColumns            = 5;
inline constexpr float NoahDeckVisibleRows        = 2.5f;
inline constexpr int   NoahDeckGridGap            = 16;
inline constexpr int   NoahDeckTopPadding         = 54;
inline constexpr int   NoahDeckBottomPadding      = 56;
inline constexpr float NoahDeckHoverScale         = 1.30f;
inline constexpr int   NoahDeckSelectionThickness = 4;
inline constexpr int   NoahRevealTitleSize        = 30;
inline constexpr int   NoahRevealBodySize         = 18;
inline constexpr int   NoahRevealTop              = 160;

// -----------------------------------------------------------------------------
// Map UI
// -----------------------------------------------------------------------------
inline constexpr float MapZoomFactor           = 1.35f;  // the map texture is scaled up by this factor
inline constexpr float MapInitialAnchor        = 1.0f;   // 0 = top of map visible, 1 = bottom of map visible
inline constexpr float MapScrollWheelStep      = 80.0f;  // px scrolled per mouse-wheel tick
inline constexpr float MapDragThreshold        = 4.0f;   // px of movement before drag is registered
inline constexpr int   MapNodeRadius           = 18;     // filled circle radius for map nodes
inline constexpr int   MapNodeOutlineRadius    = 24;     // outline circle radius (drawn behind the fill)
inline constexpr int   MapNodeOutlineThickness = 3;
inline constexpr float MapConnectionThickness  = 5.0f;   // line width of node connections
inline constexpr int   MapConnectionPixelStep  = 8;
inline constexpr int   MapConnectionPixelSize  = 5;
inline constexpr int   MapMarkerSize           = 44;
inline constexpr float MapMarkerHoverScale     = 1.18f;
inline constexpr float MapMarkerHoverBounce    = 4.0f;
inline constexpr int   MapNodeLabelFontSize    = 14;
inline constexpr int   MapTitleFontSize        = 26;     // "Choose your path" heading
inline constexpr int   MapHintFontSize         = 16;     // bottom-corner hint text
inline constexpr int   MapTitleTopMargin       = 24;     // px from screen top to the map title
inline constexpr int   MapHintBottomMargin     = 22;     // px from screen bottom to the hint text
inline constexpr int   MapHintSideMargin       = 24;     // px from screen right to the hint text

// -----------------------------------------------------------------------------
// Screen Transition  (hex-tile wipe between scenes)
// -----------------------------------------------------------------------------
inline constexpr float ScreenTransitionDuration     = 1.6f;  // total wipe duration in seconds
inline constexpr float ScreenTransitionHexSize      = 0.08f; // hex tile size as fraction of screen height
inline constexpr float ScreenTransitionRandomness   = 0.08f; // per-tile timing jitter (0 = uniform sweep)
inline constexpr float ScreenTransitionEdgeSoftness = 0.01f; // feathering at the wipe edge
inline constexpr float ScreenTransitionSpeed        = 0.3f;  // shader progress speed multiplier

// -----------------------------------------------------------------------------
// Pile Viewer  (full-screen overlay showing draw or discard pile contents)
// -----------------------------------------------------------------------------
inline constexpr int PileViewerPanelWidth        = 760;
inline constexpr int PileViewerPanelHeight       = 660;
inline constexpr int PileViewerTitleSize         = 28;  // "Draw Pile" / "Discard Pile"
inline constexpr int PileViewerTitleOffsetY      = 12;  // px from panel top to title
inline constexpr int PileViewerSubtitleSize      = 16;  // card count subtitle
inline constexpr int PileViewerSubtitleOffsetY   = 46;  // px from panel top to subtitle
inline constexpr int PileViewerHeaderHeight      = 70;  // height of the title+subtitle area
inline constexpr int PileViewerCloseButtonSize   = 36;  // X button square size
inline constexpr int PileViewerCloseButtonMargin = 8;   // inset of the X button from the panel corner
inline constexpr int PileViewerDividerInset      = 10;  // horizontal inset of the header divider line
inline constexpr int PileViewerGridTopOffset     = 80;  // px from panel top to the card grid
inline constexpr int PileViewerGridBottomPadding = 70;  // extra space below the last card row
inline constexpr int PileViewerScissorInset      = 2;   // shrinks the scissor rect to hide clipping artefacts
inline constexpr int PileViewerGridColumns       = 5;
inline constexpr float PileViewerVisibleRows     = 2.5f;
inline constexpr int PileViewerGridGap           = 14;
inline constexpr int PileViewerGridTopPadding    = 44;
inline constexpr int PileViewerGridBottomPaddingInner = 56;
inline constexpr float PileViewerHoverScale      = 1.28f;
inline constexpr int PileViewerCellWidth         = 200; // px per grid cell (card + padding)
inline constexpr int PileViewerCellHeight        = 220;
inline constexpr int PileViewerCellInset         = 4;   // gap between card and cell edge
inline constexpr int PileViewerFooterHintSize    = 13;  // scroll hint text
inline constexpr int PileViewerFooterOffsetY     = 22;  // px from panel bottom to the hint text

// -----------------------------------------------------------------------------
// Tooltips  (pop-up boxes sized to their content)
// -----------------------------------------------------------------------------
inline constexpr int TooltipWidth         = 210; // (legacy max-width, actual size is content-driven)
inline constexpr int TooltipHeight        = 140; // (legacy max-height, actual size is content-driven)
inline constexpr int TooltipPadding       = 8;   // inner padding on all sides
inline constexpr int TooltipTitleSize     = 16;  // bold title line (card or entity name)
inline constexpr int TooltipTextSize      = 14;  // body text
inline constexpr int TooltipMetaSize      = 13;  // fine-print / secondary line
inline constexpr int TooltipLineSpacing   = 4;   // extra px between body lines
inline constexpr int TooltipTitleSpacing  = 22;  // (legacy) spacing constant
inline constexpr int TooltipMetaSpacing   = 20;  // (legacy) spacing constant
inline constexpr int TooltipWrapWidth     = 26;  // max characters per line before wrapping
inline constexpr int TooltipScreenMargin  = 4;   // min px between tooltip edge and screen edge
inline constexpr int TooltipHorizontalGap = 8;   // px between the anchor rect and the tooltip box

// -----------------------------------------------------------------------------
// Debug
// -----------------------------------------------------------------------------
inline constexpr int FpsCounterX = 16; // screen position of the FPS overlay
inline constexpr int FpsCounterY = 16;

} // namespace LayoutConfig

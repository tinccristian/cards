#pragma once

// ## Assets ##
namespace AssetPaths {
inline constexpr const char* PLAYER_CARD_LIBRARY = "assets/decks/player/cards.json";
inline constexpr const char* PLAYER_STARTER_DECK = "assets/decks/player/deck_config.json";
inline constexpr const char* DEFAULT_ENEMY       = "assets/enemies/bacteria.json";
} // namespace AssetPaths

// ## Audio ##
namespace AudioPaths {
inline constexpr const char* CARD_HOVER_SOUND   = "assets/sounds/cards/card_hover.mp3";
inline constexpr const char* CARD_SHUFFLE_SOUND = "assets/sounds/cards/card_shuffle.mp3";
} // namespace AudioPaths

namespace AudioConfig {
// ## Audio / Levels ##
inline constexpr float MasterVolume      = 78.0f;
inline constexpr float CardHoverVolume   = 42.0f;
inline constexpr float CardShuffleVolume = 68.0f;

// ## Audio / Card SFX ##
inline constexpr int CardHoverVoiceCount = 6;
} // namespace AudioConfig

// ## Window ##
namespace WindowConfig {
inline constexpr int Width  = 1280;
inline constexpr int Height = 720;
inline constexpr int TargetFps = 60;
} // namespace WindowConfig

// ## Combat ##
namespace CombatConfig {
inline constexpr int PlayerMaxHealth       = 100;
inline constexpr int PlayerBaseMana        = 3;
inline constexpr int OpeningHandSize       = 5;
inline constexpr int StartingTurnNumber    = 1;
inline constexpr int EnemyCardsPerTurn     = 2;
inline constexpr int DefaultEffectDuration = 1;
inline constexpr float EnemyTurnDelaySecs  = 1.0f;
} // namespace CombatConfig

// ## UI ##
namespace LayoutConfig {
// ## UI / Shared ##
inline constexpr int DefaultButtonFontSize = 22;
inline constexpr int PanelBorderThickness  = 2;
inline constexpr int ThinBorderThickness   = 1;

// ## UI / Menu ##
inline constexpr int MenuTitleFontSize     = 48;
inline constexpr int MenuButtonWidth       = 200;
inline constexpr int MenuButtonHeight      = 60;
inline constexpr int MenuButtonGap         = 20;

// ## UI / Combat Header ##
inline constexpr int CombatTurnFontSize    = 26;
inline constexpr int CombatantBoxWidth     = 260;
inline constexpr int CombatantBoxHeight    = 160;
inline constexpr int CombatantBoxTop       = 55;
inline constexpr int CombatantBoxMargin    = 50;
inline constexpr int CombatLogOffsetY      = 8;
inline constexpr int CombatLogFontSize     = 17;

// ## UI / Cards ##
inline constexpr int CardWidth             = 120;
inline constexpr int CardHeight            = 180;
inline constexpr int CardGap               = 6;
inline constexpr int CardArtHeight         = 80;
inline constexpr float HandLeftBoundPercent  = 0.14f;
inline constexpr float HandRightBoundPercent = 0.86f;
inline constexpr float HoveredCardScale    = 1.4f;
inline constexpr float HoveredCardLift     = 40.0f;
inline constexpr float HandArchHeight      = 24.0f;
inline constexpr float HandMaxTiltDegrees  = 14.0f;
inline constexpr float HoveredTiltFactor   = 0.35f;
inline constexpr int HandBottomMargin      = 50;
inline constexpr int HandDropZoneTopPadding = 90;
inline constexpr float WiggleXAmplitude    = 3.0f;
inline constexpr float WiggleYAmplitude    = 2.0f;
inline constexpr float WiggleXFrequency    = 3.0f;
inline constexpr float WiggleYFrequency    = 2.0f;
inline constexpr float NeighborCardShift   = 20.0f;

// ## UI / Piles ##
inline constexpr int PileWidgetWidth       = 86;
inline constexpr int PileWidgetHeight      = 118;
inline constexpr float PileSideMarginPercent = 0.02f;
inline constexpr float PileBottomMarginPercent = 0.03f;
inline constexpr int PileLabelFontSize     = 12;
inline constexpr int PileLabelOffsetY      = 8;
inline constexpr int PileCountFontSize     = 26;
inline constexpr int PileEmptyFontSize     = 16;
inline constexpr int PileHintFontSize      = 11;
inline constexpr int PileHintOffsetY       = 4;
inline constexpr int PileShadowLayers      = 3;
inline constexpr float PileShadowOffset    = 2.0f;
inline constexpr int PileShadowAlpha       = 180;

// ## UI / Turn Controls ##
inline constexpr int EndTurnButtonWidth    = 160;
inline constexpr int EndTurnButtonHeight   = 50;
inline constexpr int EndTurnButtonMargin   = 24;
inline constexpr int EndTurnToPileGap      = 14;

// ## UI / Game Over ##
inline constexpr int GameOverFontSize      = 60;
inline constexpr int GameOverButtonWidth   = 240;
inline constexpr int GameOverButtonHeight  = 60;
inline constexpr int GameOverButtonOffsetY = 20;

// ## UI / Pile Viewer ##
inline constexpr int PileViewerPanelWidth  = 900;
inline constexpr int PileViewerPanelHeight = 560;
inline constexpr int PileViewerTitleSize   = 28;
inline constexpr int PileViewerTitleOffsetY = 12;
inline constexpr int PileViewerSubtitleSize = 16;
inline constexpr int PileViewerSubtitleOffsetY = 46;
inline constexpr int PileViewerHeaderHeight = 70;
inline constexpr int PileViewerCloseButtonSize = 36;
inline constexpr int PileViewerCloseButtonMargin = 8;
inline constexpr int PileViewerDividerInset = 10;
inline constexpr int PileViewerGridTopOffset = 80;
inline constexpr int PileViewerGridBottomPadding = 90;
inline constexpr int PileViewerScissorInset = 2;
inline constexpr int PileViewerGridColumns = 4;
inline constexpr int PileViewerCellWidth   = 200;
inline constexpr int PileViewerCellHeight  = 220;
inline constexpr int PileViewerCellInset   = 4;
inline constexpr int PileViewerFooterHintSize = 13;
inline constexpr int PileViewerFooterOffsetY = 22;

// ## UI / Tooltips ##
inline constexpr int TooltipWidth          = 210;
inline constexpr int TooltipHeight         = 140;
inline constexpr int TooltipPadding        = 8;
inline constexpr int TooltipTitleSize      = 16;
inline constexpr int TooltipTextSize       = 14;
inline constexpr int TooltipMetaSize       = 13;
inline constexpr int TooltipLineSpacing    = 4;
inline constexpr int TooltipTitleSpacing   = 22;
inline constexpr int TooltipMetaSpacing    = 20;
inline constexpr int TooltipWrapWidth      = 26;
inline constexpr int TooltipScreenMargin   = 4;
inline constexpr int TooltipHorizontalGap  = 8;

// ## UI / Intent ##
inline constexpr int IntentHeight          = 34;
inline constexpr int IntentOffsetY         = 6;
inline constexpr int IntentFontSize        = 15;
inline constexpr int IntentTooltipWidth    = 200;
inline constexpr int IntentTooltipHeight   = 42;
inline constexpr int IntentTooltipOffsetX  = 4;
inline constexpr int IntentTooltipFontSize = 13;
inline constexpr int IntentTooltipPadding  = 6;

// ## UI / Entity Stats ##
inline constexpr int EntityNameFontSize    = 20;
inline constexpr int EntityTextPadding     = 10;
inline constexpr int HealthBarHeight       = 18;
inline constexpr int HealthBarOffsetY      = 35;
inline constexpr int HealthTextOffsetY     = 58;
inline constexpr int ManaTextOffsetY       = 78;
inline constexpr int BlockTextOffsetY      = 98;
inline constexpr int EntityStatFontSize    = 16;

// ## UI / Card Text ##
inline constexpr int CardNameFontSize      = 12;
inline constexpr int HoveredCardNameSize   = 14;
inline constexpr int CardDescriptionSize   = 10;
inline constexpr int CardDescriptionGap    = 2;
inline constexpr int CardFooterMargin      = 20;
inline constexpr int CardFooterSize        = 11;
inline constexpr int HoveredCardFooterSize = 13;
inline constexpr int CardTextPadding       = 5;
inline constexpr int CardTextTopPadding    = 4;
inline constexpr int CardTextGap           = 4;
inline constexpr int CardDividerPadding    = 2;
inline constexpr int CardDescriptionLines  = 2;
inline constexpr int CardRightStatPadding  = 4;
} // namespace LayoutConfig

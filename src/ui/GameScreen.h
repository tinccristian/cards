#pragma once

#include "app/AppSettings.h"
#include "audio/CardAudio.h"
#include "CardFaceCache.h"
#include "Colors.h"
#include "config/Defines.h"
#include "content/MapData.h"
#include "core/Card.h"
#include "core/DamageResult.h"
#include "core/Deck.h"
#include "core/GameState.h"
#include "EnemySprite.h"
#include "PlayerSprite.h"
#include "gameplay/MapRunState.h"
#include "raylib.h"
#include <string>
#include <vector>

// Handles all rendering and input detection for every game screen.
enum class MenuAction {
    None = -1,
    NewGame = 0,
    Options = 1,
    Quit = 2
};

enum class PauseAction {
    None,
    Resume,
    Options,
    MainMenu,
    Quit
};

enum class OptionsMenuAction {
    None,
    Back
};

enum class GameOverAction {
    None,
    NewRun,
    MainMenu
};

enum class RewardPopupAction {
    None,
    CollectGold,
    OpenCardChoice,
    Continue
};

enum class RunHudAction {
    None,
    ToggleMap,
    Back,
    ViewDeck
};

enum class DamageNumberTarget {
    Player,
    Enemy
};

enum class DamageNumberStyle {
    Health,
    Block,
    Poison
};

class GameScreen {
public:
    static constexpr int RewardChoiceNone = -1;
    static constexpr int RewardChoiceSkip = -2;

    GameScreen(int screenWidth, int screenHeight, CardAudio* cardAudio = nullptr);

    // Draw the main menu and report the clicked action for this frame.
    MenuAction drawMenu(bool allowInteraction = true);

    // Draw the combat screen.
    //   endTurnClicked: set to true if End Turn was pressed (only during PLAYER_TURN).
    //   drawPileClicked / discardPileClicked: set if corresponding widget was clicked.
    //   Returns hand index of card clicked (-1 if none).
    int drawCombat(GameState& state, bool& endTurnClicked,
                   bool& drawPileClicked, bool& discardPileClicked,
                   bool allowInteraction = true);

    PauseAction drawPauseMenu();
    OptionsMenuAction drawOptionsMenu(AppSettings& settings,
                                      OptionsSection& activeSection,
                                      bool openedFromPause);
    int drawMapScreen(const MapData& mapData,
                      const MapRunState& runState,
                      bool allowInteraction = true);
    void resetMapView();

    // Draw a full-screen overlay listing cards in a pile.
    //   title: heading text ("Draw Pile" / "Discard Pile")
    //   cards: pile contents
    //   scrollOffset: first visible row index
    //   closeClicked: set to true if the X button was pressed
    // Returns the max scroll offset (caller may clamp against it).
    int drawPileViewer(const std::string& title,
                       const std::vector<Card>& cards,
                       int scrollOffset,
                       bool& closeClicked);
    RunHudAction drawRunHud(const Player& player,
                            const std::string& centerLabel,
                            bool allowInteraction = true,
                            bool showMapButton = true,
                            bool showBackButton = false,
                            bool showDeckButton = true);
    RewardPopupAction drawRewardPopup(const RewardState& rewards,
                                      int currentGold,
                                      int scrollOffset,
                                      int& maxScroll,
                                      bool allowInteraction = true);
    int drawRewardCardChoice(const RewardState& rewards,
                             bool allowInteraction = true);

    // Draw the game-over screen and return the selected action for this frame.
    GameOverAction drawGameOver(const GameState& state);

    void setTimeScale(float scale);
    void showNextTurnVignette();
    void addDamageNumber(DamageNumberTarget target,
                         int value,
                         DamageNumberStyle style,
                         int hitIndex = 0,
                         float delaySecs = 0.0f);
    void addDamageNumbers(DamageNumberTarget target,
                          const std::vector<DamageBreakdown>& events,
                          int baseHitIndex = 0,
                          float baseDelaySecs = 0.0f,
                          float delayStepSecs = LayoutConfig::HitEventDelayStep);
    void resetCombatEffects();

    void unloadAssets();

    // Returns true once the enemy death animation AND death dissolve have finished.
    // Used by main.cpp to hold the MAP transition until everything completes.
    bool isEnemyDeathAnimDone() const;

    // Returns true once the player death dissolve has finished (or if no shader loaded).
    // Used by main.cpp to hold the GAME_OVER transition.
    bool isPlayerDeathDissolveComplete() const;

    // Pile widget rectangles (needed by main.cpp for click detection)
    Rectangle drawPileRect()    const;
    Rectangle discardPileRect() const;

private:
    struct HandLayoutCard {
        Rectangle bounds;
        float     rotation = 0.0f;
        bool      scaled   = false;
    };

    int          m_width;
    int          m_height;
    float        m_timeScale = 1.0f;
    int          m_hoveredCardIndex = -1;
    float        m_hoverProgress      = 0.0f;
    int          m_hoverProgressIndex = -1;
    int          m_selectedCardIndex = -1;
    float        m_wiggleTime       = 0.0f;
    float        m_selectedCardBlend = 0.0f;
    int          m_draggedCardIndex = -1;
    bool         m_selectedCardPinned = false;
    float        m_mapScrollOffset  = 0.0f;
    bool         m_mapDragging      = false;
    bool         m_mapViewInitialized = false;
    float        m_mapDragStartMouseY = 0.0f;
    float        m_mapDragStartOffset = 0.0f;
    Vector2      m_dragGrabOffset   = { 0.0f, 0.0f };
    CardAudio*   m_cardAudio        = nullptr;
    Texture2D    m_mapTexture       = {};
    bool         m_mapTextureLoaded = false;
    std::string  m_loadedMapTexturePath;
    EnemySprite  m_enemySprite;
    std::string  m_loadedEnemySpritePath; // tracks which sheet is currently loaded
    PlayerSprite m_playerSprite;
    bool         m_playerSpriteLoaded = false;
    int          m_lastPlayerHp       = -1;
    int          m_lastEnemyHp        = -1;
    Texture2D    m_blockIcon          = {};
    bool         m_blockIconLoaded    = false;
    Texture2D    m_attackIcon         = {};
    bool         m_attackIconLoaded   = false;
    Texture2D    m_buffIcon           = {};
    bool         m_buffIconLoaded     = false;
    Texture2D    m_debuffIcon         = {};
    bool         m_debuffIconLoaded   = false;
    Texture2D    m_poisonIcon         = {};
    bool         m_poisonIconLoaded   = false;
    bool         m_mapHudHoveredLast  = false;
    bool         m_deckHudHoveredLast = false;
    bool         m_backHudHoveredLast = false;
    std::string  m_lastPileViewerHoverToken;
    Shader       m_intentFloatShader  = {};
    bool         m_intentFloatShaderLoaded = false;
    int          m_intentFloatTimeLoc = -1;
    int          m_intentFloatAmpLoc  = -1;
    int          m_intentFloatSpeedLoc = -1;
    int          m_intentFloatPhaseLoc = -1;
    mutable CardFaceCache m_cardFaceCache;
    float        m_turnVignetteTimer = 0.0f;
    float        m_enemyDeathPresentationTimer = 0.0f;
    float        m_playerDeathPresentationTimer = 0.0f;
    bool         m_enemyDeathPresentationStarted = false;
    bool         m_playerDeathPresentationStarted = false;

    struct FloatingDamageNumber {
        DamageNumberTarget target = DamageNumberTarget::Enemy;
        int   value = 0;
        DamageNumberStyle style = DamageNumberStyle::Health;
        float age = 0.0f;
        float delaySecs = 0.0f;
        float xOffset = 0.0f;
        float yOffset = 0.0f;
    };
    std::vector<FloatingDamageNumber> m_damageNumbers;

    // Deferred tooltip: populated during the frame, flushed at the very end of
    // drawCombat so it always renders on top of every other element.
    struct PendingTooltip {
        bool        active = false;
        std::string title;
        std::string body;
        Rectangle   anchor = {};  // source rect — used to position the tooltip
    };
    mutable PendingTooltip m_pendingTooltip;

    // --- helpers ---
    void syncWindowSize();
    float uiScale() const;
    int scalei(int value) const;
    float scalef(float value) const;
    void ensureMapTextureLoaded(const std::string& texturePath);
    float clampedMapOffset(float offset) const;
    Rectangle mapTextureRect() const;
    void drawButton(Rectangle rect, const std::string& text, bool hovered) const;
    void drawHealthBar(Rectangle bar, float ratio, bool hasBlock = false) const;
    void drawEntityHud(Rectangle spriteRect, const std::string& name,
                       int health, int maxHealth, int block,
                       const StatusCollection* statuses = nullptr) const;
    void drawManaHud(const Player& player) const;
    // playerMana: current mana (used to colour cost red when unaffordable). -1 = neutral.
    void drawCardFace(Rectangle rect, const Card& card, bool scaled, float rotationDegrees,
                      int playerMana = -1,
                      bool crispPresentation = false) const;
    void drawCardTooltip(const Card& card, float x, float y) const;
    void drawIntentIndicator(const Enemy& enemy, Rectangle enemySpriteRect) const;

    // Queue a deferred tooltip to be drawn at end-of-frame (renders on top of everything).
    void queueTooltip(const std::string& title, const std::string& body, Rectangle anchor) const;
    // Flush and draw any queued tooltip.
    void flushTooltip() const;
    void drawVignetteOverlay() const;
    void drawTurnVignetteOverlay() const;
    void drawDeathPresentationOverlay() const;
    void drawFloatingDamageNumbers(Rectangle playerSpriteRect, Rectangle enemySpriteRect) const;
    void updateTransientEffects(float dt);

    // Draw a pile widget (stack visual + label + count). Returns true if clicked.
    bool drawPileWidget(Rectangle rect, const std::string& label,
                        int count, Color accentColor) const;
    int drawStepperRow(float y, const std::string& label, const std::string& value) const;
    bool drawCheckboxRow(float y, const std::string& label, bool checked) const;

    Rectangle handDropZone() const;
    Rectangle drawPileButtonRect() const;
    Rectangle endTurnButtonRect() const;
    std::vector<HandLayoutCard> buildHandLayout(int cardCount, int draggedCardIndex) const;
    HandLayoutCard selectedCardLayout(const HandLayoutCard& baseLayout) const;
    HandLayoutCard blendLayout(const HandLayoutCard& from, const HandLayoutCard& to, float blend) const;
    int handInsertIndexFromMouseX(const std::vector<HandLayoutCard>& layout, float mouseX) const;

    static float archOffset(int i, int n);
    static bool  mouseOver(Rectangle rect);
};

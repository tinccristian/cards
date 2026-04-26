#pragma once

#include "content/MapData.h"
#include "core/GameState.h"
#include "gameplay/MapRunState.h"
#include "ui/GameScreen.h"

#include <string>
#include <vector>

class DevConsole {
public:
    void beginFrame(GameState& state, MapData& activeMap, MapRunState& mapRun);
    void draw(GameScreen& screen, GameState& state, MapData& activeMap, MapRunState& mapRun);
    bool blocksGameInput() const;
    bool wantsMapView() const;

private:
    enum class EditorMode {
        None,
        Map,
        Card
    };

    enum class MapTool {
        AddNode,
        LinkNodes,
        DeleteNode
    };

    struct EnemyOption {
        std::string id;
        std::string label;
    };

    struct CardBox {
        std::string id;
        std::string label;
        int left = 0;
        int top = 0;
        int right = 0;
        int bottom = 0;
        Color color = WHITE;
    };

    bool m_open = false;
    bool m_skipNextToggleCharacter = false;
    EditorMode m_editorMode = EditorMode::None;
    MapTool m_mapTool = MapTool::AddNode;
    std::string m_input;
    std::vector<std::string> m_log;
    std::vector<std::string> m_history;
    int m_historyIndex = -1;
    int m_autocompleteIndex = -1;
    std::string m_autocompleteSeed;
    float m_backspaceRepeatAt = 0.0f;

    std::vector<EnemyOption> m_enemyOptions;
    int m_selectedEnemyIndex = 0;
    int m_selectedNodeTypeIndex = 0;
    bool m_newNodeInitiallyUnlocked = false;
    int m_linkStartNodeIndex = -1;
    int m_selectedMapNodeIndex = -1;

    std::vector<CardBox> m_cardBoxes;
    int m_selectedCardBoxIndex = 0;
    bool m_draggingCardBox = false;
    bool m_resizingCardBox = false;
    Vector2 m_cardDragStartMouse = {};
    CardBox m_cardDragStartBox = {};
    bool m_cardPreviewHasSavedLayout = false;
    CardFaceCache::FaceLayout m_cardPreviewSavedLayout = {};
    std::string m_statusMessage;
    Color m_statusColor = WHITE;
    float m_statusUntil = 0.0f;

    void handleConsoleInput(GameState& state);
    void deletePreviousWord();
    void executeInput(GameState& state);
    void executeCommand(const std::string& command, GameState& state);
    void autocomplete();
    void log(const std::string& text);
    std::vector<std::pair<std::string, std::string>> commands() const;

    void enterMapEditor();
    void enterCardEditor();
    void exitEditor();

    void refreshEnemyOptions();
    void drawMapEditor(GameScreen& screen, MapData& activeMap, MapRunState& mapRun);
    void handleMapEditorInput(GameScreen& screen, MapData& activeMap, MapRunState& mapRun);
    void saveMap(MapData& activeMap);
    std::string resolveWritableMapPath(const MapData& activeMap) const;
    int mapNodeAtMouse(GameScreen& screen, const MapData& activeMap) const;
    std::string nextNodeId(const MapData& activeMap, const std::string& typeId) const;
    void rebuildMapIndexes(MapData& activeMap) const;

    void drawCardEditor(GameScreen& screen, GameState& state);
    void handleCardEditorInput();
    void initializeCardBoxes();
    void saveCardLayoutConstants();
    void showStatus(const std::string& message, Color color);
    void drawStatusBanner() const;
    std::string resolveSourcePath(const std::string& relativePath) const;
    Rectangle cardPreviewRect() const;
    void drawBoxResizeHandles(Rectangle rect, Color color, bool selected) const;
    CardFaceCache::FaceLayout currentCardFaceLayout() const;

    Rectangle consoleRect() const;
};

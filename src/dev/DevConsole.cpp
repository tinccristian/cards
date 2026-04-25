#include "DevConsole.h"

#include "config/Defines.h"
#include "content/EnemyCatalog.h"
#include "core/CardDatabase.h"
#include "ui/Colors.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <sstream>
#include <unordered_set>

namespace {

std::string trim(const std::string& value) {
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c); });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c); }).base();
    if (begin >= end) {
        return "";
    }
    return std::string(begin, end);
}

bool startsWith(const std::string& value, const std::string& prefix) {
    return value.size() >= prefix.size()
        && std::equal(prefix.begin(), prefix.end(), value.begin());
}

std::string lowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return value;
}

void drawReadableText(const std::string& text, int x, int y, int fontSize, Color color) {
    DrawText(text.c_str(), x - 1, y, fontSize, BLACK);
    DrawText(text.c_str(), x + 1, y, fontSize, BLACK);
    DrawText(text.c_str(), x, y - 1, fontSize, BLACK);
    DrawText(text.c_str(), x, y + 1, fontSize, BLACK);
    DrawText(text.c_str(), x, y, fontSize, color);
}

Rectangle sourceBoxToPreview(int left, int top, int right, int bottom, Rectangle preview) {
    const float sx = preview.width / (float)LayoutConfig::CardBorderSourceWidth;
    const float sy = preview.height / (float)LayoutConfig::CardBorderSourceHeight;
    return {
        preview.x + left * sx,
        preview.y + top * sy,
        std::max(1.0f, (right - left) * sx),
        std::max(1.0f, (bottom - top) * sy)
    };
}

void replaceConstant(std::string& text, const std::string& name, int value) {
    const std::string needle = "inline constexpr int " + name;
    const size_t start = text.find(needle);
    if (start == std::string::npos) {
        return;
    }
    const size_t equals = text.find('=', start);
    const size_t semicolon = text.find(';', equals);
    if (equals == std::string::npos || semicolon == std::string::npos) {
        return;
    }
    text.replace(equals + 1, semicolon - equals - 1, " " + std::to_string(value));
}

} // namespace

void DevConsole::beginFrame(MapData& activeMap, MapRunState& mapRun) {
    if (IsKeyPressed(KEY_GRAVE)) {
        m_open = !m_open;
        m_skipNextToggleCharacter = true;
    }

    if (m_open) {
        handleConsoleInput();
    }

    (void)activeMap;
    (void)mapRun;
}

void DevConsole::draw(GameScreen& screen, GameState& state, MapData& activeMap, MapRunState& mapRun) {
    if (m_editorMode == EditorMode::Map) {
        handleMapEditorInput(screen, activeMap, mapRun);
        drawMapEditor(screen, activeMap, mapRun);
    } else if (m_editorMode == EditorMode::Card) {
        handleCardEditorInput();
        drawCardEditor(screen, state);
    }

    drawStatusBanner();

    if (!m_open) {
        return;
    }

    const Rectangle panel = consoleRect();
    DrawRectangleRec(panel, Color{ 6, 8, 12, 246 });
    DrawRectangleLinesEx(panel, 3.0f, Colors::draw_pile_accent);

    const int pad = 16;
    const int lineHeight = 24;
    int y = (int)panel.y + pad;
    drawReadableText("Dev Console", (int)panel.x + pad, y, 24, WHITE);
    y += 34;

    const int visibleLines = std::max(1, ((int)panel.height - 92) / lineHeight);
    const int firstLine = std::max(0, (int)m_log.size() - visibleLines);
    for (int i = firstLine; i < (int)m_log.size(); ++i) {
        drawReadableText(m_log[i], (int)panel.x + pad, y, 18, Colors::text_primary);
        y += lineHeight;
    }

    const Rectangle inputRect = {
        panel.x + pad,
        panel.y + panel.height - 44.0f,
        panel.width - pad * 2.0f,
        32.0f
    };
    DrawRectangleRec(inputRect, Colors::light_bg);
    DrawRectangleLinesEx(inputRect, 2.0f, Colors::draw_pile_accent);
    const std::string prompt = "> " + m_input + (((int)(GetTime() * 2.0) % 2 == 0) ? "_" : "");
    drawReadableText(prompt, (int)inputRect.x + 10, (int)inputRect.y + 6, 20, WHITE);
}

bool DevConsole::blocksGameInput() const {
    return m_open || m_editorMode != EditorMode::None;
}

bool DevConsole::wantsMapView() const {
    return m_editorMode == EditorMode::Map;
}

void DevConsole::handleConsoleInput() {
    int key = GetCharPressed();
    while (key > 0) {
        if (m_skipNextToggleCharacter && (key == '`' || key == '~')) {
            m_skipNextToggleCharacter = false;
        } else if (key >= 32 && key <= 126) {
            m_input.push_back((char)key);
            m_autocompleteIndex = -1;
        }
        key = GetCharPressed();
    }

    const bool ctrlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    const float now = (float)GetTime();
    if (ctrlDown && IsKeyPressed(KEY_BACKSPACE)) {
        deletePreviousWord();
        m_backspaceRepeatAt = now + 0.38f;
    } else if (IsKeyPressed(KEY_BACKSPACE) && !m_input.empty()) {
        m_input.pop_back();
        m_autocompleteIndex = -1;
        m_backspaceRepeatAt = now + 0.38f;
    } else if (!ctrlDown && IsKeyDown(KEY_BACKSPACE) && !m_input.empty() && now >= m_backspaceRepeatAt) {
        m_input.pop_back();
        m_autocompleteIndex = -1;
        m_backspaceRepeatAt = now + 0.045f;
    }
    if (IsKeyPressed(KEY_ENTER)) {
        executeInput();
    }
    if (IsKeyPressed(KEY_TAB)) {
        autocomplete();
    }
    if (IsKeyPressed(KEY_UP) && !m_history.empty()) {
        if (m_historyIndex < 0) {
            m_historyIndex = (int)m_history.size() - 1;
        } else {
            m_historyIndex = std::max(0, m_historyIndex - 1);
        }
        m_input = m_history[m_historyIndex];
    }
    if (IsKeyPressed(KEY_DOWN) && !m_history.empty()) {
        if (m_historyIndex >= 0) {
            m_historyIndex++;
            if (m_historyIndex >= (int)m_history.size()) {
                m_historyIndex = -1;
                m_input.clear();
            } else {
                m_input = m_history[m_historyIndex];
            }
        }
    }
}

void DevConsole::deletePreviousWord() {
    while (!m_input.empty() && std::isspace((unsigned char)m_input.back())) {
        m_input.pop_back();
    }
    while (!m_input.empty() && !std::isspace((unsigned char)m_input.back())) {
        m_input.pop_back();
    }
    m_autocompleteIndex = -1;
}

void DevConsole::executeInput() {
    const std::string command = trim(m_input);
    if (command.empty()) {
        return;
    }
    m_history.push_back(command);
    m_historyIndex = -1;
    log("> " + command);
    m_input.clear();
    executeCommand(command);
}

void DevConsole::executeCommand(const std::string& command) {
    const std::string normalized = lowerCopy(command);
    if (normalized == "help") {
        for (const auto& [name, description] : commands()) {
            log(name + " - " + description);
        }
        return;
    }
    if (normalized == "mapeditor") {
        enterMapEditor();
        return;
    }
    if (normalized == "cardeditor") {
        enterCardEditor();
        return;
    }
    if (normalized == "exit") {
        exitEditor();
        return;
    }
    if (normalized == "clear") {
        m_log.clear();
        return;
    }
    log("Unknown command. Type help.");
}

void DevConsole::autocomplete() {
    std::vector<std::string> matches;
    const std::string seed = lowerCopy(m_input);
    for (const auto& [name, description] : commands()) {
        (void)description;
        if (startsWith(lowerCopy(name), seed)) {
            matches.push_back(name);
        }
    }
    if (matches.empty()) {
        return;
    }
    if (m_autocompleteSeed != seed || m_autocompleteIndex < 0) {
        m_autocompleteSeed = seed;
        m_autocompleteIndex = 0;
    } else {
        m_autocompleteIndex = (m_autocompleteIndex + 1) % (int)matches.size();
    }
    m_input = matches[m_autocompleteIndex];
}

void DevConsole::log(const std::string& text) {
    m_log.push_back(text);
    if (m_log.size() > 80) {
        m_log.erase(m_log.begin());
    }
}

std::vector<std::pair<std::string, std::string>> DevConsole::commands() const {
    return {
        { "help", "Show all commands." },
        { "mapEditor", "Open the active map editor." },
        { "cardEditor", "Open the card zone editor." },
        { "exit", "Close the active editor." },
        { "clear", "Clear console output." }
    };
}

void DevConsole::enterMapEditor() {
    refreshEnemyOptions();
    m_editorMode = EditorMode::Map;
    m_mapTool = MapTool::AddNode;
    m_linkStartNodeIndex = -1;
    m_selectedMapNodeIndex = -1;
    log("Map editor opened. A:add L:link D:delete S:save T:type E:enemy U:unlock.");
}

void DevConsole::enterCardEditor() {
    initializeCardBoxes();
    m_cardPreviewHasSavedLayout = false;
    m_editorMode = EditorMode::Card;
    log("Card editor opened. Drag boxes to move. Drag the square handle to resize. S saves Defines.h.");
}

void DevConsole::exitEditor() {
    m_editorMode = EditorMode::None;
    m_draggingCardBox = false;
    m_resizingCardBox = false;
    log("Editor closed.");
}

void DevConsole::refreshEnemyOptions() {
    m_enemyOptions.clear();
    m_enemyOptions.push_back({ "random", "random" });
    for (const auto& enemy : EnemyCatalog::scan(AssetPaths::ENEMY_DIRECTORY)) {
        m_enemyOptions.push_back({ enemy.id, enemy.id + " (" + enemy.name + ")" });
    }
    m_selectedEnemyIndex = std::clamp(m_selectedEnemyIndex, 0, std::max(0, (int)m_enemyOptions.size() - 1));
}

void DevConsole::drawMapEditor(GameScreen& screen, MapData& activeMap, MapRunState& mapRun) {
    (void)mapRun;
    const std::string tool = m_mapTool == MapTool::AddNode ? "add node"
        : m_mapTool == MapTool::LinkNodes ? "link nodes"
        : "delete node";
    const std::string typeLabel = activeMap.nodeTypes.empty()
        ? "none"
        : activeMap.nodeTypes[std::clamp(m_selectedNodeTypeIndex, 0, (int)activeMap.nodeTypes.size() - 1)].label;
    const std::string enemyLabel = m_enemyOptions.empty()
        ? "none"
        : m_enemyOptions[std::clamp(m_selectedEnemyIndex, 0, (int)m_enemyOptions.size() - 1)].label;
    const Rectangle panel = { 0.0f, 108.0f, 308.0f, 360.0f };
    DrawRectangleRec(panel, Color{ 6, 8, 12, 232 });
    DrawRectangleLinesEx(panel, 2.0f, Colors::draw_pile_accent);

    int y = (int)panel.y + 14;
    drawReadableText("mapEditor [" + activeMap.id + "]", (int)panel.x + 16, y, 22, WHITE);
    y += 34;
    drawReadableText("Tool: " + tool, (int)panel.x + 16, y, 18, Colors::text_primary);
    y += 24;
    drawReadableText("Type: " + typeLabel, (int)panel.x + 16, y, 18, Colors::text_primary);
    y += 24;
    drawReadableText("Enemy: " + enemyLabel, (int)panel.x + 16, y, 18, Colors::text_primary);
    y += 24;
    drawReadableText(std::string("Initially unlocked: ") + (m_newNodeInitiallyUnlocked ? "yes" : "no"),
                     (int)panel.x + 16, y, 18, Colors::text_primary);
    y += 34;

    const std::vector<std::string> tips = {
        "A  Add node",
        "L  Link nodes",
        "D  Delete node",
        "T  Next type",
        "E  Next enemy",
        "U  Toggle unlocked",
        "S  Save map JSON",
        "ESC / exit  Close"
    };
    for (const std::string& tip : tips) {
        drawReadableText(tip, (int)panel.x + 16, y, 16, Colors::text_secondary);
        y += 23;
    }

    for (int i = 0; i < (int)activeMap.nodes.size(); ++i) {
        const auto& node = activeMap.nodes[i];
        const Vector2 p = screen.debugMapSourceToScreen(activeMap, { node.x, node.y });
        const bool selected = i == m_selectedMapNodeIndex || i == m_linkStartNodeIndex;
        DrawCircleLines((int)std::round(p.x), (int)std::round(p.y), 30.0f,
                        selected ? Colors::gold_color : Colors::text_primary);
        drawReadableText(node.id, (int)p.x + 28, (int)p.y - 8, 13, WHITE);
    }
}

void DevConsole::handleMapEditorInput(GameScreen& screen, MapData& activeMap, MapRunState& mapRun) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        exitEditor();
        return;
    }
    if (IsKeyPressed(KEY_A)) {
        m_mapTool = MapTool::AddNode;
        m_linkStartNodeIndex = -1;
    }
    if (IsKeyPressed(KEY_L)) {
        m_mapTool = MapTool::LinkNodes;
        m_linkStartNodeIndex = -1;
    }
    if (IsKeyPressed(KEY_D)) {
        m_mapTool = MapTool::DeleteNode;
        m_linkStartNodeIndex = -1;
    }
    if (IsKeyPressed(KEY_T) && !activeMap.nodeTypes.empty()) {
        m_selectedNodeTypeIndex = (m_selectedNodeTypeIndex + 1) % (int)activeMap.nodeTypes.size();
    }
    if (IsKeyPressed(KEY_E)) {
        refreshEnemyOptions();
        if (!m_enemyOptions.empty()) {
            m_selectedEnemyIndex = (m_selectedEnemyIndex + 1) % (int)m_enemyOptions.size();
        }
    }
    if (IsKeyPressed(KEY_U)) {
        m_newNodeInitiallyUnlocked = !m_newNodeInitiallyUnlocked;
    }
    if (IsKeyPressed(KEY_S)) {
        saveMap(activeMap);
        mapRun.initialize(activeMap);
    }

    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || m_open) {
        return;
    }

    const int hitNode = mapNodeAtMouse(screen, activeMap);
    if (m_mapTool == MapTool::DeleteNode) {
        if (hitNode >= 0) {
            const std::string removedId = activeMap.nodes[hitNode].id;
            activeMap.nodes.erase(activeMap.nodes.begin() + hitNode);
            activeMap.connections.erase(
                std::remove_if(activeMap.connections.begin(), activeMap.connections.end(),
                               [&](const MapConnectionDefinition& connection) {
                                   return connection.fromId == removedId || connection.toId == removedId;
                               }),
                activeMap.connections.end());
            rebuildMapIndexes(activeMap);
            mapRun.initialize(activeMap);
            log("Deleted node " + removedId + ".");
        }
        return;
    }

    if (m_mapTool == MapTool::LinkNodes) {
        if (hitNode < 0) {
            return;
        }
        if (m_linkStartNodeIndex < 0) {
            m_linkStartNodeIndex = hitNode;
            m_selectedMapNodeIndex = hitNode;
            return;
        }
        if (m_linkStartNodeIndex != hitNode) {
            const std::string from = activeMap.nodes[m_linkStartNodeIndex].id;
            const std::string to = activeMap.nodes[hitNode].id;
            const bool exists = std::any_of(activeMap.connections.begin(), activeMap.connections.end(),
                                            [&](const MapConnectionDefinition& c) {
                                                return c.fromId == from && c.toId == to;
                                            });
            if (!exists) {
                activeMap.connections.push_back({ from, to });
                log("Linked " + from + " -> " + to + ".");
            }
        }
        m_linkStartNodeIndex = -1;
        return;
    }

    if (m_mapTool == MapTool::AddNode) {
        const Rectangle mapRect = screen.debugMapTextureRect();
        if (!CheckCollisionPointRec(GetMousePosition(), mapRect) || activeMap.nodeTypes.empty()) {
            return;
        }
        const auto& nodeType = activeMap.nodeTypes[std::clamp(m_selectedNodeTypeIndex, 0, (int)activeMap.nodeTypes.size() - 1)];
        const Vector2 source = screen.debugMapScreenToSource(activeMap, GetMousePosition());
        MapNodeDefinition node;
        node.id = nextNodeId(activeMap, nodeType.id);
        node.typeId = nodeType.id;
        node.encounterId = nodeType.kind == MapNodeKind::Event
            ? "noah"
            : (m_enemyOptions.empty() ? "random" : m_enemyOptions[std::clamp(m_selectedEnemyIndex, 0, (int)m_enemyOptions.size() - 1)].id);
        node.x = source.x;
        node.y = source.y;
        node.initiallyUnlocked = m_newNodeInitiallyUnlocked;
        activeMap.nodes.push_back(node);
        rebuildMapIndexes(activeMap);
        mapRun.initialize(activeMap);
        m_selectedMapNodeIndex = activeMap.findNodeIndex(node.id);
        log("Added node " + node.id + ".");
    }
}

void DevConsole::saveMap(MapData& activeMap) {
    const std::string savePath = resolveWritableMapPath(activeMap);
    if (savePath.empty()) {
        log("Map save failed: active map has no source path.");
        showStatus("Map save failed: no source path", Colors::damage_color);
        return;
    }

    nlohmann::json root;
    root["id"] = activeMap.id;
    root["texturePath"] = activeMap.texturePath;
    root["sourceWidth"] = activeMap.sourceWidth;
    root["sourceHeight"] = activeMap.sourceHeight;
    root["nodes"] = nlohmann::json::array();
    for (const auto& node : activeMap.nodes) {
        root["nodes"].push_back({
            { "id", node.id },
            { "type", node.typeId },
            { "encounterId", node.encounterId },
            { "x", (int)std::lround(node.x) },
            { "y", (int)std::lround(node.y) },
            { "initiallyUnlocked", node.initiallyUnlocked }
        });
    }
    root["connections"] = nlohmann::json::array();
    for (const auto& connection : activeMap.connections) {
        root["connections"].push_back({
            { "from", connection.fromId },
            { "to", connection.toId }
        });
    }

    std::ofstream file(savePath);
    if (!file.is_open()) {
        log("Map save failed: cannot write " + savePath + ".");
        showStatus("Map save failed", Colors::damage_color);
        return;
    }
    file << root.dump(2) << "\n";
    log("MAP SAVED: " + activeMap.id + " -> " + savePath);
    showStatus("Map saved: " + activeMap.id, Colors::heal_color);
}

std::string DevConsole::resolveWritableMapPath(const MapData& activeMap) const {
    if (activeMap.sourcePath.empty()) {
        return "";
    }

    const std::filesystem::path original(activeMap.sourcePath);
    if (original.is_absolute()) {
        return original.string();
    }

    std::error_code ec;
    const std::filesystem::path cwd = std::filesystem::current_path(ec);
    if (!ec) {
        const std::filesystem::path parentCandidate = cwd.parent_path() / original;
        if (std::filesystem::exists(parentCandidate.parent_path(), ec)) {
            const std::string cwdName = lowerCopy(cwd.filename().string());
            if (cwdName == "build" || cwdName == "dist") {
                return parentCandidate.string();
            }
        }
    }

    return original.string();
}

int DevConsole::mapNodeAtMouse(GameScreen& screen, const MapData& activeMap) const {
    const Vector2 mouse = GetMousePosition();
    for (int i = (int)activeMap.nodes.size() - 1; i >= 0; --i) {
        const Vector2 p = screen.debugMapSourceToScreen(activeMap, { activeMap.nodes[i].x, activeMap.nodes[i].y });
        if (CheckCollisionPointCircle(mouse, p, 30.0f)) {
            return i;
        }
    }
    return -1;
}

std::string DevConsole::nextNodeId(const MapData& activeMap, const std::string& typeId) const {
    std::unordered_set<std::string> used;
    for (const auto& node : activeMap.nodes) {
        used.insert(node.id);
    }
    for (int i = 1; i < 10000; ++i) {
        std::ostringstream candidate;
        candidate << typeId << "_" << i;
        if (used.find(candidate.str()) == used.end()) {
            return candidate.str();
        }
    }
    return typeId + "_new";
}

void DevConsole::rebuildMapIndexes(MapData& activeMap) const {
    activeMap.nodeIndexById.clear();
    activeMap.nodeTypeIndexById.clear();
    for (int i = 0; i < (int)activeMap.nodeTypes.size(); ++i) {
        activeMap.nodeTypeIndexById[activeMap.nodeTypes[i].id] = i;
    }
    for (int i = 0; i < (int)activeMap.nodes.size(); ++i) {
        activeMap.nodeIndexById[activeMap.nodes[i].id] = i;
    }
}

void DevConsole::drawCardEditor(GameScreen& screen, GameState& state) {
    const int screenW = GetScreenWidth();
    const int screenH = GetScreenHeight();
    DrawRectangle(0, 44, screenW, screenH - 44, Color{ 0, 0, 0, 190 });

    const Rectangle preview = cardPreviewRect();
    const std::vector<Card>& hand = state.getPlayer().getHand();
    const auto& cards = CardDatabase::getAllCards();
    const Card* previewCard = !hand.empty() ? &hand.front() : (!cards.empty() ? &cards.front() : nullptr);
    if (previewCard != nullptr) {
        if (m_cardPreviewHasSavedLayout) {
            screen.drawDebugCardFace(preview, *previewCard, m_cardPreviewSavedLayout);
        } else {
            screen.drawDebugCardFace(preview, *previewCard);
        }
    } else {
        DrawRectangleRec(preview, Colors::card_bg);
        DrawRectangleLinesEx(preview, 2.0f, Colors::card_border);
    }

    const Rectangle panel = { 0.0f, 108.0f, 430.0f, 230.0f };
    DrawRectangleRec(panel, Color{ 6, 8, 12, 232 });
    DrawRectangleLinesEx(panel, 2.0f, Colors::draw_pile_accent);
    int y = (int)panel.y + 12;
    drawReadableText("cardEditor", (int)panel.x + 16, y, 22, WHITE);
    y += 34;
    const std::string active = m_cardBoxes.empty() ? "none" : m_cardBoxes[m_selectedCardBoxIndex].label;
    drawReadableText("Active: " + active, (int)panel.x + 16, y, 17, Colors::text_primary);
    y += 28;
    if (previewCard != nullptr) {
        drawReadableText("Preview: " + previewCard->getDisplayName(), (int)panel.x + 16, y, 15, Colors::text_primary);
        y += 24;
    }
    const std::vector<std::string> tips = {
        "1/2/3/4  Select mana/art/name/text",
        "Drag inside a box  Move it",
        "Drag square handle  Resize it",
        "S  Save constants to Defines.h",
        "ESC or exit  Close editor"
    };
    for (const std::string& tip : tips) {
        drawReadableText(tip, (int)panel.x + 16, y, 16, Colors::text_secondary);
        y += 21;
    }

    for (int i = 0; i < (int)m_cardBoxes.size(); ++i) {
        const CardBox& box = m_cardBoxes[i];
        const Rectangle r = sourceBoxToPreview(box.left, box.top, box.right, box.bottom, preview);
        DrawRectangleRec(r, ColorAlpha(box.color, i == m_selectedCardBoxIndex ? 0.28f : 0.14f));
        DrawRectangleLinesEx(r, i == m_selectedCardBoxIndex ? 3.0f : 2.0f, box.color);
        drawReadableText(box.label, (int)r.x + 4, (int)r.y + 4, 13, box.color);
        drawBoxResizeHandles(r, box.color, i == m_selectedCardBoxIndex);
    }
}

void DevConsole::handleCardEditorInput() {
    if (IsKeyPressed(KEY_ESCAPE)) {
        exitEditor();
        return;
    }
    if (IsKeyPressed(KEY_ONE)) m_selectedCardBoxIndex = 0;
    if (IsKeyPressed(KEY_TWO) && m_cardBoxes.size() > 1) m_selectedCardBoxIndex = 1;
    if (IsKeyPressed(KEY_THREE) && m_cardBoxes.size() > 2) m_selectedCardBoxIndex = 2;
    if (IsKeyPressed(KEY_FOUR) && m_cardBoxes.size() > 3) m_selectedCardBoxIndex = 3;
    if (IsKeyPressed(KEY_S)) {
        saveCardLayoutConstants();
    }

    if (m_cardBoxes.empty() || m_open) {
        return;
    }

    const Rectangle preview = cardPreviewRect();
    const float sx = (float)LayoutConfig::CardBorderSourceWidth / preview.width;
    const float sy = (float)LayoutConfig::CardBorderSourceHeight / preview.height;

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        for (int i = (int)m_cardBoxes.size() - 1; i >= 0; --i) {
            const Rectangle r = sourceBoxToPreview(m_cardBoxes[i].left, m_cardBoxes[i].top,
                                                   m_cardBoxes[i].right, m_cardBoxes[i].bottom, preview);
            const Rectangle handle = { r.x + r.width - 12.0f, r.y + r.height - 12.0f, 14.0f, 14.0f };
            if (CheckCollisionPointRec(GetMousePosition(), handle) || CheckCollisionPointRec(GetMousePosition(), r)) {
                m_selectedCardBoxIndex = i;
                m_draggingCardBox = true;
                m_resizingCardBox = CheckCollisionPointRec(GetMousePosition(), handle);
                m_cardDragStartMouse = GetMousePosition();
                m_cardDragStartBox = m_cardBoxes[i];
                break;
            }
        }
    }

    if (m_draggingCardBox && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        CardBox& box = m_cardBoxes[m_selectedCardBoxIndex];
        const Vector2 mouse = GetMousePosition();
        const int dx = (int)std::lround((mouse.x - m_cardDragStartMouse.x) * sx);
        const int dy = (int)std::lround((mouse.y - m_cardDragStartMouse.y) * sy);
        if (m_resizingCardBox) {
            box.right = std::clamp(m_cardDragStartBox.right + dx, box.left + 1, LayoutConfig::CardBorderSourceWidth);
            box.bottom = std::clamp(m_cardDragStartBox.bottom + dy, box.top + 1, LayoutConfig::CardBorderSourceHeight);
        } else {
            const int width = m_cardDragStartBox.right - m_cardDragStartBox.left;
            const int height = m_cardDragStartBox.bottom - m_cardDragStartBox.top;
            box.left = std::clamp(m_cardDragStartBox.left + dx, 0, LayoutConfig::CardBorderSourceWidth - width);
            box.top = std::clamp(m_cardDragStartBox.top + dy, 0, LayoutConfig::CardBorderSourceHeight - height);
            box.right = box.left + width;
            box.bottom = box.top + height;
        }
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        m_draggingCardBox = false;
        m_resizingCardBox = false;
    }
}

void DevConsole::initializeCardBoxes() {
    m_cardBoxes = {
        { "CardManaBox", "1 Mana", LayoutConfig::CardManaBoxLeft, LayoutConfig::CardManaBoxTop,
          LayoutConfig::CardManaBoxRight, LayoutConfig::CardManaBoxBottom, Colors::gold_color },
        { "CardArtBox", "2 Art", LayoutConfig::CardArtBoxLeft, LayoutConfig::CardArtBoxTop,
          LayoutConfig::CardArtBoxRight, LayoutConfig::CardArtBoxBottom, Colors::damage_color },
        { "CardNameBox", "3 Name", LayoutConfig::CardNameBoxLeft, LayoutConfig::CardNameBoxTop,
          LayoutConfig::CardNameBoxRight, LayoutConfig::CardNameBoxBottom, Colors::draw_pile_accent },
        { "CardDescriptionBox", "4 Description", LayoutConfig::CardDescriptionBoxLeft, LayoutConfig::CardDescriptionBoxTop,
          LayoutConfig::CardDescriptionBoxRight, LayoutConfig::CardDescriptionBoxBottom, Colors::heal_color }
    };
    m_selectedCardBoxIndex = 0;
}

void DevConsole::saveCardLayoutConstants() {
    const std::string path = resolveSourcePath("src/config/Defines.h");
    std::ifstream in(path);
    if (!in.is_open()) {
        log("Card layout save failed: cannot read " + path + ".");
        showStatus("Card layout save failed", Colors::damage_color);
        return;
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string text = buffer.str();

    for (const CardBox& box : m_cardBoxes) {
        replaceConstant(text, box.id + "Left", box.left);
        replaceConstant(text, box.id + "Top", box.top);
        replaceConstant(text, box.id + "Right", box.right);
        replaceConstant(text, box.id + "Bottom", box.bottom);
    }

    std::ofstream out(path);
    if (!out.is_open()) {
        log("Card layout save failed: cannot write " + path + ".");
        showStatus("Card layout save failed", Colors::damage_color);
        return;
    }
    out << text;
    log("CARD LAYOUT SAVED: " + path + ". Rebuild to apply.");
    m_cardPreviewSavedLayout = currentCardFaceLayout();
    m_cardPreviewHasSavedLayout = true;
    showStatus("Card layout saved", Colors::heal_color);
}

void DevConsole::showStatus(const std::string& message, Color color) {
    m_statusMessage = message;
    m_statusColor = color;
    m_statusUntil = (float)GetTime() + 2.2f;
}

void DevConsole::drawStatusBanner() const {
    if (m_statusMessage.empty() || (float)GetTime() > m_statusUntil) {
        return;
    }
    const int fontSize = 24;
    const int padX = 18;
    const int padY = 10;
    const int textW = MeasureText(m_statusMessage.c_str(), fontSize);
    const Rectangle banner = {
        ((float)GetScreenWidth() - (float)textW - padX * 2.0f) * 0.5f,
        58.0f,
        (float)textW + padX * 2.0f,
        (float)fontSize + padY * 2.0f
    };
    DrawRectangleRec(banner, Color{ 6, 8, 12, 238 });
    DrawRectangleLinesEx(banner, 2.0f, m_statusColor);
    drawReadableText(m_statusMessage,
                     (int)banner.x + padX,
                     (int)banner.y + padY,
                     fontSize,
                     m_statusColor);
}

std::string DevConsole::resolveSourcePath(const std::string& relativePath) const {
    const std::filesystem::path original(relativePath);
    if (original.is_absolute()) {
        return original.string();
    }

    std::error_code ec;
    const std::filesystem::path cwd = std::filesystem::current_path(ec);
    if (!ec) {
        const std::filesystem::path parentCandidate = cwd.parent_path() / original;
        if (std::filesystem::exists(parentCandidate.parent_path(), ec)) {
            const std::string cwdName = lowerCopy(cwd.filename().string());
            if (cwdName == "build" || cwdName == "dist") {
                return parentCandidate.string();
            }
        }
    }

    return original.string();
}

Rectangle DevConsole::cardPreviewRect() const {
    const int screenW = GetScreenWidth();
    const int screenH = GetScreenHeight();
    const float previewH = std::min((float)screenH - 150.0f, 560.0f);
    const float previewW = previewH * ((float)LayoutConfig::CardBorderSourceWidth / (float)LayoutConfig::CardBorderSourceHeight);
    return { (screenW - previewW) * 0.5f, 118.0f, previewW, previewH };
}

void DevConsole::drawBoxResizeHandles(Rectangle rect, Color color, bool selected) const {
    if (!selected) {
        return;
    }
    const Rectangle handle = { rect.x + rect.width - 12.0f, rect.y + rect.height - 12.0f, 14.0f, 14.0f };
    DrawRectangleRec(handle, color);
    DrawRectangleLinesEx(handle, 2.0f, BLACK);
}

CardFaceCache::FaceLayout DevConsole::currentCardFaceLayout() const {
    CardFaceCache::FaceLayout layout;
    for (const CardBox& box : m_cardBoxes) {
        if (box.id == "CardManaBox") {
            layout.manaLeft = box.left;
            layout.manaTop = box.top;
            layout.manaRight = box.right;
            layout.manaBottom = box.bottom;
        } else if (box.id == "CardArtBox") {
            layout.artLeft = box.left;
            layout.artTop = box.top;
            layout.artRight = box.right;
            layout.artBottom = box.bottom;
        } else if (box.id == "CardNameBox") {
            layout.nameLeft = box.left;
            layout.nameTop = box.top;
            layout.nameRight = box.right;
            layout.nameBottom = box.bottom;
        } else if (box.id == "CardDescriptionBox") {
            layout.descriptionLeft = box.left;
            layout.descriptionTop = box.top;
            layout.descriptionRight = box.right;
            layout.descriptionBottom = box.bottom;
        }
    }
    return layout;
}

Rectangle DevConsole::consoleRect() const {
    return {
        0.0f,
        0.0f,
        (float)GetScreenWidth(),
        std::max(260.0f, (float)GetScreenHeight() * 0.45f)
    };
}

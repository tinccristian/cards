#include "MapData.h"

#include "config/Defines.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>

namespace {

std::optional<nlohmann::json> readJsonFile(const std::string& path, std::string& error) {
    std::ifstream file(path);
    if (!file.is_open()) {
        error = "Cannot open JSON file: " + path;
        return std::nullopt;
    }

    try {
        nlohmann::json root;
        file >> root;
        return root;
    } catch (const nlohmann::json::exception& ex) {
        error = "JSON parse error in " + path + ": " + ex.what();
        return std::nullopt;
    }
}

std::optional<MapNodeKind> nodeKindFromString(const std::string& value) {
    if (value == "enemy") {
        return MapNodeKind::Enemy;
    }
    if (value == "reward") {
        return MapNodeKind::Reward;
    }
    if (value == "shop") {
        return MapNodeKind::Shop;
    }
    if (value == "boss") {
        return MapNodeKind::Boss;
    }
    return std::nullopt;
}

std::optional<Color> colorFromJson(const nlohmann::json& entry, std::string& error) {
    if (!entry.is_array() || entry.size() < 3 || entry.size() > 4) {
        error = "Color entries must be RGB or RGBA arrays";
        return std::nullopt;
    }

    Color color = WHITE;
    color.r = static_cast<unsigned char>(entry[0].get<int>());
    color.g = static_cast<unsigned char>(entry[1].get<int>());
    color.b = static_cast<unsigned char>(entry[2].get<int>());
    color.a = static_cast<unsigned char>(entry.size() > 3 ? entry[3].get<int>() : 255);
    return color;
}

bool loadNodeTypes(const std::string& path, MapData& mapData, std::string& error) {
    auto jsonOpt = readJsonFile(path, error);
    if (!jsonOpt) {
        return false;
    }

    const auto& root = *jsonOpt;
    if (!root.contains("nodeTypes") || !root["nodeTypes"].is_array()) {
        error = "Map node type file is missing a nodeTypes array: " + path;
        return false;
    }

    for (const auto& entry : root["nodeTypes"]) {
        const std::string id = entry.value("id", "");
        const std::string label = entry.value("label", "");
        const std::string kindValue = entry.value("kind", "");
        if (id.empty() || label.empty() || kindValue.empty()) {
            error = "Map node type is missing id, label, or kind";
            return false;
        }

        const auto kind = nodeKindFromString(kindValue);
        if (!kind) {
            error = "Unknown map node kind: " + kindValue;
            return false;
        }

        auto fillColor = colorFromJson(entry.value("fillColor", nlohmann::json::array()), error);
        if (!fillColor) {
            return false;
        }

        auto outlineColor = colorFromJson(entry.value("outlineColor", nlohmann::json::array()), error);
        if (!outlineColor) {
            return false;
        }

        if (mapData.nodeTypeIndexById.find(id) != mapData.nodeTypeIndexById.end()) {
            error = "Duplicate map node type id: " + id;
            return false;
        }

        mapData.nodeTypeIndexById[id] = static_cast<int>(mapData.nodeTypes.size());
        mapData.nodeTypes.push_back(MapNodeTypeDefinition{
            id,
            label,
            *kind,
            *fillColor,
            *outlineColor
        });
    }

    return true;
}

bool loadNodes(const nlohmann::json& root, MapData& mapData, std::string& error) {
    if (!root.contains("nodes") || !root["nodes"].is_array()) {
        error = "Map data is missing a nodes array";
        return false;
    }

    for (const auto& entry : root["nodes"]) {
        MapNodeDefinition node;
        node.id = entry.value("id", "");
        node.typeId = entry.value("type", "");
        node.encounterId = entry.value("encounterId", "");
        node.x = entry.value("x", 0.0f);
        node.y = entry.value("y", 0.0f);
        node.initiallyUnlocked = entry.value("initiallyUnlocked", false);

        if (node.id.empty() || node.typeId.empty()) {
            error = "Map node is missing id or type";
            return false;
        }

        if (mapData.findNodeType(node.typeId) == nullptr) {
            error = "Map node '" + node.id + "' references unknown node type: " + node.typeId;
            return false;
        }

        if (mapData.nodeIndexById.find(node.id) != mapData.nodeIndexById.end()) {
            error = "Duplicate map node id: " + node.id;
            return false;
        }

        mapData.nodeIndexById[node.id] = static_cast<int>(mapData.nodes.size());
        mapData.nodes.push_back(node);
    }

    return !mapData.nodes.empty();
}

bool loadConnections(const nlohmann::json& root, MapData& mapData, std::string& error) {
    if (!root.contains("connections") || !root["connections"].is_array()) {
        error = "Map data is missing a connections array";
        return false;
    }

    for (const auto& entry : root["connections"]) {
        const std::string fromId = entry.value("from", "");
        const std::string toId = entry.value("to", "");
        if (fromId.empty() || toId.empty()) {
            error = "Map connection is missing from or to";
            return false;
        }

        if (mapData.findNode(fromId) == nullptr || mapData.findNode(toId) == nullptr) {
            error = "Map connection references unknown node id";
            return false;
        }

        mapData.connections.push_back(MapConnectionDefinition{ fromId, toId });
    }

    return true;
}

} // namespace

int MapData::findNodeIndex(const std::string& nodeId) const {
    const auto it = nodeIndexById.find(nodeId);
    return (it == nodeIndexById.end()) ? -1 : it->second;
}

const MapNodeDefinition* MapData::findNode(const std::string& nodeId) const {
    const int index = findNodeIndex(nodeId);
    if (index < 0) {
        return nullptr;
    }
    return &nodes[index];
}

const MapNodeTypeDefinition* MapData::findNodeType(const std::string& typeId) const {
    const auto it = nodeTypeIndexById.find(typeId);
    if (it == nodeTypeIndexById.end()) {
        return nullptr;
    }
    return &nodeTypes[it->second];
}

std::vector<int> MapData::outgoingNodeIndices(int fromIndex) const {
    std::vector<int> result;
    if (fromIndex < 0 || fromIndex >= static_cast<int>(nodes.size())) {
        return result;
    }

    for (const auto& connection : connections) {
        if (connection.fromId != nodes[fromIndex].id) {
            continue;
        }

        const int toIndex = findNodeIndex(connection.toId);
        if (toIndex >= 0) {
            result.push_back(toIndex);
        }
    }

    return result;
}

bool MapContentLoader::loadMap(const std::string& nodeTypesPath,
                               const std::string& mapDataPath,
                               MapData& outMapData,
                               std::string& error) {
    error.clear();
    outMapData = {};

    if (!loadNodeTypes(nodeTypesPath, outMapData, error)) {
        return false;
    }

    auto jsonOpt = readJsonFile(mapDataPath, error);
    if (!jsonOpt) {
        return false;
    }

    const auto& root = *jsonOpt;
    outMapData.id = root.value("id", "");
    outMapData.texturePath = root.value("texturePath", "");
    outMapData.sourceWidth = root.value("sourceWidth", MapConfig::SourceWidth);
    outMapData.sourceHeight = root.value("sourceHeight", MapConfig::SourceHeight);

    if (outMapData.id.empty() || outMapData.texturePath.empty()
        || outMapData.sourceWidth <= 0.0f || outMapData.sourceHeight <= 0.0f) {
        error = "Map data is missing id, texturePath, sourceWidth, or sourceHeight";
        return false;
    }

    if (!loadNodes(root, outMapData, error)) {
        if (error.empty()) {
            error = "Map data does not contain any nodes";
        }
        return false;
    }

    if (!loadConnections(root, outMapData, error)) {
        return false;
    }

    return true;
}

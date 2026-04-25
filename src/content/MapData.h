#pragma once

#include "raylib.h"

#include <string>
#include <unordered_map>
#include <vector>

enum class MapNodeKind {
    Enemy,
    Event,
    Reward,
    Shop,
    Boss,
    Unknown
};

struct MapNodeTypeDefinition {
    std::string id;
    std::string label;
    MapNodeKind kind = MapNodeKind::Unknown;
    Color fillColor = WHITE;
    Color outlineColor = WHITE;
};

struct MapNodeDefinition {
    std::string id;
    std::string typeId;
    std::string encounterId;
    float x = 0.0f;
    float y = 0.0f;
    bool initiallyUnlocked = false;
};

struct MapConnectionDefinition {
    std::string fromId;
    std::string toId;
};

struct MapData {
    std::string id;
    std::string sourcePath;
    std::string texturePath;
    float sourceWidth = 0.0f;
    float sourceHeight = 0.0f;
    std::vector<MapNodeTypeDefinition> nodeTypes;
    std::vector<MapNodeDefinition> nodes;
    std::vector<MapConnectionDefinition> connections;
    std::unordered_map<std::string, int> nodeIndexById;
    std::unordered_map<std::string, int> nodeTypeIndexById;

    int findNodeIndex(const std::string& nodeId) const;
    const MapNodeDefinition* findNode(const std::string& nodeId) const;
    const MapNodeTypeDefinition* findNodeType(const std::string& typeId) const;
    std::vector<int> outgoingNodeIndices(int fromIndex) const;
};

class MapContentLoader {
public:
    static bool loadMap(const std::string& nodeTypesPath,
                        const std::string& mapDataPath,
                        MapData& outMapData,
                        std::string& error);
};

#pragma once

#include "content/MapData.h"

#include <vector>

struct MapNodeProgress {
    bool unlocked = false;
    bool completed = false;
};

class MapRunState {
public:
    void initialize(const MapData& mapData);

    bool isInitialized() const;
    bool isUnlocked(int nodeIndex) const;
    bool isCompleted(int nodeIndex) const;
    bool canEnterNode(const MapData& mapData, int nodeIndex) const;

    bool selectNode(const MapData& mapData, int nodeIndex);
    void completeActiveNode(const MapData& mapData);
    void clearActiveNode();
    int getActiveNodeIndex() const;

private:
    std::vector<MapNodeProgress> m_progress;
    int m_activeNodeIndex = -1;
};

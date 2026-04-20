#include "MapRunState.h"

void MapRunState::initialize(const MapData& mapData) {
    m_progress.assign(mapData.nodes.size(), {});
    m_activeNodeIndex = -1;

    for (int index = 0; index < static_cast<int>(mapData.nodes.size()); ++index) {
        m_progress[index].unlocked = mapData.nodes[index].initiallyUnlocked;
    }
}

bool MapRunState::isInitialized() const {
    return !m_progress.empty();
}

bool MapRunState::isUnlocked(int nodeIndex) const {
    return nodeIndex >= 0
        && nodeIndex < static_cast<int>(m_progress.size())
        && m_progress[nodeIndex].unlocked;
}

bool MapRunState::isCompleted(int nodeIndex) const {
    return nodeIndex >= 0
        && nodeIndex < static_cast<int>(m_progress.size())
        && m_progress[nodeIndex].completed;
}

bool MapRunState::canEnterNode(const MapData& mapData, int nodeIndex) const {
    return nodeIndex >= 0
        && nodeIndex < static_cast<int>(mapData.nodes.size())
        && isUnlocked(nodeIndex)
        && !isCompleted(nodeIndex)
        && m_activeNodeIndex < 0;
}

bool MapRunState::selectNode(const MapData& mapData, int nodeIndex) {
    if (!canEnterNode(mapData, nodeIndex)) {
        return false;
    }

    m_activeNodeIndex = nodeIndex;
    return true;
}

void MapRunState::completeActiveNode(const MapData& mapData) {
    if (m_activeNodeIndex < 0 || m_activeNodeIndex >= static_cast<int>(m_progress.size())) {
        return;
    }

    for (auto& nodeProgress : m_progress) {
        nodeProgress.unlocked = false;
    }

    m_progress[m_activeNodeIndex].completed = true;

    for (const int nextIndex : mapData.outgoingNodeIndices(m_activeNodeIndex)) {
        if (nextIndex >= 0 && nextIndex < static_cast<int>(m_progress.size())) {
            m_progress[nextIndex].unlocked = true;
        }
    }

    m_activeNodeIndex = -1;
}

void MapRunState::clearActiveNode() {
    m_activeNodeIndex = -1;
}

int MapRunState::getActiveNodeIndex() const {
    return m_activeNodeIndex;
}

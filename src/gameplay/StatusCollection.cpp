#include "StatusCollection.h"

#include <algorithm>

void StatusCollection::add(StatusType type, int magnitude, int duration, StatusDisposition disposition) {
    if (magnitude <= 0 || duration <= 0) {
        return;
    }

    auto it = std::find_if(m_statuses.begin(), m_statuses.end(),
        [type, disposition](const StatusInstance& status) {
            return status.type == type && status.disposition == disposition;
        });

    if (it != m_statuses.end()) {
        it->magnitude += magnitude;
        it->duration = std::max(it->duration, duration);
        return;
    }

    m_statuses.push_back(StatusInstance{ type, magnitude, duration, disposition });
}

int StatusCollection::getMagnitude(StatusType type) const {
    int total = 0;
    for (const auto& status : m_statuses) {
        if (status.type == type) {
            total += status.magnitude;
        }
    }
    return total;
}

bool StatusCollection::has(StatusType type) const {
    return getMagnitude(type) > 0;
}

int StatusCollection::consume(StatusType type) {
    int total = 0;

    auto it = m_statuses.begin();
    while (it != m_statuses.end()) {
        if (it->type == type) {
            total += it->magnitude;
            it = m_statuses.erase(it);
        } else {
            ++it;
        }
    }

    return total;
}

int StatusCollection::clearNegative() {
    int removed = 0;

    auto it = m_statuses.begin();
    while (it != m_statuses.end()) {
        if (it->disposition == StatusDisposition::Negative) {
            removed += it->magnitude;
            it = m_statuses.erase(it);
        } else {
            ++it;
        }
    }

    return removed;
}

void StatusCollection::remove(StatusType type) {
    m_statuses.erase(
        std::remove_if(m_statuses.begin(), m_statuses.end(),
            [type](const StatusInstance& status) {
                return status.type == type;
            }),
        m_statuses.end());
}

const std::vector<StatusInstance>& StatusCollection::getAll() const {
    return m_statuses;
}

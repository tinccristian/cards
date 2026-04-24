#pragma once

#include <vector>

enum class StatusType {
    BonusManaNextTurn,
    NextCardFree,
    SkipTurn,
    Poison,
    Infection,
    Weakness,
    Vulnerable
};

enum class StatusDisposition {
    Positive,
    Negative
};

struct StatusInstance {
    StatusType         type        = StatusType::BonusManaNextTurn;
    int                magnitude   = 0;
    int                duration    = 0;
    StatusDisposition  disposition = StatusDisposition::Positive;
};

class StatusCollection {
public:
    void add(StatusType type, int magnitude, int duration, StatusDisposition disposition);
    int  getMagnitude(StatusType type) const;
    int  getDuration(StatusType type) const;
    bool has(StatusType type) const;
    int  consume(StatusType type);
    int  tick(StatusType type);
    int  clearNegative();
    void remove(StatusType type);

    const std::vector<StatusInstance>& getAll() const;

private:
    std::vector<StatusInstance> m_statuses;
};

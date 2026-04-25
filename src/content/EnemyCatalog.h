#pragma once

#include <string>
#include <vector>

struct EnemyCatalogEntry {
    std::string id;
    std::string name;
    std::string path;
};

namespace EnemyCatalog {
std::vector<EnemyCatalogEntry> scan(const std::string& directory);
std::string pickRandomEnemyId(const std::string& directory);
}

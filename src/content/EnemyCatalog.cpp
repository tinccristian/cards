#include "EnemyCatalog.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <random>

namespace {

std::string pathStem(const std::filesystem::path& path) {
    return path.stem().string();
}

} // namespace

namespace EnemyCatalog {

std::vector<EnemyCatalogEntry> scan(const std::string& directory) {
    std::vector<EnemyCatalogEntry> entries;
    std::error_code ec;
    if (!std::filesystem::exists(directory, ec)) {
        return entries;
    }

    for (const auto& item : std::filesystem::directory_iterator(directory, ec)) {
        if (ec || !item.is_regular_file() || item.path().extension() != ".json") {
            continue;
        }

        EnemyCatalogEntry entry;
        entry.id = pathStem(item.path());
        entry.path = item.path().generic_string();
        entry.name = entry.id;

        std::ifstream file(item.path());
        if (file.is_open()) {
            try {
                nlohmann::json root;
                file >> root;
                entry.id = root.value("id", entry.id);
                entry.name = root.value("name", entry.name);
            } catch (const nlohmann::json::exception&) {
                entry.name = entry.id;
            }
        }

        if (!entry.id.empty()) {
            entries.push_back(std::move(entry));
        }
    }

    std::sort(entries.begin(), entries.end(),
              [](const EnemyCatalogEntry& a, const EnemyCatalogEntry& b) {
                  return a.id < b.id;
              });
    return entries;
}

std::string pickRandomEnemyId(const std::string& directory) {
    const auto entries = scan(directory);
    if (entries.empty()) {
        return "";
    }

    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> dist(0, static_cast<int>(entries.size()) - 1);
    return entries[dist(rng)].id;
}

} // namespace EnemyCatalog

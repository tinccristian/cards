#pragma once

#include <memory>
#include <string>

class Enemy;

namespace EnemyFactory {
std::unique_ptr<Enemy> loadFromJSON(const std::string& filePath, std::string& error);
std::unique_ptr<Enemy> loadById(const std::string& enemyId, std::string& error);
}

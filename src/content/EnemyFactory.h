#pragma once

#include <memory>
#include <string>

class Enemy;

namespace EnemyFactory {
std::unique_ptr<Enemy> loadFromJSON(const std::string& filePath, std::string& error);
}

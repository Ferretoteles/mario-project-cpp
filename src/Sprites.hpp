#pragma once

#include "Core.hpp"

namespace Sprites {
std::vector<sf::IntRect> playerFrames(const std::string& character, bool big, const std::string& state);
sf::IntRect enemyFrame(const std::string& enemy, int frame = 0);
sf::IntRect itemFrame(ItemType type, int frame = 0);
sf::IntRect tileFrame(char tile, WorldMode world = WorldMode::Normal, bool revealed = false);
sf::IntRect backgroundFrame(int level, WeatherType weather);
std::string backgroundTexture(int level);
sf::Color timeOfDayTint(float levelTime);
std::string timeOfDayName(float levelTime);
}

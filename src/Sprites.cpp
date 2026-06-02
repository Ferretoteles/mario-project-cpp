#include "Sprites.hpp"

#include <cstdlib>

namespace {
std::vector<sf::IntRect> frames(int x, int y, int w, int h, int count, int step = 18)
{
    std::vector<sf::IntRect> out;
    for (int i = 0; i < count; ++i)
        out.push_back({x + i * step, y, w, h});
    return out;
}
}

namespace Sprites {
std::vector<sf::IntRect> playerFrames(const std::string& character, bool big, const std::string& state)
{
    const bool luigi = character == "Luigi";
    const int baseY = big ? (luigi ? 128 : 64) : (luigi ? 32 : 0);
    const int h = big ? 64 : 32;
    if (!big) {
        if (state == "run")
            return {{64, baseY, 32, h}, {96, baseY, 32, h}, {128, baseY, 32, h}};
        if (state == "jump")
            return {{160, baseY, 32, h}};
        if (state == "dead")
            return {{192, baseY, 32, h}};
        return {{0, baseY, 32, h}, {32, baseY, 32, h}};
    }

    if (state == "run")
        return {{64, baseY, 32, h}, {96, baseY, 32, h}, {128, baseY, 32, h}};
    if (state == "jump")
        return {{160, baseY, 32, h}};
    if (state == "dead")
        return {{192, baseY, 32, h}};
    return {{0, baseY, 32, h}, {32, baseY, 32, h}};
}

sf::IntRect enemyFrame(const std::string& enemy, int frame)
{
    frame = std::abs(frame);
    if (enemy == "goomba")
        return {(frame % 2) * 32, 0, 32, 32};
    if (enemy == "koopa")
        return {64 + (frame % 2) * 32, 0, 32, 48};
    if (enemy == "koopa_shell")
        return {128, 0, 32, 32};
    if (enemy == "piranha")
        return {160 + (frame % 2) * 32, 0, 32, 48};
    if (enemy == "bullet")
        return {224, 0, 32, 32};
    if (enemy == "flyer")
        return {(frame % 2) * 32, 0, 32, 32};
    if (enemy == "shooter")
        return {160, 0, 32, 48};
    if (enemy == "shadow")
        return {(frame % 2) * 32, 0, 32, 32};
    if (enemy == "bowser")
        return {0, 48, 64, 64};
    return {0, 0, 32, 32};
}

sf::IntRect itemFrame(ItemType type, int frame)
{
    frame = std::abs(frame);
    switch (type) {
    case ItemType::Coin:
        return {128 + (frame % 4) * 8, 49, 8, 16};
    case ItemType::Heart:
        return {176, 49, 16, 16};
    case ItemType::Mushroom:
        return {0, 16, 16, 16};
    case ItemType::Star:
        return {64 + (frame % 2) * 16, 16, 16, 16};
    case ItemType::FireFlower:
        return {48 + (frame % 2) * 16, 16, 16, 16};
    case ItemType::Key:
        return {96, 50, 16, 16};
    case ItemType::Chest:
        return {208, 82, 16, 16};
    default:
        return {0, 16, 16, 16};
    }
}

sf::IntRect tileFrame(char tile, WorldMode world, bool revealed)
{
    if (tile == '?')
        return {65, 104, 16, 16};
    if (tile == 'U')
        return {115, 104, 16, 16};
    if (tile == 'B')
        return {264, 105, 16, 16};
    if (tile == 'G')
        return world == WorldMode::Ghost ? sf::IntRect{164, 214, 16, 16} : sf::IntRect{2, 20, 16, 16};
    if (tile == 'D')
        return world == WorldMode::Ghost ? sf::IntRect{180, 214, 16, 16} : sf::IntRect{18, 20, 16, 16};
    if (tile == 'P' || tile == 'p')
        return world == WorldMode::Ghost ? sf::IntRect{166, 263, 16, 16} : sf::IntRect{6, 263, 16, 16};
    if (tile == '^')
        return {398, 662, 16, 16};
    if (tile == '~')
        return {619, 694, 16, 16};
    if (tile == 'T')
        return {356, 687, 16, 16};
    if (tile == 'S')
        return revealed ? sf::IntRect{66, 104, 16, 16} : sf::IntRect{2, 20, 16, 16};
    if (tile == 'C')
        return {644, 444, 16, 16};
    if (tile == 'F' || tile == 'f')
        return {414, 644, 16, 16};
    return {2, 20, 16, 16};
}

sf::IntRect backgroundFrame(int level, WeatherType weather)
{
    int y = 0;
    if (weather == WeatherType::Snow)
        y = 1244;
    else if (level == 2 || weather == WeatherType::Rain)
        y = 249;
    else if (level == 3)
        y = 498;
    else if (level == 4 || weather == WeatherType::Storm)
        y = 747;
    else if (level == 5)
        y = 996;
    return {0, y, 768, 224};
}

std::string backgroundTexture(int level)
{
    if (level % 3 == 2)
        return "background_trees";
    if (level % 3 == 0)
        return "background_clouds";
    return "background_mountains";
}

sf::Color timeOfDayTint(float levelTime)
{
    const int phase = static_cast<int>(levelTime / 28.0f) % 5;
    switch (phase) {
    case 0:
        return sf::Color(255, 232, 200, 235); // poranek
    case 1:
        return sf::Color(255, 255, 255, 255); // dzien
    case 2:
        return sf::Color(255, 170, 120, 230); // zachod
    case 3:
        return sf::Color(120, 150, 230, 210); // noc
    default:
        return sf::Color(75, 80, 150, 195); // polnoc
    }
}

std::string timeOfDayName(float levelTime)
{
    const int phase = static_cast<int>(levelTime / 28.0f) % 5;
    switch (phase) {
    case 0:
        return "PORANEK";
    case 1:
        return "DZIEN";
    case 2:
        return "ZACHOD";
    case 3:
        return "NOC";
    default:
        return "POLNOC";
    }
}
}

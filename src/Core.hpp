#pragma once

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

constexpr float Tile = 32.0f;
constexpr unsigned WindowWidth = 960;
constexpr unsigned WindowHeight = 540;

enum class AppState {
    Loading,
    Title,
    LevelSelect,
    Settings,
    Playing,
    Paused,
    Shop,
    Newspaper,
    GameOver,
    Victory,
    Editor
};

enum class WorldMode {
    Normal,
    Ghost
};

enum class EventType {
    CoinCollected,
    EnemyKilled,
    LevelCompleted,
    KeyFound,
    ChestOpened,
    BossDefeated,
    BossLanded
};

enum class ItemType {
    Coin,
    Heart,
    Mushroom,
    Star,
    FireFlower,
    Key,
    Chest
};

enum class Rarity {
    Common,
    Rare,
    Epic,
    Legendary
};

struct GameEvent {
    EventType type = EventType::CoinCollected;
    int value = 0;
    std::string tag;
};

struct UpgradeState {
    int jumpLevel = 0;
    int speedLevel = 0;
    int healthLevel = 0;
    int magnetLevel = 0;
    std::string activeSkin = "Mario";
};

struct RunStats {
    int coins = 0;
    int kills = 0;
    int jumps = 0;
    int deaths = 0;
    int xp = 0;
    int chests = 0;
    float time = 0.0f;
};

struct SaveData {
    int saveSlot = 1;
    int unlockedLevel = 1;
    int bankCoins = 0;
    int bestScore = 0;
    float volume = 60.0f;
    std::map<int, float> bestTimes;
    std::map<int, int> bestCoins;
    std::set<int> completedLevels;
    std::set<std::string> achievements;
    UpgradeState upgrades;
};

inline float rectRight(const sf::FloatRect& rect)
{
    return rect.left + rect.width;
}

inline float rectBottom(const sf::FloatRect& rect)
{
    return rect.top + rect.height;
}

inline sf::Vector2f rectCenter(const sf::FloatRect& rect)
{
    return {rect.left + rect.width * 0.5f, rect.top + rect.height * 0.5f};
}

inline sf::FloatRect growRect(sf::FloatRect rect, float amount)
{
    rect.left -= amount;
    rect.top -= amount;
    rect.width += amount * 2.0f;
    rect.height += amount * 2.0f;
    return rect;
}

inline float approach(float value, float target, float amount)
{
    if (value < target)
        return std::min(value + amount, target);
    if (value > target)
        return std::max(value - amount, target);
    return value;
}

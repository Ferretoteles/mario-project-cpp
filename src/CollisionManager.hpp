#pragma once

#include "Core.hpp"

class Level;

struct CollisionResult {
    bool onGround = false;
    bool hitWall = false;
    bool hitHead = false;
    bool hitHazard = false;
    bool hitTrampoline = false;
    int headRow = -1;
    int headCol = -1;
    int platformIndex = -1;
};

class CollisionManager {
public:
    static CollisionResult move(sf::FloatRect& rect,
                                sf::Vector2f& velocity,
                                Level& level,
                                WorldMode world,
                                bool secretsRevealed,
                                float dt);

    static bool overlaps(const sf::FloatRect& a, const sf::FloatRect& b);
    static bool tileHazardAt(const Level& level, sf::Vector2f point);
};

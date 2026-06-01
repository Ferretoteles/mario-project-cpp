#pragma once

#include "AssetManager.hpp"
#include "Core.hpp"

class Level;

class Projectile {
public:
    Projectile(sf::Vector2f pos, sf::Vector2f velocity, bool fromPlayer, int damage, sf::Color color);

    void update(Level& level, float dt);
    void draw(sf::RenderWindow& window, const AssetManager& assets, float time) const;

    sf::FloatRect rect() const;
    bool fromPlayer() const;
    int damage() const;
    bool alive() const;
    void destroy();

private:
    sf::FloatRect m_rect;
    sf::Vector2f m_velocity;
    bool m_fromPlayer = false;
    int m_damage = 1;
    float m_life = 5.0f;
    bool m_alive = true;
    sf::Color m_color;
};

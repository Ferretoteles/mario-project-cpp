#pragma once

#include "AssetManager.hpp"
#include "Core.hpp"
#include "EventSystem.hpp"
#include "Level.hpp"

class Player;

class Npc {
public:
    Npc(NpcRole role, sf::Vector2f pos, std::string name);

    void update(Level& level, Player& player, float dt);
    void draw(sf::RenderWindow& window, const AssetManager& assets, float time) const;
    void interact(Player& player, EventSystem& events);

    sf::FloatRect rect() const;
    bool rescued() const;
    NpcRole role() const;
    const std::string& name() const;
    const std::string& dialogue() const;

private:
    bool edgeAhead(Level& level) const;

    NpcRole m_role;
    sf::FloatRect m_rect;
    sf::Vector2f m_velocity;
    std::string m_name;
    std::string m_dialogue;
    int m_direction = 1;
    bool m_rescued = false;
    float m_timer = 0.0f;
};

std::unique_ptr<Npc> makeNpc(const SpawnRequest& spawn);

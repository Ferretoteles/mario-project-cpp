#pragma once

#include "AssetManager.hpp"
#include "Core.hpp"

class Menu {
public:
    void set(const std::string& title, std::vector<std::string> items);
    void up();
    void down();
    void activateSelection();
    int index() const;
    void draw(sf::RenderWindow& window, const AssetManager& assets, sf::Vector2f size, const std::string& subtitle = "", float time = 0.0f) const;

private:
    std::string m_title;
    std::vector<std::string> m_items;
    int m_selected = 0;
    sf::Clock m_effectClock;
    float m_selectionChangedAt = -10.0f;
    float m_activatedAt = -10.0f;
};

#pragma once

#include "AssetManager.hpp"
#include "Core.hpp"
#include "EventSystem.hpp"
#include "Level.hpp"
#include "SpriteComponent.hpp"

class Player;

class Item {
public:
    Item(ItemType type, sf::FloatRect rect);
    virtual ~Item() = default;

    virtual void update(Player& player, float dt);
    virtual void draw(sf::RenderWindow& window, const AssetManager& assets, float time) const;
    virtual void collect(Player& player, EventSystem& events);

    sf::FloatRect rect() const;
    bool alive() const;
    ItemType type() const;

protected:
    ItemType m_type;
    sf::FloatRect m_rect;
    sf::Vector2f m_velocity;
    bool m_alive = true;
    float m_phase = 0.0f;
    SpriteComponent m_sprite;
};

class Coin final : public Item {
public:
    explicit Coin(sf::Vector2f pos);
    void draw(sf::RenderWindow& window, const AssetManager& assets, float time) const override;
    void collect(Player& player, EventSystem& events) override;
};

class HeartItem final : public Item {
public:
    explicit HeartItem(sf::Vector2f pos);
    void draw(sf::RenderWindow& window, const AssetManager& assets, float time) const override;
    void collect(Player& player, EventSystem& events) override;
};

class Mushroom final : public Item {
public:
    explicit Mushroom(sf::Vector2f pos);
    void draw(sf::RenderWindow& window, const AssetManager& assets, float time) const override;
    void collect(Player& player, EventSystem& events) override;
};

class StarItem final : public Item {
public:
    explicit StarItem(sf::Vector2f pos);
    void draw(sf::RenderWindow& window, const AssetManager& assets, float time) const override;
    void collect(Player& player, EventSystem& events) override;
};

class FireFlowerItem final : public Item {
public:
    explicit FireFlowerItem(sf::Vector2f pos);
    void draw(sf::RenderWindow& window, const AssetManager& assets, float time) const override;
    void collect(Player& player, EventSystem& events) override;
};

class KeyItem final : public Item {
public:
    explicit KeyItem(sf::Vector2f pos);
    void draw(sf::RenderWindow& window, const AssetManager& assets, float time) const override;
    void collect(Player& player, EventSystem& events) override;
};

class ChestItem final : public Item {
public:
    ChestItem(sf::Vector2f pos, Rarity rarity);
    void draw(sf::RenderWindow& window, const AssetManager& assets, float time) const override;
    void collect(Player& player, EventSystem& events) override;

private:
    Rarity m_rarity = Rarity::Common;
};

std::unique_ptr<Item> makeItem(const SpawnRequest& spawn);

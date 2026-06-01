#pragma once

#include "AssetManager.hpp"
#include "Core.hpp"
#include "EventSystem.hpp"
#include "Level.hpp"
#include "Projectile.hpp"
#include "SpriteComponent.hpp"

class Player;

class Enemy {
public:
    Enemy(sf::FloatRect rect, int hp, int direction = -1);
    virtual ~Enemy() = default;

    virtual void update(Level& level, Player& player, std::vector<Projectile>& projectiles, EventSystem& events, WorldMode world, float dt);
    virtual void draw(sf::RenderWindow& window, const AssetManager& assets, float time) const;
    virtual bool stomp(Player& player, EventSystem& events);
    virtual void damage(int value, EventSystem& events);
    virtual std::string name() const;
    virtual bool isBoss() const;
    virtual float healthPercent() const;
    virtual int phase() const;

    sf::FloatRect rect() const;
    bool alive() const;
    virtual bool readyToRemove() const;
    bool harmful() const;

protected:
    void walk(Level& level, WorldMode world, bool secretsRevealed, float speed, float dt);
    bool ledgeAhead(Level& level) const;

    sf::FloatRect m_rect;
    sf::Vector2f m_velocity;
    int m_hp = 1;
    int m_maxHp = 1;
    int m_direction = -1;
    bool m_alive = true;
    bool m_onGround = false;
    bool m_harmful = true;
    float m_deadTimer = 0.0f;
    float m_aiTimer = 0.0f;
    SpriteComponent m_sprite;
};

class Goomba final : public Enemy {
public:
    explicit Goomba(sf::Vector2f pos);
    std::string name() const override;
    void draw(sf::RenderWindow& window, const AssetManager& assets, float time) const override;
};

class Koopa final : public Enemy {
public:
    explicit Koopa(sf::Vector2f pos);
    void update(Level& level, Player& player, std::vector<Projectile>& projectiles, EventSystem& events, WorldMode world, float dt) override;
    bool stomp(Player& player, EventSystem& events) override;
    std::string name() const override;
    void draw(sf::RenderWindow& window, const AssetManager& assets, float time) const override;

private:
    bool m_shell = false;
};

class Flyer final : public Enemy {
public:
    explicit Flyer(sf::Vector2f pos);
    void update(Level& level, Player& player, std::vector<Projectile>& projectiles, EventSystem& events, WorldMode world, float dt) override;
    std::string name() const override;
    void draw(sf::RenderWindow& window, const AssetManager& assets, float time) const override;

private:
    float m_homeY = 0.0f;
};

class Shooter final : public Enemy {
public:
    explicit Shooter(sf::Vector2f pos);
    void update(Level& level, Player& player, std::vector<Projectile>& projectiles, EventSystem& events, WorldMode world, float dt) override;
    std::string name() const override;
    void draw(sf::RenderWindow& window, const AssetManager& assets, float time) const override;
};

class ShadowMonster final : public Enemy {
public:
    explicit ShadowMonster(sf::Vector2f pos);
    void update(Level& level, Player& player, std::vector<Projectile>& projectiles, EventSystem& events, WorldMode world, float dt) override;
    std::string name() const override;
    void draw(sf::RenderWindow& window, const AssetManager& assets, float time) const override;
};

class Runner final : public Enemy {
public:
    explicit Runner(sf::Vector2f pos);
    void update(Level& level, Player& player, std::vector<Projectile>& projectiles, EventSystem& events, WorldMode world, float dt) override;
    std::string name() const override;
    void draw(sf::RenderWindow& window, const AssetManager& assets, float time) const override;
};

class Spiny final : public Enemy {
public:
    explicit Spiny(sf::Vector2f pos);
    void update(Level& level, Player& player, std::vector<Projectile>& projectiles, EventSystem& events, WorldMode world, float dt) override;
    bool stomp(Player& player, EventSystem& events) override;
    std::string name() const override;
    void draw(sf::RenderWindow& window, const AssetManager& assets, float time) const override;
};

enum class BossState {
    Sleeping,
    Awakening,
    ActiveFight,
    Dead
};

class Boss final : public Enemy {
public:
    Boss(sf::Vector2f pos, int variant);
    void update(Level& level, Player& player, std::vector<Projectile>& projectiles, EventSystem& events, WorldMode world, float dt) override;
    bool stomp(Player& player, EventSystem& events) override;
    void damage(int value, EventSystem& events) override;
    std::string name() const override;
    bool isBoss() const override;
    float healthPercent() const override;
    void draw(sf::RenderWindow& window, const AssetManager& assets, float time) const override;
    int phase() const override;
    bool readyToRemove() const override;
    void resetForDungeon(sf::Vector2f pos, BossState state);
    void setState(BossState state);
    BossState state() const;
    bool fireActive() const;
    sf::FloatRect fireRect() const;

private:
    int m_variant = 0;
    float m_shootTimer = 0.0f;
    float m_jumpTimer = 0.0f;
    float m_hurtTimer = 0.0f;
    float m_attackAnimTimer = 0.0f;
    float m_awakeTimer = 0.0f;
    float m_fireChargeTimer = 0.0f;
    float m_fireTimer = 0.0f;
    float m_fireCooldown = 1.7f;
    BossState m_state = BossState::Sleeping;
};

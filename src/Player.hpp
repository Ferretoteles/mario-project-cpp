#pragma once

#include "AssetManager.hpp"
#include "Core.hpp"
#include "EventSystem.hpp"
#include "Level.hpp"
#include "SpriteComponent.hpp"

class Player {
public:
    void reset(sf::Vector2f start, const SaveData& save);
    void respawn(sf::Vector2f start);
    void update(Level& level,
                const std::array<bool, sf::Keyboard::KeyCount>& keys,
                EventSystem& events,
                WorldMode world,
                bool secretsRevealed,
                std::vector<SpawnRequest>& blockSpawns,
                float dt);
    void draw(sf::RenderWindow& window, const AssetManager& assets, float time) const;

    void hurt(EventSystem& events);
    void kill(EventSystem& events);
    void bounce(float strength = 430.0f);
    void knockbackFrom(sf::Vector2f source, float horizontal = 320.0f, float upward = 310.0f);
    void setPosition(sf::Vector2f topLeft);
    void heal(int value);
    void addCoin(EventSystem& events, int value = 1);
    void addXp(int value);
    void makeBig();
    void giveStar(float seconds);
    void giveFireFlower(float seconds);
    void clearStarPower();
    void giveKey();

    sf::FloatRect rect() const;
    sf::Vector2f center() const;
    bool alive() const;
    bool deadAnimationFinished() const;
    bool hasKey() const;
    bool isInvincible() const;
    bool isFalling() const;
    bool canShootFire() const;
    float coinMagnetRadius() const;
    int lives() const;
    int coins() const;
    int xp() const;
    int facing() const;
    const RunStats& stats() const;
    RunStats& stats();

private:
    bool keyDown(const std::array<bool, sf::Keyboard::KeyCount>& keys, sf::Keyboard::Key key) const;
    int maxJumps() const;
    float speedBonus() const;
    float jumpBonus() const;
    float magnetRadius() const;
    void configureSprites();
    void updateSpriteAnimation();

    sf::FloatRect m_rect{0.0f, 0.0f, 34.0f, 42.0f};
    sf::Vector2f m_velocity{0.0f, 0.0f};
    int m_facing = 1;
    int m_lives = 3;
    int m_maxLives = 3;
    int m_coins = 0;
    int m_xp = 0;
    int m_jumpsUsed = 0;
    int m_groundPlatform = -1;
    bool m_onGround = false;
    bool m_big = false;
    bool m_hasKey = false;
    bool m_dead = false;
    float m_deathTimer = 0.0f;
    float m_invincible = 0.0f;
    float m_starTimer = 0.0f;
    float m_fireTimer = 0.0f;
    float m_coyote = 0.0f;
    float m_jumpBuffer = 0.0f;
    bool m_wasJumpHeld = false;
    float m_anim = 0.0f;
    UpgradeState m_upgrades;
    RunStats m_stats;
    SpriteComponent m_sprite;
    std::string m_spriteCharacter = "Mario";
};

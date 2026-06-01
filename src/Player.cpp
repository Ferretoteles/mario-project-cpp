#include "Player.hpp"

#include "AudioManager.hpp"
#include "CollisionManager.hpp"
#include "Sprites.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr float Gravity = 1850.0f;

void drawLanternFallback(sf::RenderWindow& window, sf::Vector2f center, float alpha)
{
    const sf::Uint8 a = static_cast<sf::Uint8>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f);
    sf::RectangleShape body({11.0f, 15.0f});
    body.setOrigin(5.5f, 7.5f);
    body.setPosition(center);
    body.setFillColor(sf::Color(255, 200, 72, a));
    body.setOutlineColor(sf::Color(70, 44, 24, a));
    body.setOutlineThickness(2.0f);
    window.draw(body);

    sf::RectangleShape cap({13.0f, 4.0f});
    cap.setOrigin(6.5f, 2.0f);
    cap.setPosition(center.x, center.y - 10.0f);
    cap.setFillColor(sf::Color(92, 64, 38, a));
    window.draw(cap);
}
}

void Player::reset(sf::Vector2f start, const SaveData& save)
{
    m_upgrades = save.upgrades;
    m_spriteCharacter = m_upgrades.activeSkin == "Luigi" ? "Luigi" : "Mario";
    configureSprites();
    m_rect = {start.x, start.y, 32.0f, 32.0f};
    m_velocity = {};
    m_facing = 1;
    m_lives = 3 + m_upgrades.healthLevel;
    m_maxLives = m_lives;
    m_coins = 0;
    m_xp = 0;
    m_jumpsUsed = 0;
    m_groundPlatform = -1;
    m_onGround = false;
    m_big = false;
    m_hasKey = false;
    m_dead = false;
    m_deathTimer = 0.0f;
    m_invincible = 1.0f;
    m_starTimer = 0.0f;
    m_fireTimer = 0.0f;
    m_coyote = 0.0f;
    m_jumpBuffer = 0.0f;
    m_wasJumpHeld = false;
    m_anim = 0.0f;
    m_lanternEnergy = 100.0f;
    m_lanternOn = true;
    m_lanternThrown = false;
    m_stats = {};
    updateSpriteAnimation();
}

void Player::respawn(sf::Vector2f start)
{
    const int lives = m_lives;
    const int coins = m_coins;
    const int xp = m_xp;
    const RunStats stats = m_stats;
    m_rect = {start.x, start.y, 32.0f, 32.0f};
    m_velocity = {};
    m_lives = lives;
    m_coins = coins;
    m_xp = xp;
    m_stats = stats;
    m_jumpsUsed = 0;
    m_groundPlatform = -1;
    m_onGround = false;
    m_big = false;
    m_dead = false;
    m_deathTimer = 0.0f;
    m_invincible = 1.6f;
    m_starTimer = 0.0f;
    m_fireTimer = 0.0f;
    m_coyote = 0.0f;
    m_jumpBuffer = 0.0f;
    m_wasJumpHeld = false;
    m_lanternOn = true;
    m_lanternThrown = false;
    configureSprites();
    updateSpriteAnimation();
}

void Player::update(Level& level,
                    const std::array<bool, sf::Keyboard::KeyCount>& keys,
                    EventSystem& events,
                    WorldMode world,
                    bool secretsRevealed,
                    std::vector<SpawnRequest>& blockSpawns,
                    float dt)
{
    m_stats.time += dt;
    m_anim += dt;

    if (m_dead) {
        m_deathTimer += dt;
        m_velocity.y += Gravity * dt;
        m_rect.top += m_velocity.y * dt;
        m_sprite.play("dead");
        m_sprite.update(dt);
        return;
    }

    if (m_groundPlatform >= 0 && m_groundPlatform < static_cast<int>(level.platforms().size())) {
        const auto& platform = level.platforms()[static_cast<std::size_t>(m_groundPlatform)];
        m_rect.left += platform.rect.left - platform.previous.x;
        m_rect.top += platform.rect.top - platform.previous.y;
    }

    const bool left = keyDown(keys, sf::Keyboard::A) || keyDown(keys, sf::Keyboard::Left);
    const bool right = keyDown(keys, sf::Keyboard::D) || keyDown(keys, sf::Keyboard::Right);
    const bool sprint = keyDown(keys, sf::Keyboard::LShift) || keyDown(keys, sf::Keyboard::RShift);
    const bool jumpHeld = keyDown(keys, sf::Keyboard::Space) || keyDown(keys, sf::Keyboard::W) || keyDown(keys, sf::Keyboard::Up);
    const int axis = (right ? 1 : 0) - (left ? 1 : 0);

    if (jumpHeld && !m_wasJumpHeld)
        m_jumpBuffer = 0.15f;
    m_wasJumpHeld = jumpHeld;

    const float maxSpeed = (sprint ? 360.0f : 250.0f) + speedBonus();
    const float acceleration = m_onGround ? 3050.0f : 1950.0f;
    const float friction = m_onGround ? 2700.0f : 660.0f;

    if (axis != 0) {
        m_velocity.x = std::clamp(m_velocity.x + axis * acceleration * dt, -maxSpeed, maxSpeed);
        m_facing = axis;
    } else {
        m_velocity.x = approach(m_velocity.x, 0.0f, friction * dt);
    }

    if (m_onGround) {
        m_coyote = 0.12f;
        m_jumpsUsed = 0;
    } else {
        m_coyote -= dt;
    }
    m_jumpBuffer -= dt;

    if (m_jumpBuffer > 0.0f && (m_coyote > 0.0f || m_jumpsUsed < maxJumps())) {
        const bool doubleJump = !(m_coyote > 0.0f);
        m_velocity.y = -(640.0f + jumpBonus() + (m_big ? 25.0f : 0.0f));
        m_onGround = false;
        m_groundPlatform = -1;
        m_coyote = 0.0f;
        m_jumpBuffer = 0.0f;
        m_jumpsUsed += 1;
        m_stats.jumps += 1;
        events.publish({EventType::PlayerJumped, m_stats.jumps, doubleJump ? "double" : "normal"});
        AudioManager::instance().play(doubleJump ? "double_jump" : "jump");
    }

    m_velocity.y = std::min(m_velocity.y + Gravity * dt, 960.0f);
    auto collision = CollisionManager::move(m_rect, m_velocity, level, world, secretsRevealed, dt);
    m_onGround = collision.onGround;
    m_groundPlatform = collision.platformIndex;
    if (collision.hitTrampoline) {
        m_velocity.y = -850.0f;
        m_onGround = false;
        m_jumpsUsed = 0;
        AudioManager::instance().play("double_jump");
    }
    if (collision.hitHead)
        level.hitBlock(collision.headRow, collision.headCol, blockSpawns);
    if (collision.hitHazard)
        hurt(events);

    if (m_rect.top > level.height() + 160.0f)
        kill(events);

    for (const auto& pipe : level.teleports()) {
        if (m_rect.intersects(pipe.from) && (keyDown(keys, sf::Keyboard::S) || keyDown(keys, sf::Keyboard::Down))) {
            m_rect.left = pipe.to.x;
            m_rect.top = pipe.to.y;
            AudioManager::instance().play("lantern");
        }
    }

    if (m_invincible > 0.0f)
        m_invincible -= dt;
    if (m_starTimer > 0.0f)
        m_starTimer -= dt;
    if (m_fireTimer > 0.0f)
        m_fireTimer -= dt;

    const float drain = m_lanternOn ? (world == WorldMode::Ghost ? 3.2f : 1.45f) : 0.18f;
    m_lanternEnergy = std::max(0.0f, m_lanternEnergy - drain * dt);
    if (m_lanternThrown) {
        m_lanternVel.y += 500.0f * dt;
        m_lanternPos += m_lanternVel * dt;
        if (m_lanternPos.y > rectBottom(m_rect) + 100.0f)
            m_lanternVel.y = -std::abs(m_lanternVel.y) * 0.45f;
        m_lanternEnergy = std::max(0.0f, m_lanternEnergy - 4.0f * dt);
    } else {
        m_lanternPos = center() + sf::Vector2f(m_facing * 19.0f, 3.0f);
    }

    updateSpriteAnimation();
    m_sprite.update(dt);
}

void Player::draw(sf::RenderWindow& window, const AssetManager& assets, float time) const
{
    sf::Color tint = sf::Color::White;
    if (m_starTimer > 0.0f)
        tint = sf::Color(255, static_cast<sf::Uint8>(210 + std::sin(time * 12.0f) * 40.0f), 95);
    else if (m_fireTimer > 0.0f)
        tint = sf::Color(255, 214, 158);
    if (m_invincible > 0.0f && std::fmod(time * 18.0f, 2.0f) < 1.0f)
        tint.a = 125;
    m_sprite.draw(window, assets, m_rect, m_facing < 0, tint);
}

void Player::hurt(EventSystem& events)
{
    if (isInvincible())
        return;
    if (m_big) {
        const float bottom = rectBottom(m_rect);
        m_big = false;
        m_rect.width = 32.0f;
        m_rect.height = 32.0f;
        m_rect.top = bottom - m_rect.height;
        m_invincible = 1.6f;
        AudioManager::instance().play("hurt");
        return;
    }
    kill(events);
}

void Player::kill(EventSystem& events)
{
    if (m_dead)
        return;
    m_lives = std::max(0, m_lives - 1);
    m_stats.deaths += 1;
    m_dead = true;
    m_deathTimer = 0.0f;
    m_velocity = {0.0f, -520.0f};
    events.publish({EventType::PlayerDied, m_stats.deaths, "death"});
    AudioManager::instance().play("death");
}

void Player::bounce(float strength)
{
    m_velocity.y = -strength;
    m_onGround = false;
    m_jumpsUsed = 0;
}

void Player::knockbackFrom(sf::Vector2f source, float horizontal, float upward)
{
    if (m_dead)
        return;
    const float direction = center().x < source.x ? -1.0f : 1.0f;
    m_velocity.x = direction * horizontal;
    m_velocity.y = -upward;
    m_onGround = false;
    m_groundPlatform = -1;
}

void Player::setPosition(sf::Vector2f topLeft)
{
    m_rect.left = topLeft.x;
    m_rect.top = topLeft.y;
    m_velocity = {};
    m_onGround = false;
    m_groundPlatform = -1;
    updateSpriteAnimation();
}

void Player::heal(int value)
{
    m_lives = std::min(m_lives + value, m_maxLives);
}

void Player::addCoin(EventSystem& events, int value)
{
    m_coins += value;
    m_stats.coins += value;
    events.publish({EventType::CoinCollected, m_coins, "coin"});
    AudioManager::instance().play("coin");
}

void Player::addXp(int value)
{
    m_xp += value;
    m_stats.xp += value;
}

void Player::makeBig()
{
    if (m_big)
        return;
    const float bottom = rectBottom(m_rect);
    m_big = true;
    m_rect.width = 32.0f;
    m_rect.height = 64.0f;
    m_rect.top = bottom - m_rect.height;
    AudioManager::instance().play("power");
    configureSprites();
    updateSpriteAnimation();
}

void Player::giveStar(float seconds)
{
    m_starTimer = std::max(m_starTimer, seconds);
    m_invincible = std::max(m_invincible, 0.4f);
    AudioManager::instance().play("power");
}

void Player::giveFireFlower(float seconds)
{
    m_fireTimer = std::max(m_fireTimer, seconds);
    AudioManager::instance().play("power");
}

void Player::giveKey()
{
    m_hasKey = true;
}

void Player::addFuel(float value, EventSystem& events)
{
    m_lanternEnergy = std::min(100.0f, m_lanternEnergy + value);
    m_lanternOn = true;
    events.publish({EventType::LanternFuelFound, static_cast<int>(m_lanternEnergy), "fuel"});
    AudioManager::instance().play("fuel");
}

void Player::toggleLantern(EventSystem& events)
{
    if (m_lanternEnergy <= 0.0f)
        return;
    m_lanternOn = !m_lanternOn;
    if (!m_lanternOn)
        m_lanternThrown = false;
    events.publish({EventType::LanternFuelFound, static_cast<int>(m_lanternEnergy), m_lanternOn ? "on" : "off"});
    AudioManager::instance().play("lantern");
}

void Player::throwLantern(EventSystem& events)
{
    if (m_lanternEnergy <= 8.0f)
        return;
    m_lanternOn = true;
    m_lanternThrown = !m_lanternThrown;
    if (m_lanternThrown) {
        m_lanternPos = center() + sf::Vector2f(m_facing * 25.0f, -8.0f);
        m_lanternVel = {m_facing * 470.0f, -210.0f};
    }
    events.publish({EventType::LanternFuelFound, static_cast<int>(m_lanternEnergy), "throw"});
    AudioManager::instance().play("lantern");
}

void Player::recallLantern()
{
    m_lanternThrown = false;
}

sf::FloatRect Player::rect() const { return m_rect; }
sf::Vector2f Player::center() const { return rectCenter(m_rect); }
bool Player::alive() const { return m_lives > 0; }
bool Player::deadAnimationFinished() const { return m_dead && m_deathTimer > 1.15f; }
bool Player::hasKey() const { return m_hasKey; }
bool Player::isInvincible() const { return m_invincible > 0.0f || m_starTimer > 0.0f; }
bool Player::isFalling() const { return m_velocity.y > 80.0f; }
bool Player::lanternLightActive() const { return m_lanternEnergy > 0.0f && (m_lanternOn || m_lanternThrown); }
bool Player::lanternIsOn() const { return m_lanternOn; }
bool Player::lanternIsThrown() const { return m_lanternThrown; }
bool Player::canRevealSecrets() const { return m_lanternEnergy > 5.0f && m_lanternThrown; }
bool Player::canShootFire() const { return m_fireTimer > 0.0f && !m_dead; }
sf::Vector2f Player::lanternPosition() const { return m_lanternPos; }
float Player::lanternEnergy() const { return m_lanternEnergy; }
float Player::coinMagnetRadius() const { return magnetRadius(); }
int Player::lives() const { return m_lives; }
int Player::maxLives() const { return m_maxLives; }
int Player::coins() const { return m_coins; }
int Player::xp() const { return m_xp; }
int Player::facing() const { return m_facing; }
float Player::firePowerTimer() const { return std::max(0.0f, m_fireTimer); }
std::string Player::activePowerUp() const
{
    if (m_starTimer > 0.0f)
        return "Starman";
    if (m_fireTimer > 0.0f)
        return "Fire Flower";
    if (m_big)
        return "Super Mushroom";
    return "Lantern";
}
const RunStats& Player::stats() const { return m_stats; }
RunStats& Player::stats() { return m_stats; }

bool Player::keyDown(const std::array<bool, sf::Keyboard::KeyCount>& keys, sf::Keyboard::Key key) const
{
    const int code = static_cast<int>(key);
    return code >= 0 && code < static_cast<int>(keys.size()) && keys[static_cast<std::size_t>(code)];
}

int Player::maxJumps() const
{
    return 2 + (m_upgrades.jumpLevel >= 3 ? 1 : 0);
}

float Player::speedBonus() const
{
    return m_upgrades.speedLevel * 28.0f;
}

float Player::jumpBonus() const
{
    return m_upgrades.jumpLevel * 34.0f;
}

float Player::magnetRadius() const
{
    return 80.0f + m_upgrades.magnetLevel * 55.0f;
}

void Player::configureSprites()
{
    m_sprite = SpriteComponent();
    m_sprite.addAnimation("idle", {"mario_player", Sprites::playerFrames(m_spriteCharacter, m_big, "idle"), 3.0f, true});
    m_sprite.addAnimation("run", {"mario_player", Sprites::playerFrames(m_spriteCharacter, m_big, "run"), 11.0f, true});
    m_sprite.addAnimation("jump", {"mario_player", Sprites::playerFrames(m_spriteCharacter, m_big, "jump"), 1.0f, false});
    m_sprite.addAnimation("dead", {"mario_player", Sprites::playerFrames(m_spriteCharacter, false, "dead"), 1.0f, false});
    m_sprite.play("idle");
}

void Player::updateSpriteAnimation()
{
    if (m_dead) {
        m_sprite.play("dead");
    } else if (!m_onGround) {
        m_sprite.play("jump");
    } else if (std::abs(m_velocity.x) > 35.0f) {
        m_sprite.play("run");
    } else {
        m_sprite.play("idle");
    }
}

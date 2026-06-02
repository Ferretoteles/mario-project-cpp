#include "Enemy.hpp"

#include "AudioManager.hpp"
#include "CollisionManager.hpp"
#include "Player.hpp"
#include "Sprites.hpp"

#include <algorithm>
#include <cmath>

Enemy::Enemy(sf::FloatRect rect, int hp, int direction)
    : m_rect(rect)
    , m_hp(hp)
    , m_maxHp(hp)
    , m_direction(direction)
{
}

void Enemy::update(Level& level, Player&, std::vector<Projectile>&, EventSystem&, WorldMode world, float dt)
{
    if (!m_alive) {
        m_deadTimer += dt;
        return;
    }
    walk(level, world, false, 78.0f, dt);
    m_sprite.update(dt);
}

void Enemy::draw(sf::RenderWindow& window, const AssetManager& assets, float) const
{
    if (m_sprite.current().empty())
        return;
    const sf::FloatRect target = m_alive ? m_rect : sf::FloatRect(m_rect.left, rectBottom(m_rect) - 12.0f, m_rect.width, 12.0f);
    m_sprite.draw(window, assets, target, m_direction > 0);
}

bool Enemy::stomp(Player& player, EventSystem& events)
{
    damage(1, events);
    player.bounce();
    return !m_alive;
}

void Enemy::damage(int value, EventSystem& events)
{
    if (!m_alive)
        return;
    m_hp -= value;
    if (m_hp <= 0) {
        m_alive = false;
        m_harmful = false;
        m_deadTimer = 0.0f;
        events.publish({EventType::EnemyKilled, 1, name()});
        AudioManager::instance().play("stomp");
    } else {
        AudioManager::instance().play("hurt");
    }
}

std::string Enemy::name() const { return "Enemy"; }
bool Enemy::isBoss() const { return false; }
float Enemy::healthPercent() const { return m_maxHp > 0 ? static_cast<float>(m_hp) / m_maxHp : 0.0f; }
int Enemy::phase() const { return 0; }
sf::FloatRect Enemy::rect() const { return m_rect; }
bool Enemy::alive() const { return m_alive; }
bool Enemy::readyToRemove() const { return !m_alive && m_deadTimer > 0.35f; }
bool Enemy::harmful() const { return m_harmful && m_alive; }

void Enemy::walk(Level& level, WorldMode world, bool secretsRevealed, float speed, float dt)
{
    m_aiTimer += dt;
    m_velocity.x = m_direction * speed;
    m_velocity.y = std::min(m_velocity.y + 1850.0f * dt, 920.0f);
    const auto collision = CollisionManager::move(m_rect, m_velocity, level, world, secretsRevealed, dt);
    m_onGround = collision.onGround;
    if (collision.hitWall || (m_onGround && ledgeAhead(level)))
        m_direction *= -1;
}

bool Enemy::ledgeAhead(Level& level) const
{
    const sf::Vector2f probe(rectCenter(m_rect).x + m_direction * (m_rect.width * 0.5f + 9.0f), rectBottom(m_rect) + 8.0f);
    const int row = static_cast<int>(std::floor(probe.y / Tile));
    const int col = static_cast<int>(std::floor(probe.x / Tile));
    return !level.isSolidTile(level.tileAt(row, col), WorldMode::Normal, true);
}

Goomba::Goomba(sf::Vector2f pos)
    : Enemy({pos.x, pos.y - 28.0f, 32.0f, 28.0f}, 1)
{
    m_sprite.addAnimation("walk", {"mario_enemies", {Sprites::enemyFrame("goomba", 0), Sprites::enemyFrame("goomba", 1)}, 5.0f, true});
    m_sprite.addAnimation("dead", {"mario_enemies", {Sprites::enemyFrame("goomba", 0)}, 1.0f, false});
    m_sprite.play("walk");
}

std::string Goomba::name() const { return "Goomba"; }

void Goomba::draw(sf::RenderWindow& window, const AssetManager& assets, float) const
{
    Enemy::draw(window, assets, 0.0f);
}

Koopa::Koopa(sf::Vector2f pos)
    : Enemy({pos.x, pos.y - 44.0f, 32.0f, 44.0f}, 2)
{
    m_sprite.addAnimation("walk", {"mario_enemies", {Sprites::enemyFrame("koopa", 0), Sprites::enemyFrame("koopa", 1)}, 5.0f, true});
    m_sprite.addAnimation("shell", {"mario_enemies", {Sprites::enemyFrame("koopa_shell", 0), Sprites::enemyFrame("koopa_shell", 0)}, 9.0f, true});
    m_sprite.play("walk");
}

void Koopa::update(Level& level, Player& player, std::vector<Projectile>& projectiles, EventSystem& events, WorldMode world, float dt)
{
    if (!m_alive) {
        m_deadTimer += dt;
        return;
    }
    walk(level, world, false, m_shell ? 285.0f : 68.0f, dt);
    m_sprite.play(m_shell ? "shell" : "walk");
    m_sprite.update(dt);
    if (m_shell && player.rect().intersects(m_rect) && std::abs(player.center().y - rectCenter(m_rect).y) < 20.0f)
        player.hurt(events);
    (void)projectiles;
}

bool Koopa::stomp(Player& player, EventSystem& events)
{
    player.bounce();
    if (!m_shell) {
        m_shell = true;
        m_rect.top += 18.0f;
        m_rect.height = 28.0f;
        m_harmful = false;
        AudioManager::instance().play("stomp");
        return false;
    }
    damage(5, events);
    return true;
}

std::string Koopa::name() const { return "Koopa"; }

void Koopa::draw(sf::RenderWindow& window, const AssetManager& assets, float) const
{
    Enemy::draw(window, assets, 0.0f);
}

Flyer::Flyer(sf::Vector2f pos)
    : Enemy({pos.x, pos.y - 28.0f, 32.0f, 28.0f}, 1)
    , m_homeY(pos.y - 36.0f)
{
    m_sprite.addAnimation("fly", {"mario_enemies", {Sprites::enemyFrame("goomba", 0), Sprites::enemyFrame("goomba", 1)}, 7.0f, true});
    m_sprite.play("fly");
}

void Flyer::update(Level&, Player& player, std::vector<Projectile>&, EventSystem&, WorldMode, float dt)
{
    if (!m_alive) {
        m_deadTimer += dt;
        return;
    }
    m_aiTimer += dt;
    m_direction = player.center().x < rectCenter(m_rect).x ? -1 : 1;
    m_rect.left += m_direction * 80.0f * dt;
    m_rect.top = m_homeY + std::sin(m_aiTimer * 3.1f) * 38.0f;
    m_sprite.update(dt);
}

std::string Flyer::name() const { return "Flyer"; }

void Flyer::draw(sf::RenderWindow& window, const AssetManager& assets, float) const
{
    Enemy::draw(window, assets, 0.0f);
}

Shooter::Shooter(sf::Vector2f pos)
    : Enemy({pos.x, pos.y - 42.0f, 32.0f, 42.0f}, 2)
{
    m_sprite.addAnimation("shoot", {"mario_enemies", {Sprites::enemyFrame("piranha", 0), Sprites::enemyFrame("piranha", 1)}, 4.0f, true});
    m_sprite.play("shoot");
}

void Shooter::update(Level& level, Player& player, std::vector<Projectile>& projectiles, EventSystem&, WorldMode world, float dt)
{
    (void)level;
    (void)world;
    if (!m_alive) {
        m_deadTimer += dt;
        return;
    }
    m_sprite.update(dt);
    m_direction = player.center().x < rectCenter(m_rect).x ? -1 : 1;
    m_aiTimer += dt;
    if (m_aiTimer > 2.0f) {
        m_aiTimer = 0.0f;
        projectiles.emplace_back(rectCenter(m_rect), sf::Vector2f(m_direction * 260.0f, -20.0f), false, 1, sf::Color(255, 96, 52));
        AudioManager::instance().play("boss");
    }
}

std::string Shooter::name() const { return "Shooter"; }

void Shooter::draw(sf::RenderWindow& window, const AssetManager& assets, float) const
{
    Enemy::draw(window, assets, 0.0f);
}

ShadowMonster::ShadowMonster(sf::Vector2f pos)
    : Enemy({pos.x, pos.y - 28.0f, 32.0f, 28.0f}, 2)
{
    m_sprite.addAnimation("shadow", {"mario_enemies", {Sprites::enemyFrame("goomba", 0), Sprites::enemyFrame("goomba", 1)}, 6.0f, true});
    m_sprite.play("shadow");
}

void ShadowMonster::update(Level& level, Player& player, std::vector<Projectile>&, EventSystem&, WorldMode world, float dt)
{
    if (!m_alive) {
        m_deadTimer += dt;
        return;
    }
    const float chase = world == WorldMode::Ghost || player.lanternEnergy() < 35.0f ? 135.0f : 45.0f;
    m_direction = player.center().x < rectCenter(m_rect).x ? -1 : 1;
    walk(level, world, true, chase, dt);
    m_sprite.update(dt);
}

std::string ShadowMonster::name() const { return "Shadow Monster"; }

void ShadowMonster::draw(sf::RenderWindow& window, const AssetManager& assets, float) const
{
    sf::Color tint(190, 170, 255, 210);
    m_sprite.draw(window, assets, m_rect, m_direction > 0, tint);
}

Runner::Runner(sf::Vector2f pos)
    : Enemy({pos.x, pos.y - 28.0f, 32.0f, 28.0f}, 1)
{
    m_sprite.addAnimation("run", {"mario_enemies", {Sprites::enemyFrame("goomba", 0), Sprites::enemyFrame("goomba", 1)}, 9.0f, true});
    m_sprite.play("run");
}

void Runner::update(Level& level, Player&, std::vector<Projectile>&, EventSystem&, WorldMode world, float dt)
{
    if (!m_alive) {
        m_deadTimer += dt;
        return;
    }
    walk(level, world, false, 142.0f, dt);
    m_sprite.update(dt);
}

std::string Runner::name() const { return "Runner"; }

void Runner::draw(sf::RenderWindow& window, const AssetManager& assets, float) const
{
    m_sprite.draw(window, assets, m_alive ? m_rect : sf::FloatRect(m_rect.left, rectBottom(m_rect) - 12.0f, m_rect.width, 12.0f), m_direction > 0, sf::Color(255, 164, 112));
}

Spiny::Spiny(sf::Vector2f pos)
    : Enemy({pos.x, pos.y - 30.0f, 32.0f, 30.0f}, 2)
{
    m_sprite.addAnimation("walk", {"mario_enemies", {Sprites::enemyFrame("koopa_shell", 0), Sprites::enemyFrame("koopa_shell", 0)}, 5.0f, true});
    m_sprite.play("walk");
}

void Spiny::update(Level& level, Player&, std::vector<Projectile>&, EventSystem&, WorldMode world, float dt)
{
    if (!m_alive) {
        m_deadTimer += dt;
        return;
    }
    walk(level, world, false, 92.0f, dt);
    m_sprite.update(dt);
}

bool Spiny::stomp(Player& player, EventSystem& events)
{
    player.bounce(480.0f);
    damage(1, events);
    return !m_alive;
}

std::string Spiny::name() const { return "Spiny"; }

void Spiny::draw(sf::RenderWindow& window, const AssetManager& assets, float) const
{
    const sf::Color tint = m_alive ? sf::Color(255, 112, 112) : sf::Color(150, 90, 90);
    m_sprite.draw(window, assets, m_rect, m_direction > 0, tint);
    if (m_alive) {
        for (int i = 0; i < 3; ++i) {
            sf::ConvexShape spike(3);
            const float x = m_rect.left + 7.0f + i * 8.0f;
            spike.setPoint(0, {x, m_rect.top + 7.0f});
            spike.setPoint(1, {x + 4.0f, m_rect.top - 3.0f});
            spike.setPoint(2, {x + 8.0f, m_rect.top + 7.0f});
            spike.setFillColor(sf::Color(245, 238, 208));
            spike.setOutlineColor(sf::Color(74, 54, 48));
            spike.setOutlineThickness(1.0f);
            window.draw(spike);
        }
    }
}

PiranhaPlant::PiranhaPlant(sf::Vector2f mouth)
    : Enemy({mouth.x - 14.0f, mouth.y, 28.0f, 0.0f}, 2)
    , m_mouthY(mouth.y)
{
    m_sprite.addAnimation("bite", {"mario_enemies", {Sprites::enemyFrame("piranha", 0), Sprites::enemyFrame("piranha", 1)}, 5.0f, true});
    m_sprite.play("bite");
    m_harmful = false;
    // lekkie rozsuniecie cyklu wg pozycji, zeby rosliny nie wynurzaly sie rownoczesnie
    m_phaseTimer = std::fmod(std::abs(mouth.x) * 0.013f, 2.0f);
}

void PiranhaPlant::update(Level&, Player&, std::vector<Projectile>&, EventSystem&, WorldMode, float dt)
{
    if (!m_alive) {
        m_deadTimer += dt;
        return;
    }

    constexpr float HiddenTime = 2.0f;
    constexpr float RiseTime = 0.5f;
    constexpr float OutTime = 1.8f;
    constexpr float LowerTime = 0.5f;

    m_phaseTimer += dt;
    switch (m_phase) {
    case 0:
        m_emerge = 0.0f;
        if (m_phaseTimer >= HiddenTime) { m_phase = 1; m_phaseTimer = 0.0f; }
        break;
    case 1:
        m_emerge = std::min(1.0f, m_phaseTimer / RiseTime);
        if (m_phaseTimer >= RiseTime) { m_phase = 2; m_phaseTimer = 0.0f; m_emerge = 1.0f; }
        break;
    case 2:
        m_emerge = 1.0f;
        if (m_phaseTimer >= OutTime) { m_phase = 3; m_phaseTimer = 0.0f; }
        break;
    default:
        m_emerge = std::max(0.0f, 1.0f - m_phaseTimer / LowerTime);
        if (m_phaseTimer >= LowerTime) { m_phase = 0; m_phaseTimer = 0.0f; m_emerge = 0.0f; }
        break;
    }

    const float height = m_emerge * m_fullHeight;
    m_rect.height = height;
    m_rect.top = m_mouthY - height;
    m_harmful = m_emerge > 0.45f;
    m_sprite.update(dt);
}

bool PiranhaPlant::stomp(Player& player, EventSystem& events)
{
    // nie da sie zadeptac - skok na rosline konczy sie ugryzieniem gracza
    if (m_harmful)
        player.hurt(events);
    return false;
}

std::string PiranhaPlant::name() const { return "Piranha Plant"; }

void PiranhaPlant::draw(sf::RenderWindow& window, const AssetManager& assets, float time) const
{
    if (!m_alive || m_emerge <= 0.02f)
        return;

    const float height = m_emerge * m_fullHeight;
    sf::IntRect frame = Sprites::enemyFrame("piranha", static_cast<int>(time * 5.0f) % 2);
    frame.height = std::max(1, static_cast<int>(frame.height * m_emerge)); // odsloniety jest gorny fragment sprite'a
    const sf::FloatRect target(m_rect.left, m_mouthY - height, m_rect.width, height);
    m_sprite.drawStatic(window, assets, "mario_enemies", frame, target, false);
}

Boss::Boss(sf::Vector2f pos, int variant)
    : Enemy({pos.x, pos.y - 128.0f, 128.0f, 128.0f}, 12)
    , m_variant(variant)
{
    m_sprite.addAnimation("idle", {"bowser_boss", {{0, 0, 64, 64}, {64, 0, 64, 64}}, 3.0f, true});
    m_sprite.addAnimation("attack", {"bowser_boss", {{128, 0, 64, 64}, {320, 0, 64, 64}}, 6.0f, false});
    m_sprite.addAnimation("hurt", {"bowser_boss", {{192, 0, 64, 64}}, 1.0f, false});
    m_sprite.addAnimation("death", {"bowser_boss", {{256, 0, 64, 64}}, 1.0f, false});
    m_sprite.play("idle");
    m_harmful = false;
}

void Boss::update(Level& level, Player& player, std::vector<Projectile>& projectiles, EventSystem& events, WorldMode world, float dt)
{
    (void)projectiles;
    if (!m_alive) {
        m_state = BossState::Dead;
        m_deadTimer += dt;
        m_sprite.play("death");
        m_sprite.update(dt);
        return;
    }

    m_aiTimer += dt;
    if (m_hurtTimer > 0.0f)
        m_hurtTimer = std::max(0.0f, m_hurtTimer - dt);
    if (m_attackAnimTimer > 0.0f)
        m_attackAnimTimer = std::max(0.0f, m_attackAnimTimer - dt);
    m_direction = player.center().x < rectCenter(m_rect).x ? -1 : 1;

    if (m_state == BossState::Sleeping) {
        m_harmful = false;
        m_velocity.x = 0.0f;
        m_velocity.y = std::min(m_velocity.y + 1850.0f * dt, 920.0f);
        const auto collision = CollisionManager::move(m_rect, m_velocity, level, world, true, dt);
        m_onGround = collision.onGround;
        m_sprite.play("idle");
        m_sprite.update(dt * 0.45f);
        return;
    }

    if (m_state == BossState::Awakening) {
        m_harmful = false;
        m_awakeTimer += dt;
        m_velocity.x = 0.0f;
        m_velocity.y = std::min(m_velocity.y + 1850.0f * dt, 920.0f);
        const auto collision = CollisionManager::move(m_rect, m_velocity, level, world, true, dt);
        m_onGround = collision.onGround;
        m_sprite.play(std::fmod(m_awakeTimer * 9.0f, 2.0f) > 1.0f ? "hurt" : "idle");
        m_sprite.update(dt);
        if (m_awakeTimer >= 1.35f) {
            setState(BossState::ActiveFight);
            events.publish({EventType::BossLanded, phase(), "awake"});
            AudioManager::instance().play("boss");
        }
        return;
    }

    m_harmful = true;
    const bool wasOnGround = m_onGround;
    m_jumpTimer += dt;
    if (m_fireChargeTimer > 0.0f) {
        m_fireChargeTimer = std::max(0.0f, m_fireChargeTimer - dt);
        m_attackAnimTimer = 0.25f;
        m_velocity.x = 0.0f;
        if (m_fireChargeTimer == 0.0f) {
            m_fireTimer = 0.95f;
            m_fireCooldown = 2.25f;
            AudioManager::instance().play("boss");
        }
    } else if (m_fireTimer > 0.0f) {
        m_fireTimer = std::max(0.0f, m_fireTimer - dt);
        m_attackAnimTimer = 0.25f;
        m_velocity.x = 0.0f;
    } else {
        m_fireCooldown -= dt;
        if (m_fireCooldown <= 0.0f) {
            m_fireChargeTimer = 0.52f;
            m_velocity.x = 0.0f;
        } else {
            const float speed = 64.0f;
            walk(level, world, true, speed, dt);
        }
    }

    if (m_fireChargeTimer > 0.0f || m_fireTimer > 0.0f) {
        m_velocity.y = std::min(m_velocity.y + 1850.0f * dt, 920.0f);
        const auto collision = CollisionManager::move(m_rect, m_velocity, level, world, true, dt);
        m_onGround = collision.onGround;
    }

    const float arenaLeft = 103.0f * Tile;
    const float arenaRight = 144.0f * Tile - m_rect.width;
    if (m_rect.left < arenaLeft) {
        m_rect.left = arenaLeft;
        m_direction = 1;
    } else if (m_rect.left > arenaRight) {
        m_rect.left = arenaRight;
        m_direction = -1;
    }
    if (!wasOnGround && m_onGround)
        events.publish({EventType::BossLanded, phase(), "land"});

    if (m_jumpTimer > 2.9f && m_onGround && m_fireChargeTimer <= 0.0f && m_fireTimer <= 0.0f) {
        m_velocity.y = -520.0f;
        m_jumpTimer = 0.0f;
    }

    if (m_hurtTimer > 0.0f)
        m_sprite.play("hurt");
    else if (m_attackAnimTimer > 0.0f)
        m_sprite.play("attack");
    else
        m_sprite.play("idle");
    m_sprite.update(dt);
}

bool Boss::stomp(Player& player, EventSystem& events)
{
    player.bounce(610.0f);
    if (m_state != BossState::ActiveFight)
        return false;
    damage(1, events);
    return !m_alive;
}

void Boss::damage(int value, EventSystem& events)
{
    if (!m_alive || m_state != BossState::ActiveFight || m_hurtTimer > 0.0f)
        return;
    const int oldPhase = phase();
    Enemy::damage(value, events);
    m_hurtTimer = 0.70f;
    if (m_alive && phase() != oldPhase)
        AudioManager::instance().play("boss");
    if (!m_alive) {
        m_state = BossState::Dead;
        m_sprite.play("death");
        events.publish({EventType::BossDefeated, m_variant, name()});
    }
}

std::string Boss::name() const
{
    return "Bowser";
}

bool Boss::isBoss() const { return true; }
float Boss::healthPercent() const { return Enemy::healthPercent(); }

void Boss::draw(sf::RenderWindow& window, const AssetManager& assets, float) const
{
    sf::Color tint = phase() == 3 ? sf::Color(255, 170, 170) : phase() == 2 ? sf::Color(255, 220, 180) : sf::Color::White;
    if (m_state == BossState::Sleeping)
        tint = sf::Color(170, 170, 190);
    if (m_state == BossState::Awakening && std::fmod(m_awakeTimer * 12.0f, 2.0f) > 1.0f)
        tint = sf::Color(255, 190, 130);
    if (m_hurtTimer > 0.0f && std::fmod(m_hurtTimer * 20.0f, 2.0f) > 1.0f)
        tint = sf::Color(255, 255, 255, 120);
    m_sprite.draw(window, assets, m_rect, m_direction > 0, tint);

    if (m_state == BossState::Awakening) {
        sf::CircleShape aura(78.0f, 48);
        aura.setOrigin(78.0f, 78.0f);
        aura.setPosition(rectCenter(m_rect));
        aura.setFillColor(sf::Color(255, 116, 44, static_cast<sf::Uint8>(35 + std::sin(m_awakeTimer * 14.0f) * 20.0f)));
        window.draw(aura, sf::BlendAdd);
    }

    if (m_fireChargeTimer > 0.0f) {
        const auto warning = fireRect();
        sf::RectangleShape telegraph({warning.width, warning.height});
        telegraph.setPosition(warning.left, warning.top);
        telegraph.setFillColor(sf::Color(255, 110, 38, static_cast<sf::Uint8>(42 + std::sin(m_fireChargeTimer * 36.0f) * 20.0f)));
        window.draw(telegraph, sf::BlendAdd);
    }

    if (fireActive()) {
        const auto flameRect = fireRect();
        const int dir = m_direction >= 0 ? 1 : -1;
        for (int i = 0; i < 7; ++i) {
            const float t = static_cast<float>(i) / 6.0f;
            const float radius = 25.0f - t * 7.0f;
            const float x = dir > 0 ? flameRect.left + t * flameRect.width : flameRect.left + flameRect.width - t * flameRect.width;
            const float y = flameRect.top + flameRect.height * (0.48f + std::sin(t * 9.0f + m_fireTimer * 10.0f) * 0.14f);
            sf::CircleShape flame(radius, 24);
            flame.setOrigin(radius, radius);
            flame.setPosition(x, y);
            flame.setFillColor(i % 2 == 0 ? sf::Color(255, 72, 28, 210) : sf::Color(255, 160, 40, 190));
            window.draw(flame, sf::BlendAdd);

            sf::CircleShape core(radius * 0.46f, 18);
            core.setOrigin(radius * 0.46f, radius * 0.46f);
            core.setPosition(x + dir * 5.0f, y - 2.0f);
            core.setFillColor(sf::Color(255, 235, 116, 215));
            window.draw(core, sf::BlendAdd);
        }
    }
}

int Boss::phase() const
{
    if (m_maxHp <= 0)
        return 1;
    if (m_hp <= 4)
        return 3;
    if (m_hp <= 8)
        return 2;
    return 1;
}

bool Boss::readyToRemove() const
{
    return !m_alive && m_deadTimer > 1.15f;
}

void Boss::resetForDungeon(sf::Vector2f pos, BossState state)
{
    m_rect = {pos.x, pos.y - 128.0f, 128.0f, 128.0f};
    m_velocity = {};
    m_hp = 12;
    m_maxHp = 12;
    m_alive = true;
    m_deadTimer = 0.0f;
    m_aiTimer = 0.0f;
    m_shootTimer = 0.0f;
    m_jumpTimer = 0.0f;
    m_hurtTimer = 0.0f;
    m_attackAnimTimer = 0.0f;
    m_awakeTimer = 0.0f;
    m_fireChargeTimer = 0.0f;
    m_fireTimer = 0.0f;
    m_fireCooldown = 1.8f;
    m_onGround = false;
    m_sprite.play("idle");
    setState(state);
}

void Boss::setState(BossState state)
{
    m_state = state;
    m_harmful = state == BossState::ActiveFight;
    if (state == BossState::Awakening) {
        m_awakeTimer = 0.0f;
        m_fireChargeTimer = 0.0f;
        m_fireTimer = 0.0f;
        m_fireCooldown = 1.5f;
    }
    if (state == BossState::Sleeping) {
        m_velocity.x = 0.0f;
        m_fireChargeTimer = 0.0f;
        m_fireTimer = 0.0f;
    }
}

BossState Boss::state() const { return m_state; }

bool Boss::fireActive() const
{
    return m_state == BossState::ActiveFight && m_fireTimer > 0.0f;
}

sf::FloatRect Boss::fireRect() const
{
    const float width = 205.0f;
    const float height = 64.0f;
    const float top = m_rect.top + 44.0f;
    if (m_direction >= 0)
        return {rectRight(m_rect) - 12.0f, top, width, height};
    return {m_rect.left - width + 12.0f, top, width, height};
}

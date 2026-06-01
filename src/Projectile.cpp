#include "Projectile.hpp"

#include "CollisionManager.hpp"
#include "Level.hpp"
#include "SpriteComponent.hpp"
#include "Sprites.hpp"

#include <cmath>

Projectile::Projectile(sf::Vector2f pos, sf::Vector2f velocity, bool fromPlayer, int damage, sf::Color color)
    : m_rect(pos.x, pos.y, 16.0f, 16.0f)
    , m_velocity(velocity)
    , m_fromPlayer(fromPlayer)
    , m_damage(damage)
    , m_color(color)
{
}

void Projectile::update(Level& level, float dt)
{
    m_life -= dt;
    m_rect.left += m_velocity.x * dt;
    m_rect.top += m_velocity.y * dt;
    if (m_life <= 0.0f || m_rect.left < 0.0f || m_rect.left > level.width() || m_rect.top > level.height())
        m_alive = false;

    const int row = static_cast<int>(std::floor(rectCenter(m_rect).y / Tile));
    const int col = static_cast<int>(std::floor(rectCenter(m_rect).x / Tile));
    if (level.isSolidTile(level.tileAt(row, col), WorldMode::Normal, true))
        m_alive = false;
}

void Projectile::draw(sf::RenderWindow& window, const AssetManager& assets, float time) const
{
    SpriteComponent sprite;
    if (m_fromPlayer) {
        const int frame = static_cast<int>(time * 12.0f);
        sprite.drawStatic(window, assets, "items_objects_npcs", Sprites::itemFrame(ItemType::FireFlower, frame), m_rect, m_velocity.x < 0.0f, m_color);
    } else {
        const sf::FloatRect target(m_rect.left - 4.0f, m_rect.top + 1.0f, 24.0f, 14.0f);
        sprite.drawStatic(window, assets, "mario_enemies", Sprites::enemyFrame("bullet", 0), target, m_velocity.x > 0.0f);
    }
}

sf::FloatRect Projectile::rect() const { return m_rect; }
bool Projectile::fromPlayer() const { return m_fromPlayer; }
int Projectile::damage() const { return m_damage; }
bool Projectile::alive() const { return m_alive; }
void Projectile::destroy() { m_alive = false; }

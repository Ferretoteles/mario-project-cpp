#include "Npc.hpp"

#include "AudioManager.hpp"
#include "CollisionManager.hpp"
#include "Player.hpp"

#include <cmath>

namespace {
sf::Color roleColor(NpcRole role)
{
    switch (role) {
    case NpcRole::GuideGhost:
        return sf::Color(180, 210, 255, 190);
    case NpcRole::Shopkeeper:
        return sf::Color(255, 184, 78);
    case NpcRole::Guard:
        return sf::Color(86, 150, 230);
    case NpcRole::Scientist:
        return sf::Color(190, 240, 220);
    case NpcRole::Mechanic:
        return sf::Color(220, 120, 78);
    default:
        return sf::Color(220, 220, 170);
    }
}

std::string defaultDialogue(NpcRole role)
{
    switch (role) {
    case NpcRole::GuideGhost:
        return "Maly duch: latarnia odkrywa sekrety. Rzuc ja klawiszem L.";
    case NpcRole::Shopkeeper:
        return "Sklepikarz: w menu sklepu kupisz zycia, skoki, bieg i skorki.";
    case NpcRole::Guard:
        return "Straznik: wyzszy skok i sprint pomagaja unikac przepasci.";
    case NpcRole::Scientist:
        return "Naukowiec: zbieraj karty, trofea i artefakty w skrzyniach.";
    case NpcRole::Mechanic:
        return "Mechanik: liczy sie najlepszy czas, monety i najmniej smierci.";
    default:
        return "NPC: dzieki za pomoc.";
    }
}
}

Npc::Npc(NpcRole role, sf::Vector2f pos, std::string name)
    : m_role(role)
    , m_rect(pos.x, pos.y - 44.0f, 32.0f, 44.0f)
    , m_name(std::move(name))
    , m_dialogue(defaultDialogue(role))
{
}

void Npc::update(Level& level, Player& player, float dt)
{
    m_timer += dt;
    const float distance = std::abs(player.center().x - rectCenter(m_rect).x);
    if (m_role == NpcRole::Guard && distance < 280.0f)
        m_direction = player.center().x < rectCenter(m_rect).x ? -1 : 1;

    const float speed = m_role == NpcRole::Guard && distance < 280.0f ? 82.0f : 38.0f;
    m_velocity.x = m_direction * speed;
    m_velocity.y = std::min(m_velocity.y + 1850.0f * dt, 860.0f);
    const auto collision = CollisionManager::move(m_rect, m_velocity, level, WorldMode::Normal, true, dt);
    if (collision.hitWall || (collision.onGround && edgeAhead(level)))
        m_direction *= -1;
    if (collision.hitWall && collision.onGround)
        m_velocity.y = -420.0f;
}

void Npc::draw(sf::RenderWindow& window, const AssetManager&, float time) const
{
    sf::Color color = roleColor(m_role);
    sf::RectangleShape body({m_rect.width, m_rect.height * 0.58f});
    body.setPosition(m_rect.left, m_rect.top + m_rect.height * 0.38f);
    body.setFillColor(color);
    body.setOutlineColor(sf::Color(32, 34, 48));
    body.setOutlineThickness(2.0f);
    window.draw(body);

    sf::CircleShape head(13.0f, 24);
    head.setOrigin(13.0f, 13.0f);
    head.setPosition(rectCenter(m_rect).x, m_rect.top + 14.0f + std::sin(time * 3.0f + m_timer) * 1.5f);
    head.setFillColor(m_role == NpcRole::GuideGhost ? sf::Color(220, 235, 255, 170) : sf::Color(255, 205, 156));
    head.setOutlineColor(sf::Color(32, 34, 48));
    head.setOutlineThickness(1.5f);
    window.draw(head);

    if (!m_rescued) {
        sf::CircleShape mark(6.0f, 16);
        mark.setOrigin(6.0f, 6.0f);
        mark.setPosition(rectCenter(m_rect).x, m_rect.top - 13.0f + std::sin(time * 5.0f) * 3.0f);
        mark.setFillColor(sf::Color(255, 238, 88));
        window.draw(mark);
    }
}

void Npc::interact(Player& player, EventSystem& events)
{
    if (!m_rescued) {
        m_rescued = true;
        player.addXp(75);
        player.stats().rescuedNpc += 1;
        events.publish({EventType::NpcRescued, 1, m_name});
        AudioManager::instance().play("quest");
    }
}

sf::FloatRect Npc::rect() const { return m_rect; }
bool Npc::rescued() const { return m_rescued; }
NpcRole Npc::role() const { return m_role; }
const std::string& Npc::name() const { return m_name; }
const std::string& Npc::dialogue() const { return m_dialogue; }

bool Npc::edgeAhead(Level& level) const
{
    const sf::Vector2f probe(rectCenter(m_rect).x + m_direction * 28.0f, rectBottom(m_rect) + 8.0f);
    const int row = static_cast<int>(std::floor(probe.y / Tile));
    const int col = static_cast<int>(std::floor(probe.x / Tile));
    return !level.isSolidTile(level.tileAt(row, col), WorldMode::Normal, true);
}

std::unique_ptr<Npc> makeNpc(const SpawnRequest& spawn)
{
    if (spawn.type == "guide")
        return std::make_unique<Npc>(NpcRole::GuideGhost, spawn.pos, "Maly duch");
    if (spawn.type == "shopkeeper")
        return std::make_unique<Npc>(NpcRole::Shopkeeper, spawn.pos, "Sklepikarz");
    if (spawn.type == "guard")
        return std::make_unique<Npc>(NpcRole::Guard, spawn.pos, "Straznik");
    if (spawn.type == "scientist")
        return std::make_unique<Npc>(NpcRole::Scientist, spawn.pos, "Naukowiec");
    if (spawn.type == "mechanic")
        return std::make_unique<Npc>(NpcRole::Mechanic, spawn.pos, "Mechanik");
    return std::make_unique<Npc>(NpcRole::Villager, spawn.pos, "Pixel Hero");
}

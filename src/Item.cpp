#include "Item.hpp"

#include "AudioManager.hpp"
#include "Level.hpp"
#include "Player.hpp"
#include "Sprites.hpp"

#include <algorithm>
#include <cmath>

namespace {
sf::FloatRect bobbed(sf::FloatRect rect, ItemType type, float time, float phase)
{
    if (type == ItemType::Coin || type == ItemType::Star || type == ItemType::FireFlower)
        rect.top += std::sin(time * 5.0f + phase) * 4.0f;
    return rect;
}

bool drawTextureItem(sf::RenderWindow& window, const AssetManager& assets, const std::string& textureId, sf::FloatRect rect)
{
    const sf::Texture* texture = assets.texture(textureId);
    if (!texture || texture->getSize().x == 0 || texture->getSize().y == 0)
        return false;

    sf::Sprite sprite(*texture);
    sprite.setPosition(rect.left, rect.top);
    sprite.setScale(rect.width / static_cast<float>(texture->getSize().x),
                    rect.height / static_cast<float>(texture->getSize().y));
    window.draw(sprite);
    return true;
}

void drawPixelStar(sf::RenderWindow& window, sf::FloatRect rect)
{
    static constexpr const char* pattern[9] = {
        "....X....",
        "...XXX...",
        "..XXXXX..",
        ".XXXXXXX.",
        "XXXXXXXXX",
        "..XXXXX..",
        ".XXX.XXX.",
        ".XX...XX.",
        "X.......X"
    };

    const float cell = std::floor(std::min(rect.width, rect.height) / 9.0f);
    const float starSize = cell * 9.0f;
    const sf::Vector2f origin(rect.left + (rect.width - starSize) * 0.5f, rect.top + (rect.height - starSize) * 0.5f);

    auto filled = [](int x, int y) {
        return x >= 0 && x < 9 && y >= 0 && y < 9 && pattern[y][x] == 'X';
    };

    auto drawCell = [&](int x, int y, sf::Color color) {
        sf::RectangleShape px({cell + 0.75f, cell + 0.75f});
        px.setPosition(origin.x + x * cell, origin.y + y * cell);
        px.setFillColor(color);
        window.draw(px);
    };

    for (int y = 0; y < 9; ++y) {
        for (int x = 0; x < 9; ++x) {
            if (filled(x, y))
                continue;
            bool edge = false;
            for (int oy = -1; oy <= 1; ++oy)
                for (int ox = -1; ox <= 1; ++ox)
                    edge = edge || filled(x + ox, y + oy);
            if (edge)
                drawCell(x, y, sf::Color(96, 64, 24));
        }
    }

    for (int y = 0; y < 9; ++y) {
        for (int x = 0; x < 9; ++x) {
            if (!filled(x, y))
                continue;
            const bool highlight = (x <= 3 && y <= 3) || (x == 4 && y == 1);
            drawCell(x, y, highlight ? sf::Color(255, 245, 126) : sf::Color(255, 196, 42));
        }
    }
}

void drawPixelFireFlower(sf::RenderWindow& window, sf::FloatRect rect)
{
    // Wyrazny kwiat z platkami - celowo inny od bulwiastej glowy strzelajacej rosliny (Shooter).
    auto circle = [&](sf::Vector2f c, float r, sf::Color col, sf::Color outline = sf::Color::Transparent) {
        sf::CircleShape s(r, 18);
        s.setOrigin(r, r);
        s.setPosition(c);
        s.setFillColor(col);
        if (outline != sf::Color::Transparent) {
            s.setOutlineColor(outline);
            s.setOutlineThickness(1.0f);
        }
        window.draw(s);
    };

    const float cx = rect.left + rect.width * 0.5f;
    const float bloomY = rect.top + rect.height * 0.34f;
    const float bloomR = std::min(rect.width, rect.height) * 0.17f;
    const float petalR = bloomR * 0.85f;
    const float ring = bloomR * 1.15f;

    // lodyga
    sf::RectangleShape stem({std::max(2.0f, rect.width * 0.10f), rect.height * 0.44f});
    stem.setPosition(cx - stem.getSize().x * 0.5f, bloomY);
    stem.setFillColor(sf::Color(46, 150, 60));
    stem.setOutlineColor(sf::Color(26, 96, 40));
    stem.setOutlineThickness(1.0f);
    window.draw(stem);

    // dwa liscie po bokach lodygi
    const float ly = rect.top + rect.height * 0.64f;
    sf::ConvexShape leafL(3);
    leafL.setPoint(0, {cx, ly});
    leafL.setPoint(1, {rect.left + rect.width * 0.16f, ly - rect.height * 0.06f});
    leafL.setPoint(2, {cx, ly + rect.height * 0.12f});
    leafL.setFillColor(sf::Color(72, 182, 82));
    leafL.setOutlineColor(sf::Color(34, 110, 46));
    leafL.setOutlineThickness(1.0f);
    window.draw(leafL);
    sf::ConvexShape leafR(3);
    leafR.setPoint(0, {cx, ly});
    leafR.setPoint(1, {rect.left + rect.width * 0.84f, ly - rect.height * 0.06f});
    leafR.setPoint(2, {cx, ly + rect.height * 0.12f});
    leafR.setFillColor(sf::Color(72, 182, 82));
    leafR.setOutlineColor(sf::Color(34, 110, 46));
    leafR.setOutlineThickness(1.0f);
    window.draw(leafR);

    // 6 platkow wokol srodka - czerwone z kremowym wnetrzem
    for (int i = 0; i < 6; ++i) {
        const float a = i * (3.14159265f * 2.0f / 6.0f) - 3.14159265f * 0.5f;
        const sf::Vector2f p(cx + std::cos(a) * ring, bloomY + std::sin(a) * ring);
        circle(p, petalR, sf::Color(228, 70, 52), sf::Color(150, 32, 34));
        circle(p, petalR * 0.5f, sf::Color(255, 226, 196));
    }

    // zlote oczko kwiatu z ciemnym srodkiem
    circle({cx, bloomY}, bloomR, sf::Color(255, 198, 64), sf::Color(196, 130, 30));
    circle({cx, bloomY}, bloomR * 0.5f, sf::Color(120, 70, 30));
}
}

Item::Item(ItemType type, sf::FloatRect rect)
    : m_type(type)
    , m_rect(rect)
{
    m_sprite.addAnimation("idle",
                          {"items_objects_npcs",
                           {Sprites::itemFrame(type, 0), Sprites::itemFrame(type, 1), Sprites::itemFrame(type, 2), Sprites::itemFrame(type, 3)},
                           type == ItemType::Coin ? 10.0f : 5.0f,
                           true});
    m_sprite.play("idle");
}

void Item::update(Player&, float dt)
{
    m_phase += dt;
    m_sprite.update(dt);
}

void Item::draw(sf::RenderWindow& window, const AssetManager& assets, float time) const
{
    const sf::FloatRect rect = bobbed(m_rect, m_type, time, m_phase);
    if (m_type == ItemType::Coin && drawTextureItem(window, assets, "coin", rect))
        return;
    if (m_type == ItemType::Mushroom && drawTextureItem(window, assets, "mushroom", rect))
        return;
    if (m_type == ItemType::Heart && drawTextureItem(window, assets, "heart", rect))
        return;
    if (m_type == ItemType::Key && drawTextureItem(window, assets, "key", rect))
        return;
    if (m_type == ItemType::FireFlower) {
        if (!drawTextureItem(window, assets, "fireflower", rect))
            drawPixelFireFlower(window, rect);
        return;
    }
    if (m_type == ItemType::Star) {
        if (!drawTextureItem(window, assets, "star", rect))
            drawPixelStar(window, rect);
        return;
    }
    m_sprite.draw(window, assets, rect);
}

void Item::collect(Player&, EventSystem&)
{
    m_alive = false;
}

sf::FloatRect Item::rect() const { return m_rect; }
bool Item::alive() const { return m_alive; }
ItemType Item::type() const { return m_type; }

Coin::Coin(sf::Vector2f pos)
    : Item(ItemType::Coin, {pos.x, pos.y, 24.0f, 24.0f})
{
}

void Coin::draw(sf::RenderWindow& window, const AssetManager& assets, float time) const
{
    Item::draw(window, assets, time);
}

void Coin::collect(Player& player, EventSystem& events)
{
    player.addCoin(events);
    m_alive = false;
}

HeartItem::HeartItem(sf::Vector2f pos)
    : Item(ItemType::Heart, {pos.x, pos.y, 28.0f, 28.0f})
{
}

void HeartItem::draw(sf::RenderWindow& window, const AssetManager& assets, float time) const
{
    Item::draw(window, assets, time);
}

void HeartItem::collect(Player& player, EventSystem&)
{
    player.heal(1);
    AudioManager::instance().play("power");
    m_alive = false;
}

Mushroom::Mushroom(sf::Vector2f pos)
    : Item(ItemType::Mushroom, {pos.x, pos.y, 32.0f, 32.0f})
{
}

void Mushroom::draw(sf::RenderWindow& window, const AssetManager& assets, float time) const
{
    Item::draw(window, assets, time);
}

void Mushroom::collect(Player& player, EventSystem&)
{
    player.makeBig();
    player.addXp(50);
    m_alive = false;
}

StarItem::StarItem(sf::Vector2f pos)
    : Item(ItemType::Star, {pos.x, pos.y, 32.0f, 32.0f})
{
}

void StarItem::draw(sf::RenderWindow& window, const AssetManager& assets, float time) const
{
    Item::draw(window, assets, time);
}

void StarItem::collect(Player& player, EventSystem&)
{
    player.giveStar(8.0f);
    player.addXp(120);
    m_alive = false;
}

FireFlowerItem::FireFlowerItem(sf::Vector2f pos)
    : Item(ItemType::FireFlower, {pos.x, pos.y, 32.0f, 32.0f})
{
}

void FireFlowerItem::draw(sf::RenderWindow& window, const AssetManager& assets, float time) const
{
    Item::draw(window, assets, time);
}

void FireFlowerItem::collect(Player& player, EventSystem&)
{
    player.giveFireFlower(16.0f);
    player.addXp(100);
    m_alive = false;
}

KeyItem::KeyItem(sf::Vector2f pos)
    : Item(ItemType::Key, {pos.x, pos.y - 4.0f, 32.0f, 32.0f})
{
}

void KeyItem::draw(sf::RenderWindow& window, const AssetManager& assets, float time) const
{
    Item::draw(window, assets, time);
}

void KeyItem::collect(Player& player, EventSystem& events)
{
    player.giveKey();
    player.addXp(80);
    events.publish({EventType::KeyFound, 1, "key"});
    AudioManager::instance().play("quest");
    m_alive = false;
}

std::unique_ptr<Item> makeItem(const SpawnRequest& spawn)
{
    if (spawn.type == "coin")
        return std::make_unique<Coin>(spawn.pos);
    if (spawn.type == "heart")
        return std::make_unique<HeartItem>(spawn.pos);
    if (spawn.type == "mushroom")
        return std::make_unique<Mushroom>(spawn.pos);
    if (spawn.type == "star")
        return std::make_unique<StarItem>(spawn.pos);
    if (spawn.type == "fireflower")
        return std::make_unique<FireFlowerItem>(spawn.pos);
    if (spawn.type == "key")
        return std::make_unique<KeyItem>(spawn.pos);
    return nullptr;
}

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
    if (type == ItemType::Coin || type == ItemType::Star || type == ItemType::FireFlower || type == ItemType::Fuel)
        rect.top += std::sin(time * 5.0f + phase) * 4.0f;
    return rect;
}

sf::Color rarityColor(Rarity rarity)
{
    switch (rarity) {
    case Rarity::Rare:
        return sf::Color(82, 174, 255);
    case Rarity::Epic:
        return sf::Color(176, 94, 255);
    case Rarity::Legendary:
        return sf::Color(255, 186, 57);
    default:
        return sf::Color(210, 145, 68);
    }
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
    static constexpr const char* pattern[16] = {
        ".....XXXXXX.....",
        "....XRRRRRRX....",
        "...XROOOORRX....",
        "...XROYYORRX....",
        "...XROYYORRX....",
        "...XROOOORRX....",
        "....XRRRRRX.....",
        "......XXXX......",
        ".......GG.......",
        "....LLLGGLLL....",
        "...LL..GG..LL...",
        "...L...GG...L...",
        ".......GG.......",
        ".......GG.......",
        "......GGGG......",
        ".....GG..GG....."
    };

    const float cell = std::floor(std::min(rect.width, rect.height) / 16.0f);
    const float spriteSize = cell * 16.0f;
    const sf::Vector2f origin(rect.left + (rect.width - spriteSize) * 0.5f, rect.top + rect.height - spriteSize);

    auto colorFor = [](char ch) {
        switch (ch) {
        case 'X':
            return sf::Color(70, 43, 30);
        case 'R':
            return sf::Color(230, 64, 42);
        case 'O':
            return sf::Color(255, 146, 43);
        case 'Y':
            return sf::Color(255, 242, 150);
        case 'G':
            return sf::Color(34, 132, 56);
        case 'L':
            return sf::Color(83, 194, 76);
        default:
            return sf::Color::Transparent;
        }
    };

    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 16; ++x) {
            const sf::Color color = colorFor(pattern[y][x]);
            if (color == sf::Color::Transparent)
                continue;
            sf::RectangleShape px({cell + 0.5f, cell + 0.5f});
            px.setPosition(origin.x + x * cell, origin.y + y * cell);
            px.setFillColor(color);
            window.draw(px);
        }
    }
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

FuelItem::FuelItem(sf::Vector2f pos)
    : Item(ItemType::Fuel, {pos.x, pos.y, 26.0f, 30.0f})
{
}

void FuelItem::draw(sf::RenderWindow& window, const AssetManager& assets, float time) const
{
    Item::draw(window, assets, time);
}

void FuelItem::collect(Player& player, EventSystem& events)
{
    player.addFuel(38.0f, events);
    m_alive = false;
}

ChestItem::ChestItem(sf::Vector2f pos, Rarity rarity)
    : Item(ItemType::Chest, {pos.x, pos.y, 38.0f, 30.0f})
    , m_rarity(rarity)
{
}

void ChestItem::draw(sf::RenderWindow& window, const AssetManager& assets, float) const
{
    m_sprite.draw(window, assets, m_rect, false, rarityColor(m_rarity));
}

void ChestItem::collect(Player& player, EventSystem& events)
{
    if (!player.hasKey())
        return;
    const int reward = m_rarity == Rarity::Legendary ? 60 : m_rarity == Rarity::Epic ? 35 : m_rarity == Rarity::Rare ? 20 : 10;
    for (int i = 0; i < reward; ++i)
        player.addCoin(events, 1);
    player.addXp(reward * 3);
    player.stats().chests += 1;
    events.publish({EventType::ChestOpened, reward, "chest"});
    AudioManager::instance().play("chest");
    m_alive = false;
}

Rarity ChestItem::rarity() const { return m_rarity; }

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
    if (spawn.type == "fuel")
        return std::make_unique<FuelItem>(spawn.pos);
    if (spawn.type == "chest")
        return std::make_unique<ChestItem>(spawn.pos, static_cast<Rarity>(std::clamp(spawn.variant, 0, 3)));
    return nullptr;
}

#include "AssetManager.hpp"

#include <cstdlib>
#include <fstream>
#include <iterator>

namespace {
std::vector<char> readBytes(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return {};
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
}

AssetManager::AssetManager(std::filesystem::path root)
    : m_root(std::move(root))
{
}

void AssetManager::setRoot(std::filesystem::path root)
{
    m_root = std::move(root);
}

void AssetManager::load()
{
    loadFont();
    loadTextureWithColorKey("mario_player", "sprites/mario_player_clean.png", 0);
    loadTextureWithColorKey("mario_enemies", "sprites/mario_enemies_clean.png", 0);
    loadTextureWithColorKey("hero_sheet", "sprites/hero_sheet.png", 0);
    loadTextureWithColorKey("characters", "spritesheets/characters.png");
    loadTextureWithColorKey("enemies_bosses", "spritesheets/enemies_bosses.png");
    loadTextureWithColorKey("items_objects_npcs", "spritesheets/items_objects_npcs.png");
    loadTexture("blocks", "spritesheets/blocks.png");
    loadTexture("tileset", "spritesheets/tileset.png");
    loadTexture("background_mountains", "spritesheets/background_mountains.png");
    loadTexture("background_trees", "spritesheets/background_trees.png");
    loadTexture("background_clouds", "spritesheets/background_clouds.png");
    loadTexture("level_grass_background", "levels/grass/background.png");
    loadTexture("level_desert_background", "levels/desert/background.png");
    loadTexture("level_ice_background", "levels/ice/background.png");
    loadTexture("level_castle_background", "levels/castle/background.png");
    loadTexture("level_lava_background", "levels/lava/background.png");
    loadTextureWithColorKey("level_grass_tiles", "levels/grass/tiles_sheet.png", 18);
    loadTextureWithColorKey("level_desert_tiles", "levels/desert/tiles_sheet.png", 18);
    loadTextureWithColorKey("level_ice_tiles", "levels/ice/tiles_sheet.png", 18);
    loadTextureWithColorKey("level_castle_tiles", "levels/castle/tiles_sheet.png", 18);
    loadTextureWithColorKey("level_lava_tiles", "levels/lava/tiles_sheet.png", 18);
    loadTexture("level_grass_inner", "levels/grass/inner_block.png");
    loadTexture("level_desert_inner", "levels/desert/inner_block.png");
    loadTexture("level_ice_inner", "levels/ice/inner_block.png");
    loadTexture("level_castle_inner", "levels/castle/inner_block.png");
    loadTexture("level_lava_inner", "levels/lava/inner_block.png");
    loadTextureWithColorKey("level_grass_pipes", "levels/grass/pipes_sheet.png", 18);
    loadTextureWithColorKey("level_desert_pipes", "levels/desert/pipes_sheet.png", 18);
    loadTextureWithColorKey("level_ice_pipes", "levels/ice/pipes_sheet.png", 18);
    loadTextureWithColorKey("level_castle_pipes", "levels/castle/pipes_sheet.png", 18);
    loadTextureWithColorKey("level_lava_pipes", "levels/lava/pipes_sheet.png", 18);
    loadTexture("menu_back", "menu/back.png");
    loadTextureWithColorKey("menu_button", "menu/menu_button_pressed.png", 18);
    loadTextureWithColorKey("menu_panel", "menu/menu_panel.png", 18);
    loadTexture("final_castle_background", "backgrounds/final_castle_background.png");
    loadTexture("castle_exterior", "backgrounds/castle_exterior.png");
    loadTexture("castle_dungeon_arena", "backgrounds/castle_dungeon_arena.png");
    loadTextureWithColorKey("player_sheet", "spritesheets/characters.png");
    loadTextureWithColorKey("enemy_sheet", "spritesheets/enemies_bosses.png");
    loadTextureWithColorKey("player_small", "sprites/player_small.png", 0);
    loadTextureWithColorKey("player_big", "sprites/player_big.png", 0);
    loadTextureWithColorKey("coin", "sprites/coin.png", 0);
    loadTextureWithColorKey("mushroom", "sprites/mushroom.png", 0);
    loadTextureWithColorKey("star", "sprites/star.png", 0);
    loadTextureWithColorKey("fireflower", "sprites/fireflower.png", 0);
    loadTextureWithColorKey("key", "sprites/key.png", 0);
    loadTextureWithColorKey("heart", "sprites/heart.png", 0);
    loadTextureWithColorKey("bowser_clean", "sprites/bowser_clean.png", 0);
    loadTexture("bowser_boss", "sprites/bowser_boss.png");
    loadTexture("boss", "sprites/boss.png");
}

const sf::Texture* AssetManager::texture(const std::string& id) const
{
    const auto found = m_textures.find(id);
    return found == m_textures.end() ? nullptr : &found->second;
}

const sf::Font& AssetManager::font() const
{
    return m_font;
}

bool AssetManager::hasFont() const
{
    return m_fontReady;
}

std::filesystem::path AssetManager::root() const
{
    return m_root;
}

std::filesystem::path AssetManager::path(const std::string& relative) const
{
    return m_root / relative;
}

void AssetManager::loadTexture(const std::string& id, const std::string& relative)
{
    const auto bytes = readBytes(path(relative));
    if (bytes.empty())
        return;

    sf::Texture texture;
    if (texture.loadFromMemory(bytes.data(), bytes.size())) {
        texture.setSmooth(false);
        m_textures[id] = std::move(texture);
    }
}

void AssetManager::loadTextureWithColorKey(const std::string& id, const std::string& relative, int tolerance)
{
    const auto file = path(relative);
    sf::Image image;
    bool loaded = image.loadFromFile(file.string());

    if (!loaded) {
        const auto bytes = readBytes(file);
        loaded = !bytes.empty() && image.loadFromMemory(bytes.data(), bytes.size());
    }
    if (!loaded || image.getSize().x == 0 || image.getSize().y == 0)
        return;

    const sf::Color key = image.getPixel(0, 0);
    if (tolerance <= 0) {
        image.createMaskFromColor(key);
    } else {
        const sf::Vector2u size = image.getSize();
        for (unsigned y = 0; y < size.y; ++y) {
            for (unsigned x = 0; x < size.x; ++x) {
                sf::Color pixel = image.getPixel(x, y);
                const int dr = std::abs(static_cast<int>(pixel.r) - static_cast<int>(key.r));
                const int dg = std::abs(static_cast<int>(pixel.g) - static_cast<int>(key.g));
                const int db = std::abs(static_cast<int>(pixel.b) - static_cast<int>(key.b));
                if (dr <= tolerance && dg <= tolerance && db <= tolerance) {
                    pixel.a = 0;
                    image.setPixel(x, y, pixel);
                }
            }
        }
    }

    sf::Texture texture;
    if (texture.loadFromImage(image)) {
        texture.setSmooth(false);
        m_textures[id] = std::move(texture);
    }
}

void AssetManager::loadFont()
{
    const std::vector<std::filesystem::path> candidates = {
        path("fonts/game.ttf"),
        path("fonts/PressStart2P.ttf"),
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf"
    };

    for (const auto& candidate : candidates) {
        auto bytes = readBytes(candidate);
        if (bytes.empty())
            continue;
        if (m_font.loadFromMemory(bytes.data(), bytes.size())) {
            m_fontBytes = std::move(bytes);
            m_fontReady = true;
            return;
        }
    }
}

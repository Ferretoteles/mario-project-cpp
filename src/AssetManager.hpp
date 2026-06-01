#pragma once

#include "Core.hpp"

#include <filesystem>

class AssetManager {
public:
    explicit AssetManager(std::filesystem::path root = {});

    void setRoot(std::filesystem::path root);
    void load();

    const sf::Texture* texture(const std::string& id) const;
    const sf::Font& font() const;
    bool hasFont() const;
    std::filesystem::path root() const;
    std::filesystem::path path(const std::string& relative) const;

private:
    void loadTexture(const std::string& id, const std::string& relative);
    void loadTextureWithColorKey(const std::string& id, const std::string& relative, int tolerance = 8);
    void loadFont();

    std::filesystem::path m_root;
    std::unordered_map<std::string, sf::Texture> m_textures;
    sf::Font m_font;
    std::vector<char> m_fontBytes;
    bool m_fontReady = false;
};

#pragma once

#include "AssetManager.hpp"

struct SpriteAnimation {
    std::string textureId;
    std::vector<sf::IntRect> frames;
    float fps = 8.0f;
    bool loop = true;
};

class SpriteComponent {
public:
    void addAnimation(const std::string& name, SpriteAnimation animation);
    void play(const std::string& name);
    void update(float dt);
    void draw(sf::RenderWindow& window,
              const AssetManager& assets,
              const sf::FloatRect& target,
              bool flipX = false,
              sf::Color tint = sf::Color::White) const;
    void drawStatic(sf::RenderWindow& window,
                    const AssetManager& assets,
                    const std::string& textureId,
                    sf::IntRect source,
                    const sf::FloatRect& target,
                    bool flipX = false,
                    sf::Color tint = sf::Color::White) const;
    bool hasAnimation(const std::string& name) const;
    const std::string& current() const;

private:
    const SpriteAnimation* currentAnimation() const;

    std::unordered_map<std::string, SpriteAnimation> m_animations;
    std::string m_current;
    float m_time = 0.0f;
    std::size_t m_frame = 0;
};

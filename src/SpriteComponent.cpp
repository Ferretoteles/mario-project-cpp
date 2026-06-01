#include "SpriteComponent.hpp"

void SpriteComponent::addAnimation(const std::string& name, SpriteAnimation animation)
{
    m_animations[name] = std::move(animation);
}

void SpriteComponent::play(const std::string& name)
{
    if (m_current == name)
        return;
    if (!hasAnimation(name))
        return;
    m_current = name;
    m_time = 0.0f;
    m_frame = 0;
}

void SpriteComponent::update(float dt)
{
    const SpriteAnimation* animation = currentAnimation();
    if (!animation || animation->frames.empty() || animation->fps <= 0.0f)
        return;

    m_time += dt;
    const float frameTime = 1.0f / animation->fps;
    while (m_time >= frameTime) {
        m_time -= frameTime;
        if (m_frame + 1 < animation->frames.size()) {
            ++m_frame;
        } else if (animation->loop) {
            m_frame = 0;
        }
    }
}

void SpriteComponent::draw(sf::RenderWindow& window,
                           const AssetManager& assets,
                           const sf::FloatRect& target,
                           bool flipX,
                           sf::Color tint) const
{
    const SpriteAnimation* animation = currentAnimation();
    if (!animation || animation->frames.empty())
        return;
    drawStatic(window, assets, animation->textureId, animation->frames[std::min(m_frame, animation->frames.size() - 1)], target, flipX, tint);
}

void SpriteComponent::drawStatic(sf::RenderWindow& window,
                                 const AssetManager& assets,
                                 const std::string& textureId,
                                 sf::IntRect source,
                                 const sf::FloatRect& target,
                                 bool flipX,
                                 sf::Color tint) const
{
    const sf::Texture* texture = assets.texture(textureId);
    if (!texture)
        return;

    sf::Sprite sprite(*texture);
    sprite.setTextureRect(source);
    sprite.setColor(tint);
    if (flipX) {
        sprite.setOrigin(static_cast<float>(source.width), 0.0f);
        sprite.setPosition(target.left + target.width, target.top);
        sprite.setScale(-target.width / static_cast<float>(source.width), target.height / static_cast<float>(source.height));
    } else {
        sprite.setPosition(target.left, target.top);
        sprite.setScale(target.width / static_cast<float>(source.width), target.height / static_cast<float>(source.height));
    }
    window.draw(sprite);
}

bool SpriteComponent::hasAnimation(const std::string& name) const
{
    return m_animations.find(name) != m_animations.end();
}

const std::string& SpriteComponent::current() const
{
    return m_current;
}

const SpriteAnimation* SpriteComponent::currentAnimation() const
{
    const auto found = m_animations.find(m_current);
    if (found == m_animations.end())
        return nullptr;
    return &found->second;
}

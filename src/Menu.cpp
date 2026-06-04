#include "Menu.hpp"

#include <cmath>

void Menu::set(const std::string& title, std::vector<std::string> items)
{
    m_title = title;
    m_items = std::move(items);
    m_selected = std::clamp(m_selected, 0, std::max(0, static_cast<int>(m_items.size()) - 1));
}

void Menu::up()
{
    if (!m_items.empty())
        m_selected = (m_selected + static_cast<int>(m_items.size()) - 1) % static_cast<int>(m_items.size());
}

void Menu::down()
{
    if (!m_items.empty())
        m_selected = (m_selected + 1) % static_cast<int>(m_items.size());
}

int Menu::index() const
{
    return m_selected;
}

void Menu::draw(sf::RenderWindow& window, const AssetManager& assets, sf::Vector2f size, const std::string& subtitle) const
{
    sf::VertexArray background(sf::Quads, 4);
    background[0].position = {0.0f, 0.0f};
    background[1].position = {size.x, 0.0f};
    background[2].position = {size.x, size.y};
    background[3].position = {0.0f, size.y};
    background[0].color = sf::Color(92, 184, 232);
    background[1].color = sf::Color(92, 184, 232);
    background[2].color = sf::Color(184, 224, 248);
    background[3].color = sf::Color(184, 224, 248);
    window.draw(background);

    for (int i = 0; i < 4; ++i) {
        const float x = 80.0f + i * 210.0f;
        const float y = 65.0f + (i % 2) * 34.0f;
        sf::CircleShape c1(18.0f, 24);
        sf::CircleShape c2(24.0f, 24);
        sf::CircleShape c3(18.0f, 24);
        c1.setFillColor(sf::Color(255, 255, 255, 210));
        c2.setFillColor(sf::Color(255, 255, 255, 220));
        c3.setFillColor(sf::Color(255, 255, 255, 210));
        c1.setPosition(x, y + 14.0f);
        c2.setPosition(x + 24.0f, y);
        c3.setPosition(x + 58.0f, y + 16.0f);
        window.draw(c1);
        window.draw(c2);
        window.draw(c3);
        sf::RectangleShape base({74.0f, 18.0f});
        base.setPosition(x + 11.0f, y + 32.0f);
        base.setFillColor(sf::Color(255, 255, 255, 215));
        window.draw(base);
    }

    sf::RectangleShape panel({std::min(520.0f, size.x - 80.0f), std::min(390.0f, size.y - 120.0f)});
    panel.setOrigin(panel.getSize().x * 0.5f, panel.getSize().y * 0.5f);
    panel.setPosition(size.x * 0.5f, size.y * 0.52f);
    panel.setFillColor(sf::Color(36, 58, 128, 222));
    panel.setOutlineColor(sf::Color(252, 225, 92, 220));
    panel.setOutlineThickness(3.0f);
    window.draw(panel);

    if (!assets.hasFont())
        return;

    auto drawText = [&](const std::string& text, unsigned charSize, sf::Vector2f pos, sf::Color color, bool center, float outline = 2.0f) {
        sf::Text label(text, assets.font(), charSize);
        label.setStyle(sf::Text::Bold);
        label.setFillColor(color);
        label.setOutlineColor(sf::Color(12, 20, 35));
        label.setOutlineThickness(outline);
        if (center) {
            const auto bounds = label.getLocalBounds();
            label.setOrigin(bounds.left + bounds.width * 0.5f, bounds.top + bounds.height * 0.5f);
        }
        label.setPosition({std::round(pos.x), std::round(pos.y)});
        window.draw(label);
    };

    auto drawTitle = [&](const std::string& text, unsigned charSize, sf::Vector2f pos) {
        drawText(text, charSize, pos + sf::Vector2f(4.0f, 5.0f), sf::Color(92, 54, 34, 180), true, 0.0f);
        drawText(text, charSize, pos, sf::Color(255, 232, 80), true, 2.5f);
    };

    drawTitle(m_title, 48, {size.x * 0.5f, 78.0f});
    if (!subtitle.empty())
        drawText(subtitle, 16, {size.x * 0.5f, 126.0f}, sf::Color(255, 244, 190), true);

    const float itemW = std::min(430.0f, size.x - 140.0f);
    const float itemH = 42.0f;
    const float firstY = 174.0f;
    const float itemStep = 50.0f;
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        const bool selected = i == m_selected;
        const float y = firstY + i * itemStep;
        sf::RectangleShape card({itemW, itemH});
        card.setOrigin(itemW * 0.5f, itemH * 0.5f);
        card.setPosition(std::round(size.x * 0.5f), std::round(y));
        card.setFillColor(selected ? sf::Color(218, 82, 42, 238) : sf::Color(240, 175, 69, 224));
        card.setOutlineColor(selected ? sf::Color(255, 248, 150) : sf::Color(122, 72, 38, 190));
        card.setOutlineThickness(selected ? 3.0f : 1.0f);
        window.draw(card);
        drawText(m_items[static_cast<std::size_t>(i)], 22, {card.getPosition().x, card.getPosition().y + 1.0f}, sf::Color::White, true);
    }

    sf::RectangleShape ground({size.x, 34.0f});
    ground.setPosition(0.0f, size.y - 34.0f);
    ground.setFillColor(sf::Color(184, 91, 45));
    window.draw(ground);
    drawText("W/S lub strzalki: wybor   ENTER: zatwierdz   ESC: powrot", 14, {size.x * 0.5f, size.y - 19.0f}, sf::Color(255, 244, 190), true);
}

#include "Menu.hpp"

#include <cmath>

namespace {
float clamp01(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

float easeOut(float value)
{
    const float t = clamp01(value);
    return 1.0f - (1.0f - t) * (1.0f - t);
}

void drawCoverTexture(sf::RenderWindow& window, const sf::Texture& texture, sf::Vector2f size)
{
    const sf::Vector2u texSize = texture.getSize();
    if (texSize.x == 0 || texSize.y == 0)
        return;

    const float scale = std::max(size.x / static_cast<float>(texSize.x), size.y / static_cast<float>(texSize.y));
    sf::Sprite sprite(texture);
    sprite.setScale(scale, scale);
    sprite.setPosition((size.x - static_cast<float>(texSize.x) * scale) * 0.5f,
                       (size.y - static_cast<float>(texSize.y) * scale) * 0.5f);
    window.draw(sprite);
}

void drawSpriteInRect(sf::RenderWindow& window, const sf::Texture& texture, sf::FloatRect rect, sf::Color color = sf::Color::White)
{
    const sf::Vector2u texSize = texture.getSize();
    if (texSize.x == 0 || texSize.y == 0)
        return;

    sf::Sprite sprite(texture);
    sprite.setPosition(rect.left, rect.top);
    sprite.setScale(rect.width / static_cast<float>(texSize.x), rect.height / static_cast<float>(texSize.y));
    sprite.setColor(color);
    window.draw(sprite);
}

void drawSoftCloud(sf::RenderWindow& window, sf::Vector2f pos, float scale, sf::Uint8 alpha)
{
    const sf::Color cloudColor(255, 255, 255, alpha);
    const std::array<sf::Vector3f, 4> parts{{
        {0.0f, 18.0f, 22.0f},
        {28.0f, 8.0f, 30.0f},
        {66.0f, 18.0f, 24.0f},
        {94.0f, 24.0f, 18.0f},
    }};
    for (const auto& part : parts) {
        sf::CircleShape circle(part.z * scale, 24);
        circle.setPosition(pos.x + part.x * scale, pos.y + part.y * scale);
        circle.setFillColor(cloudColor);
        window.draw(circle);
    }
    sf::RectangleShape base({112.0f * scale, 25.0f * scale});
    base.setPosition(pos.x + 10.0f * scale, pos.y + 36.0f * scale);
    base.setFillColor(cloudColor);
    window.draw(base);
}

void drawFallbackMenuBackground(sf::RenderWindow& window, sf::Vector2f size)
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
}

void drawMenuClouds(sf::RenderWindow& window, const sf::Texture& texture, sf::Vector2f size, float time)
{
    const sf::Vector2u texSize = texture.getSize();
    if (texSize.x == 0 || texSize.y == 0)
        return;

    const int cloudCount = 3;
    const int cloudH = static_cast<int>(texSize.y / cloudCount);
    const float widths[cloudCount] = {210.0f, 260.0f, 180.0f};
    const float speeds[cloudCount] = {18.0f, 12.0f, 15.0f};
    const float yPositions[cloudCount] = {28.0f, 82.0f, 138.0f};
    const float offsets[cloudCount] = {40.0f, 330.0f, 650.0f};

    for (int i = 0; i < cloudCount; ++i) {
        const sf::IntRect rect(0, i * cloudH, static_cast<int>(texSize.x), i == cloudCount - 1 ? static_cast<int>(texSize.y) - i * cloudH : cloudH);
        const float cloudW = std::min(widths[i], size.x * 0.32f);
        const float cloudHeight = cloudW * static_cast<float>(rect.height) / static_cast<float>(rect.width);
        const float span = size.x + cloudW + 80.0f;
        const float x = size.x + 40.0f - std::fmod(time * speeds[i] + offsets[i], span);

        sf::Sprite cloud(texture, rect);
        cloud.setPosition(std::round(x), std::round(yPositions[i]));
        cloud.setScale(cloudW / static_cast<float>(rect.width), cloudHeight / static_cast<float>(rect.height));
        cloud.setColor(sf::Color(255, 255, 255, 185));
        window.draw(cloud);
    }
}
}

void Menu::set(const std::string& title, std::vector<std::string> items)
{
    m_title = title;
    m_items = std::move(items);
    m_selected = std::clamp(m_selected, 0, std::max(0, static_cast<int>(m_items.size()) - 1));
}

void Menu::up()
{
    if (!m_items.empty()) {
        m_selected = (m_selected + static_cast<int>(m_items.size()) - 1) % static_cast<int>(m_items.size());
        m_selectionChangedAt = m_effectClock.getElapsedTime().asSeconds();
    }
}

void Menu::down()
{
    if (!m_items.empty()) {
        m_selected = (m_selected + 1) % static_cast<int>(m_items.size());
        m_selectionChangedAt = m_effectClock.getElapsedTime().asSeconds();
    }
}

void Menu::activateSelection()
{
    m_activatedAt = m_effectClock.getElapsedTime().asSeconds();
}

int Menu::index() const
{
    return m_selected;
}

void Menu::draw(sf::RenderWindow& window, const AssetManager& assets, sf::Vector2f size, const std::string& subtitle, float time) const
{
    const bool mainMenu = m_title == "MARIO STYLE RUN";
    const float now = m_effectClock.getElapsedTime().asSeconds();

    if (mainMenu) {
        if (const sf::Texture* bg = assets.texture("menu_back"))
            drawCoverTexture(window, *bg, size);
        else
            drawFallbackMenuBackground(window, size);

        sf::RectangleShape shade(size);
        shade.setFillColor(sf::Color(6, 12, 24, 38));
        window.draw(shade);

        if (const sf::Texture* clouds = assets.texture("menu_clouds"))
            drawMenuClouds(window, *clouds, size, time);
    } else {
        drawFallbackMenuBackground(window, size);
    }

    if (!mainMenu) {
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
    }

    float panelW = mainMenu ? std::min(430.0f, size.x - 44.0f) : std::min(520.0f, size.x - 80.0f);
    float panelH = mainMenu ? std::min(520.0f, size.y - 20.0f) : std::min(390.0f, size.y - 120.0f);
    if (mainMenu) {
        if (const sf::Texture* panelTexture = assets.texture("menu_panel")) {
            const sf::Vector2u panelSize = panelTexture->getSize();
            if (panelSize.x > 0 && panelSize.y > 0) {
                panelW = panelH * static_cast<float>(panelSize.x) / static_cast<float>(panelSize.y);
                if (panelW > size.x - 44.0f) {
                    panelW = size.x - 44.0f;
                    panelH = panelW * static_cast<float>(panelSize.y) / static_cast<float>(panelSize.x);
                }
            }
        }
    }
    sf::RectangleShape panel({panelW, panelH});
    panel.setOrigin(panel.getSize().x * 0.5f, panel.getSize().y * 0.5f);
    panel.setPosition(size.x * 0.5f, mainMenu ? std::round(size.y * 0.5f) : size.y * 0.52f);
    panel.setFillColor(mainMenu ? sf::Color(20, 28, 52, 214) : sf::Color(36, 58, 128, 222));
    panel.setOutlineColor(mainMenu ? sf::Color(255, 230, 104, 230) : sf::Color(252, 225, 92, 220));
    panel.setOutlineThickness(3.0f);
    if (mainMenu) {
        if (const sf::Texture* panelTexture = assets.texture("menu_panel")) {
            drawSpriteInRect(window, *panelTexture, {panel.getPosition().x - panelW * 0.5f, panel.getPosition().y - panelH * 0.5f, panelW, panelH});
        } else {
            sf::RectangleShape shadow(panel.getSize() + sf::Vector2f(10.0f, 12.0f));
            shadow.setOrigin(shadow.getSize().x * 0.5f, shadow.getSize().y * 0.5f);
            shadow.setPosition(panel.getPosition() + sf::Vector2f(6.0f, 8.0f));
            shadow.setFillColor(sf::Color(0, 0, 0, 92));
            window.draw(shadow);
            window.draw(panel);
        }
    } else {
        sf::RectangleShape shadow(panel.getSize() + sf::Vector2f(10.0f, 12.0f));
        shadow.setOrigin(shadow.getSize().x * 0.5f, shadow.getSize().y * 0.5f);
        shadow.setPosition(panel.getPosition() + sf::Vector2f(6.0f, 8.0f));
        shadow.setFillColor(sf::Color(0, 0, 0, 92));
        window.draw(shadow);
        window.draw(panel);
    }

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

    const float panelTop = panel.getPosition().y - panel.getSize().y * 0.5f;
    const float panelBottom = panel.getPosition().y + panel.getSize().y * 0.5f;
    drawTitle(m_title, mainMenu ? 34 : 48, mainMenu ? sf::Vector2f(panel.getPosition().x, panelTop + 46.0f) : sf::Vector2f(size.x * 0.5f, 78.0f));
    if (!subtitle.empty())
        drawText(subtitle, mainMenu ? 12 : 16, mainMenu ? sf::Vector2f(panel.getPosition().x, panelTop + 82.0f) : sf::Vector2f(size.x * 0.5f, 126.0f), sf::Color(255, 244, 190), true);

    const float itemW = mainMenu ? panelW * 0.78f : std::min(430.0f, size.x - 140.0f);
    const float firstY = mainMenu ? panelTop + 128.0f : 164.0f;
    const float listBottom = mainMenu ? panelBottom - 58.0f : size.y - 48.0f;
    const float itemStep = m_items.size() > 1 ? std::min(mainMenu ? 42.0f : 50.0f, (listBottom - firstY) / static_cast<float>(m_items.size() - 1)) : 50.0f;
    const float itemH = std::clamp(itemStep - (mainMenu ? 8.0f : 8.0f), mainMenu ? 30.0f : 30.0f, mainMenu ? 37.0f : 42.0f);
    const unsigned itemTextSize = mainMenu ? 16 : (m_items.size() > 7 ? 19 : 22);
    sf::Vector2f selectedCardPos{};
    float selectedCardW = itemW;
    float selectedCardH = itemH;
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        const bool selected = i == m_selected;
        const float y = firstY + i * itemStep;
        const float movePulse = selected ? std::max(0.0f, 1.0f - (now - m_selectionChangedAt) / 0.18f) : 0.0f;
        const float clickPulse = selected ? std::max(0.0f, 1.0f - (now - m_activatedAt) / 0.14f) : 0.0f;
        const float grow = selected ? 1.0f + 0.025f + 0.035f * easeOut(movePulse) - 0.025f * clickPulse : 1.0f;
        const float cardW = itemW * grow;
        const float cardH = itemH * grow;
        const sf::Vector2f cardPos(mainMenu ? std::round(panel.getPosition().x) : std::round(size.x * 0.5f),
                                   std::round(y + (mainMenu ? 3.0f * movePulse : 0.0f)));

        if (selected && mainMenu) {
            sf::RectangleShape glow({cardW + 16.0f, cardH + 12.0f});
            glow.setOrigin(glow.getSize().x * 0.5f, glow.getSize().y * 0.5f);
            glow.setPosition(cardPos);
            glow.setFillColor(sf::Color(255, 220, 92, static_cast<sf::Uint8>(52 + 62 * clickPulse)));
            window.draw(glow);
        }

        if (mainMenu) {
            const sf::Color tint = selected
                ? sf::Color(255, static_cast<sf::Uint8>(240 + 15 * clickPulse), static_cast<sf::Uint8>(190 + 40 * clickPulse))
                : sf::Color(232, 238, 255, 245);
            if (const sf::Texture* buttonTexture = assets.texture("menu_button")) {
                drawSpriteInRect(window, *buttonTexture, {cardPos.x - cardW * 0.5f, cardPos.y - cardH * 0.5f, cardW, cardH}, tint);
            } else {
                sf::RectangleShape card({cardW, cardH});
                card.setOrigin(cardW * 0.5f, cardH * 0.5f);
                card.setPosition(cardPos);
                card.setFillColor(selected ? sf::Color(230, 96, 48, static_cast<sf::Uint8>(238 + 17 * clickPulse)) : sf::Color(32, 72, 122, 218));
                card.setOutlineColor(selected ? sf::Color(255, 248, 150) : sf::Color(146, 184, 218, 178));
                card.setOutlineThickness(selected ? 3.0f : 1.0f);
                window.draw(card);
            }
        } else {
            sf::RectangleShape card({cardW, cardH});
            card.setOrigin(cardW * 0.5f, cardH * 0.5f);
            card.setPosition(cardPos);
            card.setFillColor(selected ? sf::Color(230, 96, 48, static_cast<sf::Uint8>(238 + 17 * clickPulse)) : sf::Color(240, 175, 69, 224));
            card.setOutlineColor(selected ? sf::Color(255, 248, 150) : sf::Color(122, 72, 38, 190));
            card.setOutlineThickness(selected ? 3.0f : 1.0f);
            window.draw(card);
        }
        drawText(m_items[static_cast<std::size_t>(i)], itemTextSize, {cardPos.x, cardPos.y + 1.0f}, sf::Color::White, true);
        if (selected) {
            selectedCardPos = cardPos;
            selectedCardW = cardW;
            selectedCardH = cardH;
        }
    }

    if (mainMenu && !m_items.empty()) {
        const float spin = std::abs(std::sin(time * 5.8f));
        const float coinSize = std::clamp(selectedCardH * 0.9f, 24.0f, 34.0f);
        const sf::Vector2f coinPos(selectedCardPos.x + selectedCardW * 0.5f + coinSize * 0.85f, selectedCardPos.y);
        if (const sf::Texture* coin = assets.texture("menu_coin")) {
            sf::Sprite sprite(*coin);
            sprite.setOrigin(coin->getSize().x * 0.5f, coin->getSize().y * 0.5f);
            sprite.setPosition(coinPos);
            sprite.setScale((0.42f + 0.58f * spin) * coinSize / static_cast<float>(coin->getSize().x),
                            coinSize / static_cast<float>(coin->getSize().y));
            sprite.setColor(sf::Color(255, static_cast<sf::Uint8>(210 + 45 * spin), static_cast<sf::Uint8>(135 + 60 * spin)));
            window.draw(sprite);
        } else {
            sf::CircleShape coinShape(coinSize * 0.5f, 28);
            coinShape.setOrigin(coinSize * 0.5f, coinSize * 0.5f);
            coinShape.setPosition(coinPos);
            coinShape.setScale(0.36f + 0.64f * spin, 1.0f);
            coinShape.setFillColor(sf::Color(255, static_cast<sf::Uint8>(194 + 48 * spin), 54));
            coinShape.setOutlineColor(sf::Color(112, 70, 20));
            coinShape.setOutlineThickness(2.0f);
            window.draw(coinShape);
        }
    }

    if (!mainMenu) {
        sf::RectangleShape ground({size.x, 34.0f});
        ground.setPosition(0.0f, size.y - 34.0f);
        ground.setFillColor(sf::Color(184, 91, 45));
        window.draw(ground);
    }
    drawText("W/S lub strzalki: wybor   ENTER: zatwierdz   ESC: powrot", mainMenu ? 13 : 14, {mainMenu ? panel.getPosition().x : size.x * 0.5f, mainMenu ? panelBottom - 18.0f : size.y - 19.0f}, sf::Color(255, 244, 190), true);
}

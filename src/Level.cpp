#include "Level.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace {
constexpr int Rows = 15;
constexpr int GroundRow = 13;

sf::Color withAlpha(sf::Color color, sf::Uint8 alpha)
{
    color.a = alpha;
    return color;
}

void drawRect(sf::RenderWindow& window, const sf::FloatRect& rect, sf::Color fill, sf::Color outline = sf::Color::Transparent)
{
    sf::RectangleShape shape({rect.width, rect.height});
    shape.setPosition(rect.left, rect.top);
    shape.setFillColor(fill);
    if (outline != sf::Color::Transparent) {
        shape.setOutlineColor(outline);
        shape.setOutlineThickness(2.0f);
    }
    window.draw(shape);
}

void drawCircle(sf::RenderWindow& window, sf::Vector2f pos, float radius, sf::Color color)
{
    sf::CircleShape shape(radius, 32);
    shape.setOrigin(radius, radius);
    shape.setPosition(pos);
    shape.setFillColor(color);
    window.draw(shape);
}

float smoothFade(float value, float edge0, float edge1)
{
    if (edge0 == edge1)
        return value >= edge1 ? 1.0f : 0.0f;
    const float t = std::clamp((value - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

sf::Uint8 castleBackdropAlpha(float x, float y, float visibility)
{
    const float leftFade = smoothFade(x, 0.0f, 0.14f);
    const float rightFade = smoothFade(1.0f - x, 0.0f, 0.14f);
    const float topFade = smoothFade(y, 0.0f, 0.18f);
    const float edgeFade = std::min({leftFade, rightFade, topFade, 1.0f});
    return static_cast<sf::Uint8>(std::clamp(visibility, 0.0f, 1.0f) * edgeFade * 242.0f);
}

void drawNightBlendQuad(sf::RenderWindow& window, const sf::FloatRect& rect, sf::Color leftColor, sf::Color rightColor)
{
    sf::VertexArray quad(sf::Quads, 4);
    quad[0].position = {rect.left, rect.top};
    quad[1].position = {rect.left + rect.width, rect.top};
    quad[2].position = {rect.left + rect.width, rect.top + rect.height};
    quad[3].position = {rect.left, rect.top + rect.height};
    quad[0].color = leftColor;
    quad[3].color = leftColor;
    quad[1].color = rightColor;
    quad[2].color = rightColor;
    window.draw(quad);
}

bool drawCastleExteriorAsset(sf::RenderWindow& window, const AssetManager& assets, float groundY, float visibility)
{
    const sf::Texture* texture = assets.texture("castle_exterior");
    if (!texture || texture->getSize().x == 0 || texture->getSize().y == 0)
        return false;

    const float dstW = 52.0f * Tile;
    const float scale = dstW / static_cast<float>(texture->getSize().x);
    const float dstH = static_cast<float>(texture->getSize().y) * scale;
    const sf::Vector2f pos(101.0f * Tile - dstW * 0.5f, groundY - dstH * 0.88f);
    const std::array<float, 7> xStops{0.0f, 0.06f, 0.14f, 0.50f, 0.86f, 0.94f, 1.0f};
    const std::array<float, 4> yStops{0.0f, 0.10f, 0.18f, 1.0f};
    const sf::Color tint(218, 222, 238);

    sf::VertexArray castle(sf::Quads);
    for (std::size_t yi = 0; yi + 1 < yStops.size(); ++yi) {
        for (std::size_t xi = 0; xi + 1 < xStops.size(); ++xi) {
            const float x0 = xStops[xi];
            const float x1 = xStops[xi + 1];
            const float y0 = yStops[yi];
            const float y1 = yStops[yi + 1];

            const std::array<sf::Vector2f, 4> positions{{
                {pos.x + dstW * x0, pos.y + dstH * y0},
                {pos.x + dstW * x1, pos.y + dstH * y0},
                {pos.x + dstW * x1, pos.y + dstH * y1},
                {pos.x + dstW * x0, pos.y + dstH * y1},
            }};
            const std::array<sf::Vector2f, 4> coords{{
                {texture->getSize().x * x0, texture->getSize().y * y0},
                {texture->getSize().x * x1, texture->getSize().y * y0},
                {texture->getSize().x * x1, texture->getSize().y * y1},
                {texture->getSize().x * x0, texture->getSize().y * y1},
            }};
            const std::array<sf::Uint8, 4> alpha{{
                castleBackdropAlpha(x0, y0, visibility),
                castleBackdropAlpha(x1, y0, visibility),
                castleBackdropAlpha(x1, y1, visibility),
                castleBackdropAlpha(x0, y1, visibility),
            }};

            for (int i = 0; i < 4; ++i) {
                sf::Color color = tint;
                color.a = alpha[i];
                castle.append({positions[i], color, coords[i]});
            }
        }
    }

    sf::RenderStates states(texture);
    window.draw(castle, states);

    const sf::Color mistOuter(18, 24, 42, static_cast<sf::Uint8>(42.0f * std::clamp(visibility, 0.0f, 1.0f)));
    const sf::Color mistInner(18, 24, 42, 0);
    drawNightBlendQuad(window, {pos.x - 12.0f, pos.y + dstH * 0.10f, dstW * 0.16f, dstH * 0.78f}, mistOuter, mistInner);
    drawNightBlendQuad(window, {pos.x + dstW * 0.84f, pos.y + dstH * 0.10f, dstW * 0.16f + 12.0f, dstH * 0.78f}, mistInner, mistOuter);

    sf::VertexArray baseFog(sf::Quads, 4);
    baseFog[0].position = {pos.x, groundY - 74.0f};
    baseFog[1].position = {pos.x + dstW, groundY - 74.0f};
    baseFog[2].position = {pos.x + dstW, groundY + 8.0f};
    baseFog[3].position = {pos.x, groundY + 8.0f};
    baseFog[0].color = withAlpha(sf::Color(16, 24, 34), 0);
    baseFog[1].color = withAlpha(sf::Color(16, 24, 34), 0);
    baseFog[2].color = withAlpha(sf::Color(16, 24, 34), static_cast<sf::Uint8>(58.0f * std::clamp(visibility, 0.0f, 1.0f)));
    baseFog[3].color = withAlpha(sf::Color(16, 24, 34), static_cast<sf::Uint8>(58.0f * std::clamp(visibility, 0.0f, 1.0f)));
    window.draw(baseFog);
    return true;
}

bool drawDungeonArenaAsset(sf::RenderWindow& window, const AssetManager& assets, float groundY)
{
    const sf::Texture* texture = assets.texture("castle_dungeon_arena");
    if (!texture || texture->getSize().x == 0 || texture->getSize().y == 0)
        return false;

    const float dstW = 54.0f * Tile;
    const float scale = dstW / static_cast<float>(texture->getSize().x);
    const float dstH = static_cast<float>(texture->getSize().y) * scale;
    sf::Sprite arena(*texture);
    arena.setPosition(92.0f * Tile, groundY - dstH * 0.74f);
    arena.setScale(scale, scale);
    window.draw(arena);
    return true;
}

bool drawFinalCastleLevelBackground(sf::RenderWindow& window, const AssetManager& assets, float leftWorld, sf::Vector2f center, sf::Vector2f size)
{
    const sf::Texture* texture = assets.texture("final_castle_background");
    if (!texture || texture->getSize().x == 0 || texture->getSize().y == 0)
        return false;

    const sf::Vector2u texSize = texture->getSize();
    const float scale = std::max(size.x / static_cast<float>(texSize.x), size.y / static_cast<float>(texSize.y));
    const float dstW = static_cast<float>(texSize.x) * scale;
    const float dstH = static_cast<float>(texSize.y) * scale;
    const float viewTop = center.y - size.y * 0.5f;

    sf::Sprite background(*texture);
    background.setScale(scale, scale);
    background.setPosition(leftWorld - (dstW - size.x) * 0.5f, viewTop + size.y - dstH);
    window.draw(background);
    return true;
}

bool drawCastleEntranceFacade(sf::RenderWindow& window, const AssetManager& assets, float groundY, float visibility)
{
    if (visibility <= 0.0f)
        return true;

    if (drawCastleExteriorAsset(window, assets, groundY, visibility))
        return true;

    const sf::Uint8 a = static_cast<sf::Uint8>(std::clamp(visibility, 0.0f, 1.0f) * 235.0f);
    const float x = 101.0f * Tile;
    const float y = groundY - 254.0f;
    const sf::Color outline = withAlpha(sf::Color(24, 28, 38), a);
    const sf::Color shadow = withAlpha(sf::Color(38, 45, 58), a);
    const sf::Color wall = withAlpha(sf::Color(82, 88, 100), a);
    const sf::Color wallLight = withAlpha(sf::Color(126, 124, 118), static_cast<sf::Uint8>(a * 0.88f));
    const sf::Color roof = withAlpha(sf::Color(45, 54, 70), a);
    const sf::Color windowColor = withAlpha(sf::Color(30, 38, 52), a);

    auto drawBattlements = [&](float left, float top, int count, float blockW, sf::Color color) {
        for (int i = 0; i < count; ++i)
            drawRect(window, {left + i * blockW * 1.55f, top, blockW, 16.0f}, color, outline);
    };

    auto drawMasonry = [&](sf::FloatRect area, sf::Color lineColor) {
        for (float yy = area.top + 24.0f; yy < area.top + area.height - 8.0f; yy += 24.0f)
            drawRect(window, {area.left + 6.0f, yy, area.width - 12.0f, 2.0f}, lineColor);
        int row = 0;
        for (float yy = area.top + 8.0f; yy < area.top + area.height - 12.0f; yy += 24.0f) {
            const float start = area.left + (row % 2 == 0 ? 22.0f : 6.0f);
            for (float xx = start; xx < area.left + area.width - 8.0f; xx += 42.0f)
                drawRect(window, {xx, yy, 2.0f, 18.0f}, lineColor);
            ++row;
        }
    };

    auto drawWindow = [&](float wx, float wy, float w = 14.0f, float h = 24.0f) {
        drawRect(window, {wx, wy + 7.0f, w, h - 7.0f}, windowColor, outline);
        drawCircle(window, {wx + w * 0.5f, wy + 7.0f}, w * 0.5f, windowColor);
        drawRect(window, {wx + w * 0.5f - 1.0f, wy + 3.0f, 2.0f, h - 2.0f}, withAlpha(sf::Color(84, 94, 108), static_cast<sf::Uint8>(a * 0.7f)));
    };

    sf::ConvexShape distantPeak(3);
    distantPeak.setPoint(0, {x - 360.0f, groundY});
    distantPeak.setPoint(1, {x - 210.0f, groundY - 180.0f});
    distantPeak.setPoint(2, {x - 60.0f, groundY});
    distantPeak.setFillColor(withAlpha(sf::Color(125, 188, 194), static_cast<sf::Uint8>(a * 0.28f)));
    window.draw(distantPeak);

    drawRect(window, {x - 290.0f, y + 188.0f, 580.0f, 64.0f}, shadow, outline);
    drawBattlements(x - 278.0f, y + 168.0f, 12, 24.0f, wall);
    drawMasonry({x - 290.0f, y + 188.0f, 580.0f, 64.0f}, withAlpha(sf::Color(34, 38, 48), static_cast<sf::Uint8>(a * 0.9f)));

    drawRect(window, {x - 184.0f, y + 92.0f, 368.0f, 160.0f}, wall, outline);
    drawMasonry({x - 184.0f, y + 92.0f, 368.0f, 160.0f}, withAlpha(sf::Color(40, 43, 51), static_cast<sf::Uint8>(a * 0.85f)));
    drawBattlements(x - 168.0f, y + 72.0f, 8, 26.0f, wallLight);

    for (float tx : {-248.0f, -166.0f, 166.0f, 248.0f}) {
        const float towerTop = tx == -248.0f || tx == 248.0f ? y + 78.0f : y + 32.0f;
        const float towerH = groundY - towerTop;
        drawRect(window, {x + tx - 31.0f, towerTop, 62.0f, towerH}, wall, outline);
        drawMasonry({x + tx - 31.0f, towerTop, 62.0f, towerH}, withAlpha(sf::Color(42, 46, 54), static_cast<sf::Uint8>(a * 0.8f)));
        drawBattlements(x + tx - 35.0f, towerTop - 18.0f, 3, 16.0f, wallLight);
        sf::ConvexShape cap(3);
        cap.setPoint(0, {x + tx - 38.0f, towerTop - 18.0f});
        cap.setPoint(1, {x + tx, towerTop - 58.0f});
        cap.setPoint(2, {x + tx + 38.0f, towerTop - 18.0f});
        cap.setFillColor(roof);
        window.draw(cap);
        drawWindow(x + tx - 8.0f, towerTop + 36.0f, 16.0f, 28.0f);
        drawWindow(x + tx - 8.0f, towerTop + 96.0f, 16.0f, 28.0f);
    }

    drawRect(window, {x - 76.0f, y + 12.0f, 152.0f, 240.0f}, withAlpha(sf::Color(98, 98, 106), a), outline);
    drawMasonry({x - 76.0f, y + 12.0f, 152.0f, 240.0f}, withAlpha(sf::Color(44, 47, 56), static_cast<sf::Uint8>(a * 0.82f)));
    drawBattlements(x - 62.0f, y - 8.0f, 5, 18.0f, wallLight);
    sf::ConvexShape keepRoof(3);
    keepRoof.setPoint(0, {x - 90.0f, y - 8.0f});
    keepRoof.setPoint(1, {x, y - 76.0f});
    keepRoof.setPoint(2, {x + 90.0f, y - 8.0f});
    keepRoof.setFillColor(roof);
    window.draw(keepRoof);
    drawRect(window, {x - 8.0f, y - 96.0f, 16.0f, 28.0f}, roof, outline);
    drawRect(window, {x + 7.0f, y - 88.0f, 32.0f, 8.0f}, withAlpha(sf::Color(150, 55, 60), a));

    drawWindow(x - 42.0f, y + 48.0f, 17.0f, 32.0f);
    drawWindow(x + 25.0f, y + 48.0f, 17.0f, 32.0f);
    drawWindow(x - 8.0f, y + 104.0f, 16.0f, 32.0f);

    drawRect(window, {x - 48.0f, groundY - 86.0f, 96.0f, 86.0f}, withAlpha(sf::Color(24, 25, 31), a), outline);
    drawCircle(window, {x, groundY - 86.0f}, 48.0f, withAlpha(sf::Color(24, 25, 31), a));
    drawRect(window, {x - 32.0f, groundY - 68.0f, 64.0f, 68.0f}, withAlpha(sf::Color(104, 64, 42), a), outline);
    drawRect(window, {x - 2.0f, groundY - 68.0f, 4.0f, 68.0f}, withAlpha(sf::Color(54, 31, 24), a));
    for (int i = 0; i < 4; ++i)
        drawRect(window, {x - 28.0f, groundY - 56.0f + i * 16.0f, 56.0f, 3.0f}, withAlpha(sf::Color(56, 34, 26), a));
    return true;
}

void drawCastleInteriorBackdrop(sf::RenderWindow& window, const AssetManager& assets)
{
    const float left = 92.0f * Tile;
    const float top = 0.0f;
    const float width = 54.0f * Tile;
    const float height = 15.0f * Tile;

    if (drawDungeonArenaAsset(window, assets, GroundRow * Tile))
        return;

    drawRect(window, {left, top, width, height}, sf::Color(13, 16, 24));
    drawRect(window, {left, top + 4.2f * Tile, width, 5.4f * Tile}, sf::Color(26, 29, 38));
    drawRect(window, {left, top + 9.6f * Tile, width, 3.4f * Tile}, sf::Color(20, 21, 28));

    for (int row = 0; row < 15; ++row) {
        for (int col = 92; col <= 145; ++col) {
            const float x = col * Tile;
            const float y = row * Tile;
            const bool offset = row % 2 == 1;
            if ((col + row) % 2 == 0)
                drawRect(window, {x + (offset ? 8.0f : 0.0f), y + 15.0f, 22.0f, 2.0f}, sf::Color(43, 45, 52, 135));
            if ((col + row) % 5 == 0)
                drawRect(window, {x + 4.0f, y + 4.0f, 2.0f, 12.0f}, sf::Color(38, 40, 48, 105));
        }
    }

    for (float x : {104.0f * Tile, 125.0f * Tile, 140.0f * Tile}) {
        drawRect(window, {x, 2.1f * Tile, 2.0f * Tile, 3.4f * Tile}, sf::Color(9, 12, 18), sf::Color(58, 58, 62));
        drawCircle(window, {x + Tile, 2.1f * Tile}, Tile, sf::Color(9, 12, 18));
        drawRect(window, {x + 0.9f * Tile, 2.0f * Tile, 0.2f * Tile, 3.5f * Tile}, sf::Color(52, 58, 70));
        drawRect(window, {x + 0.18f * Tile, 3.55f * Tile, 1.64f * Tile, 0.18f * Tile}, sf::Color(52, 58, 70));
    }

    for (float x : {99.0f * Tile, 112.0f * Tile, 134.0f * Tile}) {
        drawRect(window, {x, 5.0f * Tile, 5.2f * Tile, 2.2f * Tile}, sf::Color(48, 31, 26), sf::Color(18, 14, 14));
        drawRect(window, {x + 0.32f * Tile, 5.32f * Tile, 4.56f * Tile, 0.22f * Tile}, sf::Color(135, 84, 48));
        drawRect(window, {x + 0.32f * Tile, 6.18f * Tile, 4.56f * Tile, 0.22f * Tile}, sf::Color(135, 84, 48));
        for (int i = 0; i < 5; ++i) {
            const sf::Color book = i % 3 == 0 ? sf::Color(150, 50, 56) : i % 3 == 1 ? sf::Color(55, 92, 138) : sf::Color(192, 152, 72);
            drawRect(window, {x + 16.0f + i * 20.0f, 5.56f * Tile, 10.0f, 18.0f}, book);
            drawRect(window, {x + 20.0f + i * 18.0f, 6.42f * Tile, 9.0f, 16.0f}, book);
        }
    }

    for (float x : {101.0f * Tile, 124.0f * Tile, 143.0f * Tile}) {
        for (int i = 0; i < 5; ++i) {
            sf::CircleShape link(5.0f, 12);
            link.setOrigin(5.0f, 5.0f);
            link.setPosition(x, 0.7f * Tile + i * 14.0f);
            link.setFillColor(sf::Color::Transparent);
            link.setOutlineColor(sf::Color(125, 116, 102));
            link.setOutlineThickness(2.0f);
            window.draw(link);
        }
    }

    for (float x : {108.0f * Tile, 139.0f * Tile}) {
        drawRect(window, {x + 13.0f, 6.8f * Tile, 6.0f, 16.0f}, sf::Color(72, 46, 34));
        sf::ConvexShape flame(4);
        flame.setPoint(0, {x + 16.0f, 6.28f * Tile});
        flame.setPoint(1, {x + 26.0f, 6.62f * Tile});
        flame.setPoint(2, {x + 16.0f, 6.96f * Tile});
        flame.setPoint(3, {x + 6.0f, 6.62f * Tile});
        flame.setFillColor(sf::Color(255, 126, 44));
        window.draw(flame);
        drawCircle(window, {x + 16.0f, 6.62f * Tile}, 5.0f, sf::Color(255, 234, 120));
    }

    sf::VertexArray glow(sf::Quads, 4);
    glow[0].position = {left, 6.3f * Tile};
    glow[1].position = {left + width, 6.3f * Tile};
    glow[2].position = {left + width, 10.5f * Tile};
    glow[3].position = {left, 10.5f * Tile};
    glow[0].color = sf::Color(255, 144, 74, 22);
    glow[1].color = sf::Color(255, 144, 74, 22);
    glow[2].color = sf::Color(255, 144, 74, 0);
    glow[3].color = sf::Color(255, 144, 74, 0);
    window.draw(glow);
}

}

void Level::build(int number)
{
    m_number = std::clamp(number, 1, 5);
    switch (m_number) {
    case 1:
        buildLevel1();
        break;
    case 2:
        buildLevel2();
        break;
    case 3:
        buildLevel3();
        break;
    case 4:
        buildLevel4();
        break;
    default:
        buildLevel5();
        break;
    }
}

void Level::buildLevel1()
{
    clear(186);
    m_start = {3.0f * Tile, 9.0f * Tile};
    m_weather = WeatherType::Clear;
    m_skyTop = sf::Color(92, 184, 232);
    m_skyBottom = sf::Color(184, 224, 248);
    m_headline = "Wczoraj bohater uratowal pierwsza wioske.";
    baseGround({{46, 49}, {96, 99}, {144, 147}});
    placeFlag(174);

    fill(9, 11, 15, 'B');
    setTile(9, 16, '?');
    setTile(9, 22, '?');
    fill(8, 30, 36, 'B');
    setTile(7, 33, '?');
    placePipe(38, 2, 2, {-1.0f, -1.0f}, true);
    fill(9, 55, 61, 'B');
    setTile(8, 59, '?');
    placePipe(72, 2, 3, {-1.0f, -1.0f}, true);
    fill(8, 82, 88, 'B');
    setTile(7, 85, '?');
    fill(9, 111, 117, 'B');
    setTile(8, 115, '?');
    placePipe(126, 3, 3, {-1.0f, -1.0f}, true);
    fill(8, 137, 143, 'B');
    setTile(7, 140, '?');
    placeStairs(160, 8, 1);

    fill(12, 82, 84, '^');
    fill(12, 134, 136, '^');
    setTile(12, 104, 'C');
    addCoins(7, 10, 23, 2);
    addCoins(6, 31, 43, 2);
    addCoins(6, 56, 70, 2);
    addCoins(7, 81, 93, 2);
    addCoins(6, 112, 126, 2);
    addCoins(6, 137, 153, 2);
    addItem("mushroom", 28, 12);
    addItem("star", 150, 11);
    addEnemy("goomba", 23, 13);
    addEnemy("goomba", 54, 13);
    addEnemy("goomba", 90, 13);
    addEnemy("runner", 118, 13);
    addEnemy("koopa", 151, 13);
}

void Level::buildLevel2()
{
    clear(236);
    m_start = {3.0f * Tile, 9.0f * Tile};
    m_weather = WeatherType::Clear;
    m_skyTop = sf::Color(108, 194, 238);
    m_skyBottom = sf::Color(199, 234, 250);
    m_headline = "Gazeta: w rurach znaleziono sekretna komnate z kluczem.";
    baseGround({{88, 91}, {151, 153}});
    placeFlag(178);

    fill(9, 12, 18, 'B');
    setTile(8, 18, '?');
    fill(7, 28, 34, 'B');
    setTile(7, 31, '?');
    fill(9, 48, 54, 'B');
    fill(8, 82, 92, 'B');
    setTile(8, 89, '?');
    fill(9, 112, 120, 'B');
    setTile(8, 116, '?');
    fill(7, 142, 150, 'B');
    setTile(6, 146, '?');
    placePipe(26, 2, 2, {-1.0f, -1.0f}, true);
    placePipe(35, 2, 3, {210.0f * Tile, 10.0f * Tile});
    placePipe(60, 3, 3, {-1.0f, -1.0f}, true);
    placePipe(101, 2, 4, {-1.0f, -1.0f}, true);
    placePipe(132, 2, 2);
    placePipe(158, 3, 3, {-1.0f, -1.0f}, true);
    placeStairs(165, 7, 1);
    addSecretRoom(206, {39.0f * Tile, 10.0f * Tile}, true);

    addCoins(7, 11, 24, 2);
    addCoins(5, 29, 43, 2);
    addCoins(7, 84, 100, 2);
    addCoins(6, 106, 128, 2);
    addCoins(5, 142, 164, 2);
    addItem("mushroom", 52, 12);
    addEnemy("goomba", 22, 13);
    addEnemy("koopa", 68, 13);
    addEnemy("goomba", 92, 13);
    addEnemy("shooter", 102, 9);
    addEnemy("runner", 125, 13);
    addEnemy("koopa", 156, 13);
    addEnemy("shooter", 160, 10);
}

void Level::buildLevel3()
{
    clear(232);
    m_start = {3.0f * Tile, 9.0f * Tile};
    m_weather = WeatherType::Clear;
    m_skyTop = sf::Color(88, 178, 230);
    m_skyBottom = sf::Color(183, 223, 246);
    m_headline = "Gazeta: platformy nad dolina prowadza do sekretnej sciezki.";
    baseGround({{25, 30}, {58, 64}, {98, 105}, {142, 148}, {180, 184}});
    placeFlag(220);

    fill(9, 10, 14, 'B');
    setTile(9, 15, '?');
    fill(7, 24, 30, 'B');
    fill(6, 45, 51, 'B');
    setTile(5, 49, '?');
    fill(8, 73, 78, 'B');
    fill(6, 91, 98, 'B');
    setTile(6, 94, '?');
    fill(5, 128, 145, 'B');
    setTile(4, 137, '?');
    fill(8, 162, 170, 'B');
    setTile(7, 166, '?');
    fill(7, 194, 201, 'B');
    setTile(6, 198, '?');
    placePipe(112, 2, 2, {-1.0f, -1.0f}, true);
    placePipe(188, 2, 2, {-1.0f, -1.0f}, true);
    placeStairs(207, 7, 1);

    fill(12, 41, 44, '^');
    fill(12, 82, 85, '^');
    fill(12, 154, 156, '^');
    setTile(12, 67, 'T');
    setTile(12, 108, 'C');
    addCoins(6, 25, 39, 2);
    addCoins(4, 46, 62, 2);
    addCoins(5, 89, 106, 2);
    addCoins(4, 128, 150, 2);
    addCoins(7, 162, 180, 2);
    addCoins(6, 194, 212, 2);
    addItem("star", 50, 4);
    addItem("mushroom", 166, 7);
    addEnemy("goomba", 20, 13);
    addEnemy("koopa", 53, 13);
    addEnemy("goomba", 80, 13);
    addEnemy("flyer", 96, 9);
    addEnemy("flyer", 136, 7);
    addEnemy("runner", 160, 13);
    addEnemy("shooter", 190, 9);
    addEnemy("spiny", 205, 13);
    m_platforms.push_back({{36.0f * Tile, 8.2f * Tile, 3.5f * Tile, 16.0f}, {}, 33.0f * Tile, 43.0f * Tile, 100.0f, 1, false});
    m_platforms.push_back({{66.0f * Tile, 7.0f * Tile, 3.2f * Tile, 16.0f}, {}, 63.0f * Tile, 72.0f * Tile, 115.0f, -1, false});
    m_platforms.push_back({{104.0f * Tile, 6.7f * Tile, 3.0f * Tile, 16.0f}, {}, 100.0f * Tile, 116.0f * Tile, 120.0f, 1, false});
    m_platforms.push_back({{151.0f * Tile, 7.4f * Tile, 3.2f * Tile, 16.0f}, {}, 149.0f * Tile, 160.0f * Tile, 110.0f, -1, false});
    m_platforms.push_back({{183.0f * Tile, 6.3f * Tile, 3.0f * Tile, 16.0f}, {}, 180.0f * Tile, 192.0f * Tile, 130.0f, 1, true});
}

void Level::buildLevel4()
{
    clear(258);
    m_start = {3.0f * Tile, 9.0f * Tile};
    m_weather = WeatherType::Clear;
    m_skyTop = sf::Color(90, 104, 146);
    m_skyBottom = sf::Color(164, 162, 176);
    m_headline = "Gazeta: zamek zamknal brame, ale klucz czeka w rurze.";
    baseGround({{37, 42}, {77, 82}, {132, 137}, {183, 187}});
    placeFlag(206);

    fill(10, 8, 18, 'B');
    fill(8, 22, 30, 'B');
    setTile(7, 27, '?');
    fill(7, 46, 55, 'B');
    fill(9, 68, 76, 'B');
    fill(6, 93, 102, 'B');
    setTile(6, 98, '?');
    fill(8, 128, 136, 'B');
    fill(7, 168, 176, 'B');
    setTile(6, 172, '?');
    fill(8, 192, 201, 'B');
    placePipe(55, 2, 3, {234.0f * Tile, 10.0f * Tile});
    addSecretRoom(230, {59.0f * Tile, 10.0f * Tile}, true);
    placePipe(107, 2, 2, {-1.0f, -1.0f}, true);
    placePipe(146, 2, 3, {-1.0f, -1.0f}, true);
    placeStairs(194, 8, 1);

    fill(14, 38, 42, '~');
    fill(14, 78, 82, '~');
    fill(14, 133, 137, '~');
    fill(14, 184, 187, '~');
    fill(12, 31, 34, '^');
    fill(12, 89, 92, '^');
    fill(12, 151, 154, '^');
    setTile(8, 112, '^');
    fill(9, 112, 112, 'B');
    for (int row = 9; row <= 12; ++row)
        setTile(row, 158, 'L');
    setTile(12, 66, 'C');

    addCoins(7, 12, 30, 2);
    addCoins(5, 47, 62, 2);
    addCoins(5, 92, 110, 2);
    addCoins(7, 128, 146, 2);
    addCoins(5, 168, 188, 2);
    addCoins(6, 192, 204, 2);
    addItem("mushroom", 99, 5);
    addItem("star", 172, 6);
    addEnemy("goomba", 26, 13);
    addEnemy("koopa", 63, 13);
    addEnemy("shooter", 88, 9);
    addEnemy("spiny", 118, 13);
    addEnemy("koopa", 146, 13);
    addEnemy("runner", 166, 13);
    addEnemy("flyer", 186, 8);
    addEnemy("shooter", 198, 9);
}

void Level::buildLevel5()
{
    clear(166);
    m_start = {3.0f * Tile, 9.0f * Tile};
    m_weather = WeatherType::Clear;
    m_skyTop = sf::Color(84, 124, 180);
    m_skyBottom = sf::Color(176, 198, 226);
    m_headline = "Gazeta: Bowser pokonany, droga do flagi zostala otwarta.";
    baseGround({{25, 29}, {55, 60}, {85, 88}});
    placeFlag(154);

    fill(9, 10, 16, 'B');
    setTile(9, 17, '?');
    fill(8, 36, 43, 'B');
    setTile(7, 41, '?');
    fill(8, 65, 73, 'B');
    placePipe(47, 2, 3, {-1.0f, -1.0f}, true);
    fill(12, 77, 80, '^');
    fill(12, 91, 94, '^');
    fill(9, 94, 102, 'B');
    setTile(8, 98, '?');
    fill(7, 113, 119, 'B');
    setTile(6, 116, '?');
    for (int row = 8; row <= 12; ++row)
        setTile(row, 144, 'L');

    for (int col = 90; col <= 145; ++col) {
        if (tileAt(13, col) == 'G')
            setTile(13, col, 'K');
        if (tileAt(14, col) == 'D')
            setTile(14, col, 'M');
    }
    for (int row = 3; row <= 12; ++row) {
        for (int col = 92; col <= 145; ++col) {
            if (tileAt(row, col) == '.')
                setTile(row, col, 'W');
        }
    }
    for (int col = 94; col <= 102; ++col)
        setTile(9, col, 'W');
    setTile(8, 98, 'W');
    for (int row = 4; row <= 12; ++row) {
        for (int col : {99, 103, 142, 145}) {
            if (tileAt(row, col) == 'W' || tileAt(row, col) == '.')
                setTile(row, col, 'I');
        }
    }
    for (int col : {104, 141})
        setTile(7, col, 'O');
    for (int row = 4; row <= 6; ++row) {
        setTile(row, 110, 'Y');
        setTile(row, 111, 'Y');
        setTile(row, 136, 'Y');
        setTile(row, 137, 'Y');
    }
    for (int col = 116; col <= 121; ++col)
        setTile(5, col, 'R');
    for (int col = 132; col <= 136; ++col)
        setTile(6, col, 'R');
    for (int col : {107, 108, 139, 140})
        setTile(4, col, 'Z');
    for (int row = 3; row <= 7; ++row) {
        setTile(row, 122, 'V');
        setTile(row, 129, 'V');
    }

    addCoins(7, 10, 22, 2);
    addCoins(6, 36, 50, 2);
    addCoins(7, 66, 82, 2);
    addCoins(6, 112, 126, 2);
    addItem("mushroom", 69, 7);
    addItem("star", 116, 6);
    addEnemy("goomba", 18, 13);
    addEnemy("koopa", 51, 13);
    addEnemy("runner", 72, 13);
    addEnemy("spiny", 91, 13);
    addEnemy("shooter", 120, 9);
    addEnemy("boss", 130, 13);
}

void Level::generateBonusRoom(int seed)
{
    const int start = 12 + (seed % 10);
    for (int col = start; col < start + 10; ++col)
        addItem("coin", col, 5 + col % 3);
    addItem("chest", start + 5, 9, 3);
}

void Level::update(float dt)
{
    for (auto& platform : m_platforms) {
        platform.previous = {platform.rect.left, platform.rect.top};
        platform.rect.left += platform.direction * platform.speed * dt;
        if (platform.rect.left < platform.minX) {
            platform.rect.left = platform.minX;
            platform.direction = 1;
        } else if (platform.rect.left > platform.maxX) {
            platform.rect.left = platform.maxX;
            platform.direction = -1;
        }
        if (platform.disappearing) {
            platform.timer += dt;
            platform.visible = std::fmod(platform.timer, 4.0f) < 2.75f;
        }
    }
}

void Level::draw(sf::RenderWindow& window, const AssetManager& assets, WorldMode world, bool secretsRevealed, float time, bool castleInterior) const
{
    const sf::View view = window.getView();
    const sf::Vector2f center = view.getCenter();
    const sf::Vector2f size = view.getSize();
    const float leftWorld = center.x - size.x * 0.5f;
    const float rightWorld = center.x + size.x * 0.5f;
    const float groundY = GroundRow * Tile;
    const bool dungeonInterior = m_number == 5 && castleInterior;
    const bool finalBackdrop = m_number == 5 && !dungeonInterior && drawFinalCastleLevelBackground(window, assets, leftWorld, center, size);

    if (dungeonInterior)
        drawCastleInteriorBackdrop(window, assets);

    if (!finalBackdrop && !dungeonInterior) {
        sf::VertexArray sky(sf::Quads, 4);
        sky[0].position = {leftWorld, center.y - size.y * 0.5f};
        sky[1].position = {rightWorld, center.y - size.y * 0.5f};
        sky[2].position = {rightWorld, center.y + size.y * 0.5f};
        sky[3].position = {leftWorld, center.y + size.y * 0.5f};
        sky[0].color = m_skyTop;
        sky[1].color = m_skyTop;
        sky[2].color = m_skyBottom;
        sky[3].color = m_skyBottom;
        window.draw(sky);

        const float mountainOffset = -std::fmod(leftWorld * 0.08f, 360.0f);
        for (int i = -1; i < 7; ++i) {
            const float x = leftWorld + mountainOffset + i * 360.0f;
            sf::ConvexShape mountain(3);
            mountain.setPoint(0, {x, groundY});
            mountain.setPoint(1, {x + 150.0f, groundY - 128.0f});
            mountain.setPoint(2, {x + 300.0f, groundY});
            mountain.setFillColor(sf::Color(98, 178, 94, 150));
            window.draw(mountain);
            sf::ConvexShape cap(3);
            cap.setPoint(0, {x + 124.0f, groundY - 104.0f});
            cap.setPoint(1, {x + 150.0f, groundY - 128.0f});
            cap.setPoint(2, {x + 178.0f, groundY - 104.0f});
            cap.setFillColor(sf::Color(238, 248, 238, 120));
            window.draw(cap);
        }
    }

    if (m_number == 5 && !dungeonInterior) {
        const float castleVisibility = std::clamp((rightWorld - 72.0f * Tile) / (24.0f * Tile), 0.0f, 1.0f);
        drawCastleEntranceFacade(window, assets, groundY, castleVisibility);
    }

    if (!finalBackdrop && !dungeonInterior) {
        const float bushOffset = -std::fmod(leftWorld * 0.18f, 220.0f);
        for (int i = -1; i < 10; ++i) {
            const float x = leftWorld + bushOffset + i * 220.0f;
            const sf::Color bush(72, 178, 68, 170);
            drawCircle(window, {x + 24.0f, groundY - 16.0f}, 18.0f, bush);
            drawCircle(window, {x + 45.0f, groundY - 24.0f}, 24.0f, bush);
            drawCircle(window, {x + 72.0f, groundY - 16.0f}, 18.0f, bush);
            drawRect(window, {x + 17.0f, groundY - 16.0f, 66.0f, 16.0f}, bush);
        }

        const float cloudOffset = -std::fmod(leftWorld * 0.28f, 360.0f);
        for (int i = -1; i < 8; ++i) {
            const float x = leftWorld + cloudOffset + i * 360.0f;
            const float y = 72.0f + (i % 3) * 12.0f;
            const sf::Color cloud = withAlpha(sf::Color::White, 210);
            drawCircle(window, {x + 44.0f, y + 12.0f}, 18.0f, cloud);
            drawCircle(window, {x + 70.0f, y}, 25.0f, cloud);
            drawCircle(window, {x + 103.0f, y + 13.0f}, 20.0f, cloud);
            drawRect(window, {x + 42.0f, y + 13.0f, 66.0f, 16.0f}, cloud);
        }
    }

    const int firstCol = std::max(0, static_cast<int>(std::floor(leftWorld / Tile)) - 2);
    const int lastCol = std::min(m_columns - 1, static_cast<int>(std::ceil(rightWorld / Tile)) + 2);
    for (int row = 0; row < Rows; ++row) {
        for (int col = firstCol; col <= lastCol; ++col) {
            const char tile = tileAt(row, col);
            if (tile != '.')
                drawTile(window, assets, tile, tileRect(row, col), world, secretsRevealed, time);
        }
    }

    for (const auto& platform : m_platforms) {
        if (!platform.visible)
            continue;
        // Rysujemy dokladnie na hitboxie - bez wychodzacego na zewnatrz obrysu,
        // ktory wczesniej przesuwal wizualnie deske o 2 px ponad powierzchnie kolizji.
        const sf::FloatRect r = platform.rect;
        const sf::Color body = platform.disappearing ? sf::Color(216, 138, 72) : sf::Color(190, 92, 49);
        drawRect(window, r, body);
        drawRect(window, {r.left, r.top, r.width, 3.0f}, sf::Color(232, 170, 110));            // jasna gorna krawedz
        drawRect(window, {r.left, rectBottom(r) - 3.0f, r.width, 3.0f}, sf::Color(82, 45, 34)); // ciemny spod
        drawRect(window, {r.left, r.top + r.height * 0.5f - 1.0f, r.width, 2.0f}, sf::Color(112, 55, 38)); // sloj na srodku, pelna szerokosc
    }
}

char Level::tileAt(int row, int col) const
{
    if (row < 0 || row >= static_cast<int>(m_tiles.size()) || col < 0 || col >= m_columns)
        return '.';
    return m_tiles[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
}

void Level::setTile(int row, int col, char tile)
{
    if (row < 0 || row >= static_cast<int>(m_tiles.size()) || col < 0 || col >= m_columns)
        return;
    m_tiles[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = tile;
}

sf::FloatRect Level::tileRect(int row, int col) const
{
    return {col * Tile, row * Tile, Tile, Tile};
}

bool Level::isSolidTile(char tile, WorldMode world, bool secretsRevealed) const
{
    if (tile == 'S')
        return world == WorldMode::Normal && !secretsRevealed;
    switch (tile) {
    case 'G':
    case 'D':
    case 'K':
    case 'M':
    case 'B':
    case '?':
    case 'U':
    case 'P':
    case 'p':
    case 'T':
    case 'L':
        return true;
    default:
        return false;
    }
}

bool Level::isHazardTile(char tile) const
{
    return tile == '^' || tile == '~';
}

bool Level::isTrampoline(char tile) const
{
    return tile == 'T';
}

bool Level::hitBlock(int row, int col, std::vector<SpawnRequest>& spawns)
{
    const char tile = tileAt(row, col);
    if (tile == '?') {
        setTile(row, col, 'U');
        const int selector = (row + col + m_number) % 5;
        const std::string type = selector == 0 ? "mushroom" : selector == 1 ? "star" : selector == 2 ? "fireflower" : "coin";
        spawns.push_back({type, {col * Tile + 10.0f, row * Tile - 30.0f}, selector});
        return true;
    }
    if (tile == 'B') {
        setTile(row, col, '.');
        spawns.push_back({"burst", {col * Tile + 24.0f, row * Tile + 24.0f}, 0});
        return true;
    }
    return false;
}

int Level::number() const { return m_number; }
int Level::columns() const { return m_columns; }
float Level::width() const { return m_columns * Tile; }
float Level::height() const { return Rows * Tile; }
WeatherType Level::weather() const { return m_weather; }
sf::Vector2f Level::playerStart() const { return m_start; }
sf::Vector2f Level::flagPosition() const { return m_flag; }
const std::string& Level::headline() const { return m_headline; }
const std::vector<SpawnRequest>& Level::enemySpawns() const { return m_enemySpawns; }
const std::vector<SpawnRequest>& Level::itemSpawns() const { return m_itemSpawns; }
const std::vector<TeleportPipe>& Level::teleports() const { return m_teleports; }
std::vector<MovingPlatform>& Level::platforms() { return m_platforms; }
const std::vector<MovingPlatform>& Level::platforms() const { return m_platforms; }

void Level::clear(int columns)
{
    m_columns = columns;
    m_tiles.assign(Rows, std::string(static_cast<std::size_t>(m_columns), '.'));
    m_platforms.clear();
    m_teleports.clear();
    m_enemySpawns.clear();
    m_itemSpawns.clear();
}

void Level::placeFlag(int col)
{
    m_flag = {col * Tile, 6.0f * Tile};
    setTile(6, col, 'F');
    for (int row = 7; row <= 12; ++row)
        setTile(row, col, 'f');
}

void Level::addSecretRoom(int startCol, sf::Vector2f returnTarget, bool keyRoom)
{
    const int endCol = startCol + 18;
    fill(7, startCol, endCol, 'B');
    for (int col = startCol; col <= endCol; ++col) {
        setTile(13, col, 'G');
        setTile(14, col, 'D');
    }
    for (int row = 8; row <= 12; ++row) {
        setTile(row, startCol, 'B');
        setTile(row, endCol, 'B');
    }
    fill(10, startCol + 3, startCol + 7, 'B');
    setTile(9, startCol + 5, '?');
    addCoins(9, startCol + 2, startCol + 8, 1);
    addCoins(11, startCol + 10, startCol + 14, 1);
    addItem(keyRoom ? "key" : "mushroom", startCol + 9, 12);
    addItem("star", startCol + 12, 11);
    placePipe(startCol + 15, 2, 2, returnTarget);
}

void Level::baseGround(const std::vector<std::pair<int, int>>& pits)
{
    auto inPit = [&pits](int col) {
        for (const auto& pit : pits) {
            if (col >= pit.first && col <= pit.second)
                return true;
        }
        return false;
    };

    for (int col = 0; col < m_columns; ++col) {
        if (inPit(col)) {
            setTile(GroundRow + 1, col, '~');
            continue;
        }
        setTile(GroundRow, col, 'G');
        setTile(GroundRow + 1, col, 'D');
    }
}

void Level::fill(int row, int from, int to, char tile)
{
    for (int col = from; col <= to; ++col)
        setTile(row, col, tile);
}

void Level::placePipe(int col, int width, int height, sf::Vector2f target, bool plant)
{
    const int top = GroundRow - height;
    for (int x = col; x < col + width; ++x) {
        setTile(top, x, 'P');
        for (int y = top + 1; y < GroundRow; ++y)
            setTile(y, x, 'p');
    }
    if (target.x >= 0.0f && target.y >= 0.0f)
        m_teleports.push_back({{col * Tile, top * Tile, width * Tile, height * Tile}, target});
    if (plant) {
        // gardziel rury: srodek u gory, skad wynurza sie roslina
        m_enemySpawns.push_back({"piranha", {(col + width * 0.5f) * Tile, top * Tile}, 0});
    }
}

void Level::placeStairs(int startCol, int steps, int direction)
{
    for (int step = 0; step < steps; ++step) {
        const int col = startCol + step * direction;
        const int height = step + 1;
        for (int row = GroundRow; row > GroundRow - height; --row)
            setTile(row, col, row == GroundRow - height + 1 ? 'G' : 'D');
    }
}

void Level::addCoins(int row, int from, int to, int every)
{
    for (int col = from; col <= to; col += every)
        addItem("coin", col, row);
}

void Level::addEnemy(const std::string& type, int col, int row, int variant)
{
    m_enemySpawns.push_back({type, {col * Tile + 6.0f, row * Tile}, variant});
}

void Level::addItem(const std::string& type, int col, int row, int variant)
{
    sf::Vector2f pos{col * Tile + 4.0f, row * Tile + 4.0f};
    if (type == "mushroom" || type == "star" || type == "fireflower" || type == "key")
        pos = {col * Tile, row * Tile};
    if (type == "chest")
        pos = {col * Tile - 3.0f, row * Tile + 2.0f};
    m_itemSpawns.push_back({type, pos, variant});
}

void Level::drawTile(sf::RenderWindow& window, const AssetManager& assets, char tile, const sf::FloatRect& rect, WorldMode world, bool secretsRevealed, float time) const
{
    (void)world;

    const int col = static_cast<int>(rect.left / Tile);
    const int row = static_cast<int>(rect.top / Tile);
    if (m_number == 5 && (assets.texture("castle_dungeon_arena") || assets.texture("castle_exterior")) && (tile == 'W' || tile == 'Z' || tile == 'R' || tile == 'Y' || tile == 'V' || tile == 'K' || tile == 'M' || tile == 'I' || tile == 'O'))
        return;
    if (m_number == 5 && tile == 'L' && row >= 8 && row <= 12 && (col == 100 || col == 144))
        return;

    if (tile == 'Z') {
        drawRect(window, rect, sf::Color(32, 34, 43), sf::Color(19, 17, 24));
        drawRect(window, {rect.left + 4.0f, rect.top + 8.0f, rect.width - 8.0f, rect.height - 8.0f}, sf::Color(8, 11, 19), sf::Color(73, 70, 76));
        drawCircle(window, {rect.left + rect.width * 0.5f, rect.top + 9.0f}, rect.width * 0.34f, sf::Color(8, 11, 19));
        drawRect(window, {rect.left + rect.width * 0.5f - 1.0f, rect.top + 5.0f, 2.0f, rect.height - 8.0f}, sf::Color(54, 64, 82));
        drawRect(window, {rect.left + 8.0f, rect.top + 17.0f, rect.width - 16.0f, 2.0f}, sf::Color(54, 64, 82));
        drawRect(window, {rect.left + 6.0f, rect.top + 6.0f, rect.width - 12.0f, 4.0f}, sf::Color(102, 120, 148, 58));
    } else if (tile == 'R') {
        drawRect(window, rect, sf::Color(42, 30, 25), sf::Color(18, 14, 13));
        drawRect(window, {rect.left + 2.0f, rect.top + 5.0f, rect.width - 4.0f, 4.0f}, sf::Color(134, 86, 48));
        drawRect(window, {rect.left + 2.0f, rect.top + 20.0f, rect.width - 4.0f, 4.0f}, sf::Color(134, 86, 48));
        const sf::Color colors[4] = {sf::Color(150, 54, 56), sf::Color(58, 94, 138), sf::Color(197, 150, 70), sf::Color(82, 142, 78)};
        for (int i = 0; i < 4; ++i) {
            drawRect(window, {rect.left + 5.0f + i * 6.0f, rect.top + 9.0f, 4.0f, 10.0f}, colors[i]);
            if (i < 3)
                drawRect(window, {rect.left + 8.0f + i * 7.0f, rect.top + 24.0f, 5.0f, 6.0f}, colors[3 - i]);
        }
    } else if (tile == 'W') {
        drawRect(window, rect, sf::Color(58, 54, 68), sf::Color(36, 32, 44));
        drawRect(window, {rect.left, rect.top + 10.0f, rect.width, 2.0f}, sf::Color(38, 34, 46));
        drawRect(window, {rect.left, rect.top + 21.0f, rect.width, 2.0f}, sf::Color(38, 34, 46));
        drawRect(window, {rect.left + 15.0f, rect.top, 2.0f, 10.0f}, sf::Color(38, 34, 46));
        drawRect(window, {rect.left + 5.0f, rect.top + 11.0f, 2.0f, 10.0f}, sf::Color(38, 34, 46));
        drawRect(window, {rect.left + 24.0f, rect.top + 22.0f, 2.0f, 10.0f}, sf::Color(38, 34, 46));
        drawRect(window, {rect.left + 3.0f, rect.top + 3.0f, rect.width - 6.0f, 3.0f}, sf::Color(82, 76, 92));
        if ((col + row) % 4 == 0)
            drawRect(window, {rect.left + 7.0f, rect.top + 17.0f, 8.0f, 2.0f}, sf::Color(92, 84, 88));
        if ((col * 3 + row) % 7 == 0)
            drawRect(window, {rect.left + 22.0f, rect.top + 6.0f, 4.0f, 4.0f}, sf::Color(32, 29, 38));
    } else if (tile == 'K') {
        drawRect(window, rect, sf::Color(78, 72, 74), sf::Color(34, 30, 34));
        drawRect(window, {rect.left, rect.top, rect.width, 5.0f}, sf::Color(122, 112, 105));
        drawRect(window, {rect.left, rect.top + 13.0f, rect.width, 2.0f}, sf::Color(39, 35, 38));
        drawRect(window, {rect.left, rect.top + 25.0f, rect.width, 2.0f}, sf::Color(39, 35, 38));
        drawRect(window, {rect.left + 14.0f, rect.top + 1.0f, 2.0f, 12.0f}, sf::Color(39, 35, 38));
        drawRect(window, {rect.left + 4.0f, rect.top + 15.0f, 2.0f, 10.0f}, sf::Color(39, 35, 38));
        drawRect(window, {rect.left + 23.0f, rect.top + 27.0f, 2.0f, 5.0f}, sf::Color(39, 35, 38));
    } else if (tile == 'M') {
        drawRect(window, rect, sf::Color(45, 41, 47), sf::Color(26, 23, 30));
        drawRect(window, {rect.left + 4.0f, rect.top + 5.0f, 10.0f, 3.0f}, sf::Color(67, 61, 69));
        drawRect(window, {rect.left + 18.0f, rect.top + 18.0f, 8.0f, 3.0f}, sf::Color(30, 27, 33));
        drawRect(window, {rect.left, rect.top + 15.0f, rect.width, 2.0f}, sf::Color(26, 23, 30));
    } else if (tile == 'I') {
        drawRect(window, rect, sf::Color(55, 50, 60), sf::Color(28, 25, 34));
        drawRect(window, {rect.left + 5.0f, rect.top, rect.width - 10.0f, rect.height}, sf::Color(83, 76, 84), sf::Color(35, 31, 38));
        drawRect(window, {rect.left + 8.0f, rect.top + 4.0f, 4.0f, rect.height - 8.0f}, sf::Color(114, 104, 102));
        drawRect(window, {rect.left + 20.0f, rect.top + 2.0f, 3.0f, rect.height - 4.0f}, sf::Color(39, 35, 42));
        drawRect(window, {rect.left + 3.0f, rect.top + 12.0f, rect.width - 6.0f, 2.0f}, sf::Color(38, 34, 42));
    } else if (tile == 'O') {
        drawRect(window, rect, sf::Color(58, 54, 68), sf::Color(36, 32, 44));
        drawRect(window, {rect.left + 13.0f, rect.top + 16.0f, 6.0f, 12.0f}, sf::Color(72, 46, 34));
        drawRect(window, {rect.left + 9.0f, rect.top + 14.0f, 14.0f, 5.0f}, sf::Color(112, 66, 36));
        const float flicker = std::sin(time * 9.0f + rect.left * 0.1f) * 2.0f;
        sf::ConvexShape flame(4);
        flame.setPoint(0, {rect.left + 16.0f, rect.top + 4.0f + flicker});
        flame.setPoint(1, {rect.left + 23.0f, rect.top + 13.0f});
        flame.setPoint(2, {rect.left + 16.0f, rect.top + 20.0f});
        flame.setPoint(3, {rect.left + 9.0f, rect.top + 13.0f});
        flame.setFillColor(sf::Color(255, 134, 48));
        window.draw(flame);
        drawCircle(window, {rect.left + 16.0f, rect.top + 13.0f}, 4.0f, sf::Color(255, 236, 115));
    } else if (tile == 'Y') {
        drawRect(window, rect, sf::Color(58, 54, 68), sf::Color(36, 32, 44));
        drawRect(window, {rect.left + 5.0f, rect.top + 4.0f, rect.width - 10.0f, 4.0f}, sf::Color(218, 180, 78));
        drawRect(window, {rect.left + 7.0f, rect.top + 8.0f, rect.width - 14.0f, rect.height - 8.0f}, sf::Color(156, 34, 48), sf::Color(82, 22, 34));
        sf::ConvexShape notch(3);
        notch.setPoint(0, {rect.left + rect.width * 0.5f - 5.0f, rectBottom(rect)});
        notch.setPoint(1, {rect.left + rect.width * 0.5f + 5.0f, rectBottom(rect)});
        notch.setPoint(2, {rect.left + rect.width * 0.5f, rectBottom(rect) - 7.0f});
        notch.setFillColor(sf::Color(58, 54, 68));
        window.draw(notch);
    } else if (tile == 'V') {
        drawRect(window, rect, sf::Color(58, 54, 68), sf::Color(36, 32, 44));
        for (int i = 0; i < 3; ++i) {
            sf::CircleShape link(5.0f, 12);
            link.setOrigin(5.0f, 5.0f);
            link.setPosition(rect.left + rect.width * 0.5f, rect.top + 6.0f + i * 11.0f);
            link.setFillColor(sf::Color::Transparent);
            link.setOutlineColor(sf::Color(168, 156, 134));
            link.setOutlineThickness(2.0f);
            window.draw(link);
        }
    } else if (tile == 'G') {
        drawRect(window, rect, sf::Color(194, 107, 48), sf::Color(93, 54, 38));
        drawRect(window, {rect.left + 3.0f, rect.top + 3.0f, rect.width - 6.0f, rect.height - 6.0f}, sf::Color(216, 132, 58));
        drawRect(window, {rect.left, rect.top, rect.width, 4.0f}, sf::Color(246, 177, 70));
        drawRect(window, {rect.left + 15.0f, rect.top + 4.0f, 2.0f, rect.height - 6.0f}, sf::Color(128, 70, 42));
        drawRect(window, {rect.left + 4.0f, rect.top + 15.0f, rect.width - 8.0f, 2.0f}, sf::Color(128, 70, 42));
    } else if (tile == 'D') {
        drawRect(window, rect, sf::Color(156, 84, 43), sf::Color(87, 48, 35));
        drawRect(window, {rect.left + 6.0f, rect.top + 6.0f, 7.0f, 7.0f}, sf::Color(190, 105, 50));
        drawRect(window, {rect.left + 21.0f, rect.top + 18.0f, 6.0f, 6.0f}, sf::Color(111, 62, 39));
    } else if (tile == 'B') {
        drawRect(window, rect, sf::Color(190, 86, 45), sf::Color(83, 42, 33));
        drawRect(window, {rect.left, rect.top + 10.0f, rect.width, 2.0f}, sf::Color(92, 45, 34));
        drawRect(window, {rect.left, rect.top + 21.0f, rect.width, 2.0f}, sf::Color(92, 45, 34));
        drawRect(window, {rect.left + 14.0f, rect.top, 2.0f, 10.0f}, sf::Color(92, 45, 34));
        drawRect(window, {rect.left + 5.0f, rect.top + 11.0f, 2.0f, 10.0f}, sf::Color(92, 45, 34));
        drawRect(window, {rect.left + 24.0f, rect.top + 22.0f, 2.0f, 10.0f}, sf::Color(92, 45, 34));
    } else if (tile == '?' || tile == 'U') {
        drawRect(window, rect, tile == '?' ? sf::Color(244, 176, 45) : sf::Color(126, 112, 86), sf::Color(92, 63, 40));
        drawRect(window, {rect.left + 3.0f, rect.top + 3.0f, rect.width - 6.0f, rect.height - 6.0f}, tile == '?' ? sf::Color(255, 200, 65) : sf::Color(146, 132, 100));
        if (tile == '?' && assets.hasFont()) {
            sf::Text mark("?", assets.font(), 22);
            mark.setStyle(sf::Text::Bold);
            mark.setFillColor(sf::Color(112, 66, 28));
            mark.setOutlineColor(sf::Color(255, 236, 132));
            mark.setOutlineThickness(1.0f);
            const auto bounds = mark.getLocalBounds();
            mark.setOrigin(bounds.left + bounds.width * 0.5f, bounds.top + bounds.height * 0.5f);
            mark.setPosition(rect.left + rect.width * 0.5f, rect.top + rect.height * 0.48f);
            window.draw(mark);
        }
    } else if (tile == 'P' || tile == 'p') {
        drawRect(window, rect, sf::Color(36, 174, 58), sf::Color(12, 86, 32));
        drawRect(window, {rect.left + 7.0f, rect.top + 4.0f, 5.0f, rect.height - 8.0f}, sf::Color(155, 255, 137));
        drawRect(window, {rect.left + rect.width - 9.0f, rect.top + 3.0f, 3.0f, rect.height - 6.0f}, sf::Color(18, 115, 38));
        if (tile == 'P')
            drawRect(window, {rect.left - 5.0f, rect.top - 6.0f, rect.width + 10.0f, 18.0f}, sf::Color(60, 207, 74), sf::Color(12, 86, 32));
    } else if (tile == '^') {
        drawRect(window, {rect.left + 1.0f, rect.top + rect.height - 6.0f, rect.width - 2.0f, 5.0f}, sf::Color(54, 47, 54));
        drawRect(window, {rect.left + 3.0f, rect.top + rect.height - 8.0f, rect.width - 6.0f, 3.0f}, sf::Color(114, 104, 112));
        for (int i = 0; i < 3; ++i) {
            const float step = rect.width / 3.0f;
            sf::ConvexShape spike(3);
            spike.setPoint(0, {rect.left + i * step + 2.0f, rectBottom(rect) - 7.0f});
            spike.setPoint(1, {rect.left + i * step + step * 0.5f, rect.top + 2.0f});
            spike.setPoint(2, {rect.left + (i + 1) * step - 2.0f, rectBottom(rect) - 7.0f});
            spike.setFillColor(sf::Color(236, 238, 232));
            spike.setOutlineColor(sf::Color(46, 42, 48));
            spike.setOutlineThickness(1.0f);
            window.draw(spike);
            sf::ConvexShape highlight(3);
            highlight.setPoint(0, {rect.left + i * step + step * 0.42f, rect.top + 8.0f});
            highlight.setPoint(1, {rect.left + i * step + step * 0.50f, rect.top + 3.5f});
            highlight.setPoint(2, {rect.left + i * step + step * 0.50f, rectBottom(rect) - 11.0f});
            highlight.setFillColor(sf::Color(255, 255, 255, 170));
            window.draw(highlight);
        }
    } else if (tile == '~') {
        drawRect(window, rect, world == WorldMode::Ghost ? sf::Color(105, 62, 160) : sf::Color(198, 61, 54));
        for (int i = 0; i < 4; ++i)
            drawCircle(window, {rect.left + rect.width * (0.16f + i * 0.23f), rect.top + rect.height * 0.48f + std::sin(time * 5.0f + i) * 2.0f}, rect.width * 0.08f, sf::Color(255, 192, 73));
    } else if (tile == 'T') {
        drawRect(window, rect, sf::Color(54, 178, 226), sf::Color(22, 80, 110));
        drawRect(window, {rect.left + 4.0f, rect.top + rect.height * 0.25f, rect.width - 8.0f, rect.height * 0.18f}, sf::Color(255, 230, 88));
    } else if (tile == 'S') {
        if (secretsRevealed || world == WorldMode::Ghost)
            drawRect(window, rect, withAlpha(sf::Color(255, 235, 115), 120), sf::Color(255, 235, 115));
        else
            drawRect(window, rect, sf::Color(104, 73, 47));
    } else if (tile == 'C') {
        drawRect(window, {rect.left + rect.width * 0.46f, rect.top - rect.height * 0.36f, 4.0f, rect.height * 1.35f}, sf::Color(240, 240, 230));
        drawRect(window, {rect.left + rect.width * 0.53f, rect.top - rect.height * 0.28f, rect.width * 0.62f, rect.height * 0.38f}, sf::Color(86, 185, 245));
    } else if (tile == 'L') {
        drawRect(window, rect, sf::Color(116, 63, 38), sf::Color(52, 32, 24));
        drawRect(window, {rect.left + 5.0f, rect.top + 4.0f, rect.width - 10.0f, rect.height - 8.0f}, sf::Color(155, 86, 48));
        drawRect(window, {rect.left + rect.width - 10.0f, rect.top + rect.height * 0.5f - 3.0f, 5.0f, 6.0f}, sf::Color(255, 218, 90));
    } else if (tile == 'F' || tile == 'f') {
        drawRect(window, {rect.left + rect.width * 0.48f, rect.top, 4.0f, rect.height}, sf::Color(240, 240, 230));
        if (tile == 'F') {
            sf::ConvexShape flag(3);
            flag.setPoint(0, {rect.left + rect.width * 0.55f, rect.top + rect.height * 0.12f});
            flag.setPoint(1, {rect.left + rect.width * 1.35f, rect.top + rect.height * 0.38f});
            flag.setPoint(2, {rect.left + rect.width * 0.55f, rect.top + rect.height * 0.64f});
            flag.setFillColor(sf::Color(250, 74, 94));
            window.draw(flag);
        }
    }
}

#include "CollisionManager.hpp"

#include "Level.hpp"

#include <cmath>
// Zbiera wszystkie obiekty stałe znajdujące się w pobliżu badanego obszaru.
// Dzięki temu kolizje są sprawdzane tylko lokalnie, a nie dla całej mapy.
namespace {
std::vector<std::tuple<sf::FloatRect, int, int, int>> solidBodies(Level& level, const sf::FloatRect& area, WorldMode world, bool secretsRevealed)
{
    std::vector<std::tuple<sf::FloatRect, int, int, int>> bodies;
    const int left = std::max(0, static_cast<int>(std::floor(area.left / Tile)) - 1);
    const int right = std::min(level.columns() - 1, static_cast<int>(std::floor(rectRight(area) / Tile)) + 1);
    const int top = std::max(0, static_cast<int>(std::floor(area.top / Tile)) - 1);
    const int bottom = std::min(14, static_cast<int>(std::floor(rectBottom(area) / Tile)) + 1);

    for (int row = top; row <= bottom; ++row) {
        for (int col = left; col <= right; ++col) {
            if (level.isSolidTile(level.tileAt(row, col), world, secretsRevealed))
                bodies.emplace_back(level.tileRect(row, col), row, col, -1);
        }
    }

    auto& platforms = level.platforms();
    for (int i = 0; i < static_cast<int>(platforms.size()); ++i) {
        if (platforms[static_cast<std::size_t>(i)].visible && platforms[static_cast<std::size_t>(i)].rect.intersects(area))
            bodies.emplace_back(platforms[static_cast<std::size_t>(i)].rect, -1, -1, i);
    }

    return bodies;
}
}

CollisionResult CollisionManager::move(sf::FloatRect& rect,
                                       sf::Vector2f& velocity,
                                       Level& level,
                                       WorldMode world,
                                       bool secretsRevealed,
                                       float dt)
{
    CollisionResult result;

    rect.left += velocity.x * dt;
    for (const auto& [body, row, col, platform] : solidBodies(level, growRect(rect, 1.0f), world, secretsRevealed)) {
        if (!rect.intersects(body))
            continue;
        if (velocity.x > 0.0f) {
            rect.left = body.left - rect.width;
            result.hitWall = true;
        } else if (velocity.x < 0.0f) {
            rect.left = rectRight(body);
            result.hitWall = true;
        }
        velocity.x = 0.0f;
    }

    rect.top += velocity.y * dt;
    for (const auto& [body, row, col, platform] : solidBodies(level, growRect(rect, 1.0f), world, secretsRevealed)) {
        if (!rect.intersects(body))
            continue;
        if (velocity.y > 0.0f) {
            rect.top = body.top - rect.height;
            velocity.y = 0.0f;
            result.onGround = true;
            result.platformIndex = platform;
            if (row >= 0 && level.isTrampoline(level.tileAt(row, col)))
                result.hitTrampoline = true;
        } else if (velocity.y < 0.0f) {
            rect.top = rectBottom(body);
            velocity.y = 40.0f;
            result.hitHead = true;
            result.headRow = row;
            result.headCol = col;
        }
    }

    if (rect.left < 0.0f) {
        rect.left = 0.0f;
        velocity.x = 0.0f;
    }
    if (rectRight(rect) > level.width()) {
        rect.left = level.width() - rect.width;
        velocity.x = 0.0f;
    }

    const sf::Vector2f feetA(rect.left + 6.0f, rectBottom(rect) + 3.0f);
    const sf::Vector2f feetB(rectRight(rect) - 6.0f, rectBottom(rect) + 3.0f);
    // Kolce rania rowniez gdy bohater wbiegnie w nie bokiem, a nie tylko gdy
    // spadnie na nie z gory. Sondujemy dolna czesc tulowia po obu stronach,
    // tuz nad stopami - kafel z kolcami lezy zaraz nad podlogiem, na ktorej
    // stoi gracz. Wysokosc liczona wzgledem stop, wiec dziala dla malej i duzej
    // postaci, a krawedzie jam z lawa (puste na tej wysokosci) nie dotykaja.
    const float sideY = rectBottom(rect) - 10.0f;
    const sf::Vector2f sideA(rect.left + 3.0f, sideY);
    const sf::Vector2f sideB(rectRight(rect) - 3.0f, sideY);
    result.hitHazard = tileHazardAt(level, feetA) || tileHazardAt(level, feetB)
                    || tileHazardAt(level, sideA) || tileHazardAt(level, sideB);
    return result;
}

bool CollisionManager::tileHazardAt(const Level& level, sf::Vector2f point)
{
    const int row = static_cast<int>(std::floor(point.y / Tile));
    const int col = static_cast<int>(std::floor(point.x / Tile));
    return level.isHazardTile(level.tileAt(row, col));
}

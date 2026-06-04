#include "Sprites.hpp"

#include <cstdlib>

namespace Sprites {
std::vector<sf::IntRect> playerFrames(const std::string& character, bool big, const std::string& state)
{
    const bool luigi = character == "Luigi";
    const int baseY = big ? (luigi ? 128 : 64) : (luigi ? 32 : 0);
    const int h = big ? 64 : 32;
    if (!big) {
        if (state == "run")
            return {{64, baseY, 32, h}, {96, baseY, 32, h}, {128, baseY, 32, h}};
        if (state == "jump")
            return {{160, baseY, 32, h}};
        if (state == "dead")
            return {{192, baseY, 32, h}};
        return {{0, baseY, 32, h}, {32, baseY, 32, h}};
    }

    if (state == "run")
        return {{64, baseY, 32, h}, {96, baseY, 32, h}, {128, baseY, 32, h}};
    if (state == "jump")
        return {{160, baseY, 32, h}};
    if (state == "dead")
        return {{192, baseY, 32, h}};
    return {{0, baseY, 32, h}, {32, baseY, 32, h}};
}

sf::IntRect enemyFrame(const std::string& enemy, int frame)
{
    frame = std::abs(frame);
    if (enemy == "goomba")
        return {(frame % 2) * 32, 0, 32, 32};
    if (enemy == "koopa")
        return {64 + (frame % 2) * 32, 0, 32, 48};
    if (enemy == "koopa_shell")
        return {128, 0, 32, 32};
    if (enemy == "piranha")
        return {160 + (frame % 2) * 32, 0, 32, 48};
    if (enemy == "bullet")
        return {224, 0, 32, 32};
    if (enemy == "flyer")
        return {(frame % 2) * 32, 0, 32, 32};
    if (enemy == "shooter")
        return {160, 0, 32, 48};
    if (enemy == "shadow")
        return {(frame % 2) * 32, 0, 32, 32};
    if (enemy == "bowser")
        return {0, 48, 64, 64};
    return {0, 0, 32, 32};
}

sf::IntRect itemFrame(ItemType type, int frame)
{
    frame = std::abs(frame);
    switch (type) {
    case ItemType::Coin:
        return {128 + (frame % 4) * 8, 49, 8, 16};
    case ItemType::Heart:
        return {176, 49, 16, 16};
    case ItemType::Mushroom:
        return {0, 16, 16, 16};
    case ItemType::Star:
        return {64 + (frame % 2) * 16, 16, 16, 16};
    case ItemType::FireFlower:
        return {48 + (frame % 2) * 16, 16, 16, 16};
    case ItemType::Key:
        return {96, 50, 16, 16};
    case ItemType::Chest:
        return {208, 82, 16, 16};
    default:
        return {0, 16, 16, 16};
    }
}
}

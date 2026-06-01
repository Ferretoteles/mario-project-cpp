#pragma once

#include "Core.hpp"
#include "AssetManager.hpp"

struct SpawnRequest {
    std::string type;
    sf::Vector2f pos;
    int variant = 0;
};

struct MovingPlatform {
    sf::FloatRect rect;
    sf::Vector2f previous;
    float minX = 0.0f;
    float maxX = 0.0f;
    float speed = 0.0f;
    int direction = 1;
    bool disappearing = false;
    float timer = 0.0f;
    bool visible = true;
};

struct TeleportPipe {
    sf::FloatRect from;
    sf::Vector2f to;
};

class Level {
public:
    void build(int number);
    void generateBonusRoom(int seed);
    void update(float dt);
    void draw(sf::RenderWindow& window, const AssetManager& assets, WorldMode world, bool secretsRevealed, float time, bool castleInterior = false) const;

    char tileAt(int row, int col) const;
    void setTile(int row, int col, char tile);
    sf::FloatRect tileRect(int row, int col) const;
    bool isSolidTile(char tile, WorldMode world, bool secretsRevealed) const;
    bool isHazardTile(char tile) const;
    bool isTrampoline(char tile) const;
    bool hitBlock(int row, int col, std::vector<SpawnRequest>& spawns);

    int number() const;
    int columns() const;
    float width() const;
    float height() const;
    WeatherType weather() const;
    sf::Vector2f playerStart() const;
    sf::Vector2f flagPosition() const;
    const std::string& headline() const;
    const std::vector<SpawnRequest>& enemySpawns() const;
    const std::vector<SpawnRequest>& itemSpawns() const;
    const std::vector<SpawnRequest>& npcSpawns() const;
    const std::vector<TeleportPipe>& teleports() const;
    std::vector<MovingPlatform>& platforms();
    const std::vector<MovingPlatform>& platforms() const;

private:
    void clear(int columns);
    void buildLevel1();
    void buildLevel2();
    void buildLevel3();
    void buildLevel4();
    void buildLevel5();
    void placeFlag(int col);
    void addSecretRoom(int startCol, sf::Vector2f returnTarget, bool keyRoom);
    void baseGround(const std::vector<std::pair<int, int>>& pits);
    void fill(int row, int from, int to, char tile);
    void placePipe(int col, int width, int height, sf::Vector2f target = {-1.0f, -1.0f});
    void placeStairs(int startCol, int steps, int direction);
    void addCoins(int row, int from, int to, int every = 2);
    void addEnemy(const std::string& type, int col, int row, int variant = 0);
    void addItem(const std::string& type, int col, int row, int variant = 0);
    void addNpc(const std::string& type, int col, int row, int variant = 0);
    void drawTile(sf::RenderWindow& window, const AssetManager& assets, char tile, const sf::FloatRect& rect, WorldMode world, bool secretsRevealed, float time) const;

    int m_number = 1;
    int m_columns = 0;
    std::vector<std::string> m_tiles;
    std::vector<MovingPlatform> m_platforms;
    std::vector<TeleportPipe> m_teleports;
    std::vector<SpawnRequest> m_enemySpawns;
    std::vector<SpawnRequest> m_itemSpawns;
    std::vector<SpawnRequest> m_npcSpawns;
    sf::Vector2f m_start{3.0f * Tile, 10.0f * Tile};
    sf::Vector2f m_flag{160.0f * Tile, 7.0f * Tile};
    WeatherType m_weather = WeatherType::Clear;
    sf::Color m_skyTop{92, 184, 232};
    sf::Color m_skyBottom{246, 229, 180};
    std::string m_headline;
};

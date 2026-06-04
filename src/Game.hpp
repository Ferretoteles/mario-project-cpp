#pragma once

#include "AchievementManager.hpp"
#include "AssetManager.hpp"
#include "Enemy.hpp"
#include "Item.hpp"
#include "Level.hpp"
#include "Menu.hpp"
#include "Player.hpp"
#include "QuestManager.hpp"
#include "SaveManager.hpp"

#include <filesystem>
#include <random>

struct Particle {
    sf::Vector2f pos;
    sf::Vector2f velocity;
    sf::Color color;
    std::string text;
    float life = 0.0f;
    float total = 0.0f;
    float radius = 3.0f;
};

enum class CastleCutscenePhase {
    None,
    Opening,
    Entering,
    FadeOut,
    FadeIn
};

enum class DungeonStage {
    Outside = 0,
    Stars = 1,
    Monsters = 2,
    BossFight = 3
};

class Game {
public:
    Game();
    void run();

private:
    void processEvents();
    void handleKeyPressed(sf::Keyboard::Key key);
    void handleKeyReleased(sf::Keyboard::Key key);
    void handleMousePressed(sf::Mouse::Button button, sf::Vector2i pixel);
    bool keyDown(sf::Keyboard::Key key) const;

    void setState(AppState state);
    void loadSaveSlot(int slot);
    void loadLevel(int level);
    void spawnFromLevel();
    std::unique_ptr<Enemy> makeEnemy(const SpawnRequest& spawn);
    void addItemSpawn(const SpawnRequest& spawn);
    Boss* bossEnemy();
    const Boss* bossEnemy() const;
    void setupDungeonStage(DungeonStage stage, bool movePlayer);
    void clearDungeonGameplayObjects(bool keepBoss);
    void clearDungeonLayout();
    void buildDungeonStageLayout(DungeonStage stage);
    void updateDungeonStages(float dt);
    void startDungeonStageTransition(DungeonStage nextStage);
    sf::Vector2f dungeonStageStart(DungeonStage stage) const;
    sf::Vector2f dungeonBossStart() const;
    std::string dungeonStageText() const;

    void update(float dt);
    void updatePlaying(float dt);
    void updateParticles(float dt);
    void updateCollisions();
    void completeLevel();
    void respawnOrGameOver();
    void buyShopItem(int index);
    void editorPaint(sf::Vector2i pixel, char tile);

    void render();
    void drawWorld();
    void drawHud();
    void drawBossBar();
    void drawDungeonStagePanel();
    void drawCastleGate();
    void drawCastleFade();
    void drawMinimap();
    void drawParticles();
    void drawLevelSelect();
    void drawSettings();
    void drawShop();
    void drawNewspaper();
    void drawGameOver();
    void drawVictory();
    void drawEditor();
    void drawText(const std::string& text, unsigned size, sf::Vector2f pos, sf::Color color, bool center = false, float outline = 1.5f);
    void spawnBurst(sf::Vector2f pos, sf::Color color, int count);
    void spawnText(const std::string& text, sf::Vector2f pos, sf::Color color);

    std::filesystem::path findAssetRoot() const;

    sf::RenderWindow m_window;
    sf::View m_worldView;
    sf::View m_uiView;
    std::array<bool, sf::Keyboard::KeyCount> m_keys{};
    AppState m_state = AppState::Title;

    std::filesystem::path m_assetRoot;
    AssetManager m_assets;
    SaveManager m_saveManager;
    SaveData m_save;
    EventSystem m_events;
    AchievementManager m_achievements;
    QuestManager m_quests;
    Menu m_menu;

    Level m_level;
    Player m_player;
    std::vector<std::unique_ptr<Enemy>> m_enemies;
    std::vector<std::unique_ptr<Item>> m_items;
    std::vector<Projectile> m_projectiles;
    std::vector<Particle> m_particles;

    WorldMode m_world = WorldMode::Normal;
    sf::Vector2f m_camera{0.0f, 0.0f};
    sf::Vector2f m_checkpoint{3.0f * Tile, 9.0f * Tile};
    std::string m_shopMessage;
    float m_autoSaveTimer = 0.0f;
    float m_screenShakeTimer = 0.0f;
    float m_screenShakeStrength = 0.0f;
    float m_time = 0.0f;
    int m_currentLevel = 1;
    int m_selectedLevel = 1;
    int m_selectedSlot = 1;
    int m_score = 0;
    char m_editorTile = 'G';
    bool m_shopMushroomNextRun = false;
    bool m_shopShieldNextRun = false;
    bool m_shopKeyNextRun = false;
    bool m_adminMode = false;
    bool m_bossFightStarted = false;
    bool m_castleInterior = false;
    CastleCutscenePhase m_castlePhase = CastleCutscenePhase::None;
    DungeonStage m_dungeonStage = DungeonStage::Outside;
    DungeonStage m_pendingDungeonStage = DungeonStage::Outside;
    float m_castleTimer = 0.0f;
    float m_castleDoorOpen = 0.0f;
    float m_dungeonStageTransitionTimer = 0.0f;
    float m_dungeonMessageTimer = 0.0f;
    float m_bossItemTimer = 0.0f;
    std::string m_ending = "Hero Ending";
    std::mt19937 m_rng;
};

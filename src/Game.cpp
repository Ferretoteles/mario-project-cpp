#include "Game.hpp"

#include "AudioManager.hpp"
#include "CollisionManager.hpp"
#include "Sprites.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
// Oblicza odległość między dwoma punktami.
// Używane np. przy sprawdzaniu dystansu między graczem a obiektami gry.
namespace {
float distance(sf::Vector2f a, sf::Vector2f b)
{
    const sf::Vector2f d = a - b;
    return std::sqrt(d.x * d.x + d.y * d.y);
}

sf::Color withAlpha(sf::Color color, sf::Uint8 alpha)
{
    color.a = alpha;
    return color;
}

void drawHeartShape(sf::RenderWindow& window, sf::Vector2f pos, float size, sf::Color color, sf::Color outline)
{
    auto drawCirclePart = [&](sf::Vector2f offset, sf::Color fill) {
        sf::CircleShape circle(size * 0.26f, 20);
        circle.setOrigin(size * 0.26f, size * 0.26f);
        circle.setPosition(pos + offset);
        circle.setFillColor(fill);
        circle.setOutlineColor(outline);
        circle.setOutlineThickness(1.5f);
        window.draw(circle);
    };

    sf::ConvexShape tip(3);
    tip.setPoint(0, {pos.x - size * 0.42f, pos.y + size * 0.08f});
    tip.setPoint(1, {pos.x + size * 0.42f, pos.y + size * 0.08f});
    tip.setPoint(2, {pos.x, pos.y + size * 0.56f});
    tip.setFillColor(color);
    tip.setOutlineColor(outline);
    tip.setOutlineThickness(1.5f);
    window.draw(tip);
    drawCirclePart({-size * 0.21f, -size * 0.05f}, color);
    drawCirclePart({size * 0.21f, -size * 0.05f}, color);
}
// Rysuje ikonę serca w interfejsie gracza.
// Jeśli dostępna jest tekstura, zostaje użyta grafika, a w przeciwnym razie rysowany jest prosty kształt.
void drawHeartIcon(sf::RenderWindow& window, const AssetManager& assets, sf::Vector2f pos, float size, bool filled)
{
    const sf::Color color = filled ? sf::Color(245, 74, 96) : sf::Color(73, 77, 94);
    const sf::Color outline = filled ? sf::Color(70, 18, 34) : sf::Color(32, 36, 48);

    if (const sf::Texture* texture = assets.texture("heart")) {
        sf::Sprite shadow(*texture);
        shadow.setColor(sf::Color(0, 0, 0, 95));
        shadow.setPosition(pos.x - size * 0.5f + 2.0f, pos.y - size * 0.5f + 3.0f);
        shadow.setScale(size / texture->getSize().x, size / texture->getSize().y);
        window.draw(shadow);

        sf::Sprite sprite(*texture);
        sprite.setColor(filled ? sf::Color::White : sf::Color(95, 95, 112));
        sprite.setPosition(pos.x - size * 0.5f, pos.y - size * 0.5f);
        sprite.setScale(size / texture->getSize().x, size / texture->getSize().y);
        window.draw(sprite);
        return;
    }

    drawHeartShape(window, pos + sf::Vector2f(2.0f, 3.0f), size, sf::Color(0, 0, 0, 90), sf::Color(0, 0, 0, 0));
    drawHeartShape(window, pos, size, color, outline);
}

bool drawTextureIcon(sf::RenderWindow& window, const AssetManager& assets, const std::string& id, sf::Vector2f center, float size)
{
    const sf::Texture* texture = assets.texture(id);
    if (!texture || texture->getSize().x == 0 || texture->getSize().y == 0)
        return false;

    sf::Sprite shadow(*texture);
    shadow.setColor(sf::Color(0, 0, 0, 90));
    shadow.setPosition(center.x - size * 0.5f + 2.0f, center.y - size * 0.5f + 2.0f);
    shadow.setScale(size / static_cast<float>(texture->getSize().x), size / static_cast<float>(texture->getSize().y));
    window.draw(shadow);

    sf::Sprite sprite(*texture);
    sprite.setPosition(center.x - size * 0.5f, center.y - size * 0.5f);
    sprite.setScale(size / static_cast<float>(texture->getSize().x), size / static_cast<float>(texture->getSize().y));
    window.draw(sprite);
    return true;
}

sf::Vector2f bossCheckpointPosition()
{
    return {96.0f * Tile, 12.0f * Tile};
}

sf::Vector2f castleInteriorStart()
{
    return {106.0f * Tile, 12.0f * Tile};
}

SpawnRequest makeSpawn(const std::string& type, int col, int row, int variant = 0)
{
    return {type, {col * Tile, row * Tile}, variant};
}
}

Game::Game()
    : m_window(sf::VideoMode(WindowWidth, WindowHeight), "LanternRun SFML - Classic Mario Style", sf::Style::Titlebar | sf::Style::Close)
    , m_worldView(sf::FloatRect(0.0f, 0.0f, static_cast<float>(WindowWidth), static_cast<float>(WindowHeight)))
    , m_uiView(sf::FloatRect(0.0f, 0.0f, static_cast<float>(WindowWidth), static_cast<float>(WindowHeight)))
    , m_rng(std::random_device{}())
{
    m_window.setVerticalSyncEnabled(true);
    m_window.setKeyRepeatEnabled(false);

    m_assetRoot = findAssetRoot();
    m_assets.setRoot(m_assetRoot);
    m_assets.load();
    m_saveManager.setRoot(m_assetRoot);
    loadSaveSlot(1);

    m_achievements.attach(m_save, m_events);
    m_events.subscribe(EventType::CoinCollected, [this](const GameEvent& e) { m_quests.onEvent(e, m_player.stats()); });
    m_events.subscribe(EventType::EnemyKilled, [this](const GameEvent& e) { m_player.stats().kills += 1; m_quests.onEvent(e, m_player.stats()); });
    m_events.subscribe(EventType::KeyFound, [this](const GameEvent& e) {
        m_quests.onEvent(e, m_player.stats());
        // Osiagniecie za zdobycie 2 kluczy (po jednym z sekretnych pokoi L2 i L4).
        if (++m_keysCollected >= 2 && m_save.achievements.insert("Sekretny pokoj").second)
            spawnText("OSIAGNIECIE: Sekretny pokoj!", m_player.center() + sf::Vector2f(0.0f, -56.0f), sf::Color(255, 220, 90));
    });
    m_events.subscribe(EventType::BossDefeated, [this](const GameEvent& e) { m_quests.onEvent(e, m_player.stats()); });
    m_events.subscribe(EventType::BossLanded, [this](const GameEvent&) {
        m_screenShakeTimer = 0.18f;
        m_screenShakeStrength = 5.0f;
    });

    AudioManager::instance().initialize(m_save.volume);
    setState(AppState::Title);
}

void Game::run()
{
    sf::Clock clock;
    while (m_window.isOpen()) {
        processEvents();
        float dt = std::min(clock.restart().asSeconds(), 0.04f);
        m_time += dt;
        update(dt);
        render();
    }
}

void Game::processEvents()
{
    sf::Event event{};
    while (m_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            m_window.close();
        } else if (event.type == sf::Event::Resized) {
            const float w = static_cast<float>(event.size.width);
            const float h = static_cast<float>(event.size.height);
            m_uiView.setSize(w, h);
            m_uiView.setCenter(w * 0.5f, h * 0.5f);
            m_worldView.setSize(w, h);
        } else if (event.type == sf::Event::KeyPressed) {
            handleKeyPressed(event.key.code);
        } else if (event.type == sf::Event::KeyReleased) {
            handleKeyReleased(event.key.code);
        } else if (event.type == sf::Event::MouseButtonPressed) {
            handleMousePressed(event.mouseButton.button, {event.mouseButton.x, event.mouseButton.y});
        }
    }
}

void Game::handleKeyPressed(sf::Keyboard::Key key)
{
    const int code = static_cast<int>(key);
    if (code >= 0 && code < static_cast<int>(m_keys.size()))
        m_keys[static_cast<std::size_t>(code)] = true;

    if (key == sf::Keyboard::Escape) {
        if (m_state == AppState::Playing)
            setState(AppState::Paused);
        else if (m_state == AppState::Paused)
            setState(AppState::Playing);
        else if (m_state == AppState::Title)
            m_window.close();
        else
            setState(AppState::Title);
        return;
    }

    if (key == sf::Keyboard::Num1 && m_state == AppState::Title)
        loadSaveSlot(1);
    if (key == sf::Keyboard::Num2 && m_state == AppState::Title)
        loadSaveSlot(2);
    if (key == sf::Keyboard::Num3 && m_state == AppState::Title)
        loadSaveSlot(3);

    if ((m_state == AppState::Title || m_state == AppState::LevelSelect) && key == sf::Keyboard::F9) {
        m_adminMode = !m_adminMode;
        m_selectedLevel = m_adminMode ? 5 : std::clamp(m_selectedLevel, 1, m_save.unlockedLevel);
        AudioManager::instance().play("menu_select");
        return;
    }

    if ((m_state == AppState::Title || m_state == AppState::LevelSelect) && m_adminMode && key == sf::Keyboard::F5) {
        AudioManager::instance().play("menu_select");
        loadLevel(5);
        return;
    }

    if (m_state == AppState::Title) {
        if (key == sf::Keyboard::Up || key == sf::Keyboard::W) {
            m_menu.up();
            AudioManager::instance().play("menu_move");
        }
        if (key == sf::Keyboard::Down || key == sf::Keyboard::S) {
            m_menu.down();
            AudioManager::instance().play("menu_move");
        }
        if (key == sf::Keyboard::Enter) {
            AudioManager::instance().play("menu_select");
            const int choice = m_menu.index();
            if (choice == 0) {
                loadLevel(std::clamp(m_selectedLevel, 1, m_adminMode ? 5 : m_save.unlockedLevel));
            } else if (choice == 1) {
                m_save.upgrades.activeSkin = m_save.upgrades.activeSkin == "Luigi" ? "Mario" : "Luigi";
                m_saveManager.save(m_save);
                setState(AppState::Title);
            } else if (choice == 2) {
                setState(AppState::LevelSelect);
            } else if (choice == 3) {
                setState(AppState::Shop);
            } else if (choice == 4) {
                setState(AppState::Achievements);
            } else if (choice == 5) {
                setState(AppState::Settings);
            } else {
                m_window.close();
            }
        }
        return;
    }

    if (m_state == AppState::LevelSelect) {
        if (key == sf::Keyboard::Left || key == sf::Keyboard::A) {
            m_selectedLevel = std::max(1, m_selectedLevel - 1);
            AudioManager::instance().play("menu_move");
        }
        if (key == sf::Keyboard::Right || key == sf::Keyboard::D) {
            m_selectedLevel = std::min(5, m_selectedLevel + 1);
            AudioManager::instance().play("menu_move");
        }
        if (key == sf::Keyboard::Enter && m_selectedLevel <= (m_adminMode ? 5 : m_save.unlockedLevel)) {
            AudioManager::instance().play("menu_select");
            loadLevel(m_selectedLevel);
        }
        return;
    }

    if (m_state == AppState::Settings) {
        if (key == sf::Keyboard::Left || key == sf::Keyboard::A) {
            m_save.volume = std::max(0.0f, m_save.volume - 5.0f);
            AudioManager::instance().play("menu_move");
        }
        if (key == sf::Keyboard::Right || key == sf::Keyboard::D) {
            m_save.volume = std::min(100.0f, m_save.volume + 5.0f);
            AudioManager::instance().play("menu_move");
        }
        AudioManager::instance().setVolume(m_save.volume);
        m_saveManager.save(m_save);
        return;
    }
    if (m_state == AppState::Achievements) {
        if (key == sf::Keyboard::Enter) {
            AudioManager::instance().play("menu_select");
            setState(AppState::Title);
        }
        return;
    }

    if (m_state == AppState::Shop) {
        if (key == sf::Keyboard::Up || key == sf::Keyboard::W) {
            m_menu.up();
            AudioManager::instance().play("menu_move");
        }
        if (key == sf::Keyboard::Down || key == sf::Keyboard::S) {
            m_menu.down();
            AudioManager::instance().play("menu_move");
        }
        if (key == sf::Keyboard::Enter) {
            AudioManager::instance().play("menu_select");
            buyShopItem(m_menu.index());
        }
        return;
    }

    if (m_state == AppState::Paused) {
        if (key == sf::Keyboard::Up || key == sf::Keyboard::W) {
            m_menu.up();
            AudioManager::instance().play("menu_move");
        }
        if (key == sf::Keyboard::Down || key == sf::Keyboard::S) {
            m_menu.down();
            AudioManager::instance().play("menu_move");
        }
        if (key == sf::Keyboard::P) {
            AudioManager::instance().play("menu_select");
            setState(AppState::Playing);
        }
        if (key == sf::Keyboard::R) {
            AudioManager::instance().play("menu_select");
            loadLevel(m_currentLevel);
        }
        if (key == sf::Keyboard::Q) {
            AudioManager::instance().play("menu_select");
            setState(AppState::Title);
        }
        if (key == sf::Keyboard::Enter) {
            AudioManager::instance().play("menu_select");
            if (m_menu.index() == 0)
                setState(AppState::Playing);
            else if (m_menu.index() == 1)
                loadLevel(m_currentLevel);
            else
                setState(AppState::Title);
        }
        return;
    }

    if (m_state == AppState::Newspaper) {
        if (key == sf::Keyboard::Enter) {
            AudioManager::instance().play("menu_select");
            if (m_currentLevel >= 5)
                setState(AppState::Victory);
            else
                loadLevel(m_currentLevel + 1);
        }
        return;
    }

    if (m_state == AppState::GameOver || m_state == AppState::Victory) {
        if (key == sf::Keyboard::Enter) {
            AudioManager::instance().play("menu_select");
            setState(AppState::Title);
        }
        return;
    }

    if (m_state == AppState::Playing) {
        if (key == sf::Keyboard::P)
            setState(AppState::Paused);
        if ((key == sf::Keyboard::X || key == sf::Keyboard::C) && m_player.canShootFire()) {
            const sf::Vector2f origin = m_player.center() + sf::Vector2f(m_player.facing() * 18.0f, -8.0f);
            m_projectiles.emplace_back(origin, sf::Vector2f(m_player.facing() * 450.0f, -55.0f), true, 2, sf::Color::White);
            AudioManager::instance().play("boss");
        }
        if (key == sf::Keyboard::F2)
            setState(AppState::Editor);
        return;
    }

    if (m_state == AppState::Editor) {
        if (key == sf::Keyboard::F2)
            setState(AppState::Playing);
        if (key == sf::Keyboard::Num1) m_editorTile = 'G';
        if (key == sf::Keyboard::Num2) m_editorTile = 'B';
        if (key == sf::Keyboard::Num3) m_editorTile = '?';
        if (key == sf::Keyboard::Num4) m_editorTile = '^';
        if (key == sf::Keyboard::Num5) m_editorTile = '~';
        if (key == sf::Keyboard::Num6) m_editorTile = 'T';
        if (key == sf::Keyboard::Num7) m_editorTile = 'S';
    }
}

void Game::handleKeyReleased(sf::Keyboard::Key key)
{
    const int code = static_cast<int>(key);
    if (code >= 0 && code < static_cast<int>(m_keys.size()))
        m_keys[static_cast<std::size_t>(code)] = false;
}

void Game::handleMousePressed(sf::Mouse::Button button, sf::Vector2i pixel)
{
    if (m_state != AppState::Editor)
        return;
    editorPaint(pixel, button == sf::Mouse::Left ? m_editorTile : '.');
}

bool Game::keyDown(sf::Keyboard::Key key) const
{
    const int code = static_cast<int>(key);
    return code >= 0 && code < static_cast<int>(m_keys.size()) && m_keys[static_cast<std::size_t>(code)];
}

void Game::setState(AppState state)
{
    m_state = state;
    if (state == AppState::Title)
        m_menu.set("MARIO STYLE RUN", {"Start", "Postac: " + m_save.upgrades.activeSkin, "Wybor poziomu", "Sklep", "Osiagniecia", "Ustawienia", "Wyjscie"});
    if (state == AppState::Paused)
        m_menu.set("PAUZA", {"Resume", "Restart", "Exit"});
    if (state == AppState::Shop) {
        m_shopMessage.clear();
        m_menu.set("SKLEP", {"Serce +1 - 25 monet", "Grzybek na start - 35", "Tarcza - 50", "Klucz bonusowy - 60", "Powrot"});
    }
}

void Game::loadSaveSlot(int slot)
{
    m_selectedSlot = slot;
    m_save = m_saveManager.load(slot);
    m_save.saveSlot = slot;
    m_selectedLevel = std::clamp(m_save.unlockedLevel, 1, 5);
    AudioManager::instance().initialize(m_save.volume);
}

void Game::loadLevel(int level)
{
    m_currentLevel = std::clamp(level, 1, 5);
    m_level.build(m_currentLevel);
    m_world = WorldMode::Normal;
    m_checkpoint = m_level.playerStart();
    m_activeCheckpointCol = -1;
    m_player.reset(m_checkpoint, m_save);
    if (m_shopMushroomNextRun) {
        m_player.makeBig();
        m_shopMushroomNextRun = false;
    }
    if (m_shopShieldNextRun) {
        m_player.giveStar(8.0f);
        m_shopShieldNextRun = false;
    }
    if (m_shopKeyNextRun) {
        m_player.giveKey();
        m_shopKeyNextRun = false;
    }
    m_bossFightStarted = false;
    m_castleInterior = false;
    m_castlePhase = CastleCutscenePhase::None;
    m_dungeonStage = DungeonStage::Outside;
    m_pendingDungeonStage = DungeonStage::Outside;
    m_castleTimer = 0.0f;
    m_castleDoorOpen = 0.0f;
    m_dungeonStageTransitionTimer = 0.0f;
    m_dungeonMessageTimer = 0.0f;
    m_bossItemTimer = 0.0f;
    m_enemies.clear();
    m_items.clear();
    m_projectiles.clear();
    m_particles.clear();
    m_quests.resetForLevel(m_currentLevel);
    m_screenShakeTimer = 0.0f;
    m_screenShakeStrength = 0.0f;
    spawnFromLevel();
    setState(AppState::Playing);
}

void Game::spawnFromLevel()
{
    for (const auto& spawn : m_level.enemySpawns()) {
        auto enemy = makeEnemy(spawn);
        if (enemy)
            m_enemies.push_back(std::move(enemy));
    }
    for (const auto& spawn : m_level.itemSpawns())
        addItemSpawn(spawn);
}

std::unique_ptr<Enemy> Game::makeEnemy(const SpawnRequest& spawn)
{
    if (spawn.type == "goomba")
        return std::make_unique<Goomba>(spawn.pos);
    if (spawn.type == "koopa")
        return std::make_unique<Koopa>(spawn.pos);
    if (spawn.type == "flyer")
        return std::make_unique<Flyer>(spawn.pos);
    if (spawn.type == "shooter")
        return std::make_unique<Shooter>(spawn.pos);
    if (spawn.type == "piranha")
        return std::make_unique<PiranhaPlant>(spawn.pos);
    if (spawn.type == "shadow")
        return std::make_unique<ShadowMonster>(spawn.pos);
    if (spawn.type == "runner")
        return std::make_unique<Runner>(spawn.pos);
    if (spawn.type == "spiny")
        return std::make_unique<Spiny>(spawn.pos);
    if (spawn.type == "boss")
        return std::make_unique<Boss>(spawn.pos, spawn.variant);
    return nullptr;
}

void Game::addItemSpawn(const SpawnRequest& spawn)
{
    auto item = makeItem(spawn);
    if (item)
        m_items.push_back(std::move(item));
}

Boss* Game::bossEnemy()
{
    for (auto& enemy : m_enemies) {
        if (enemy->isBoss())
            return dynamic_cast<Boss*>(enemy.get());
    }
    return nullptr;
}

const Boss* Game::bossEnemy() const
{
    for (const auto& enemy : m_enemies) {
        if (enemy->isBoss())
            return dynamic_cast<const Boss*>(enemy.get());
    }
    return nullptr;
}

sf::Vector2f Game::dungeonStageStart(DungeonStage) const
{
    return {105.0f * Tile, 12.0f * Tile};
}

sf::Vector2f Game::dungeonBossStart() const
{
    return {122.0f * Tile, 13.0f * Tile};
}

void Game::clearDungeonGameplayObjects(bool keepBoss)
{
    m_items.clear();
    m_projectiles.clear();
    m_enemies.erase(std::remove_if(m_enemies.begin(), m_enemies.end(), [keepBoss](const auto& enemy) {
        return !keepBoss || !enemy->isBoss();
    }), m_enemies.end());
}

void Game::clearDungeonLayout()
{
    for (int row = 3; row <= 12; ++row) {
        for (int col = 101; col <= 143; ++col)
            m_level.setTile(row, col, '.');
    }
    for (int row = 8; row <= 12; ++row) {
        m_level.setTile(row, 100, 'L');
        m_level.setTile(row, 144, 'L');
    }
}

void Game::buildDungeonStageLayout(DungeonStage stage)
{
    clearDungeonLayout();
    if (stage == DungeonStage::Stars) {
        for (int col = 106; col <= 110; ++col)
            m_level.setTile(11, col, 'B');
        for (int col = 114; col <= 118; ++col)
            m_level.setTile(9, col, 'B');
        for (int col = 123; col <= 126; ++col)
            m_level.setTile(7, col, 'B');
        for (int col = 132; col <= 136; ++col)
            m_level.setTile(9, col, 'B');
        m_level.setTile(12, 120, '^');
        m_level.setTile(12, 121, '^');
    } else if (stage == DungeonStage::Monsters) {
        for (int col = 107; col <= 112; ++col)
            m_level.setTile(10, col, 'B');
        for (int col = 134; col <= 139; ++col)
            m_level.setTile(10, col, 'B');
    } else if (stage == DungeonStage::BossFight) {
        for (int col = 108; col <= 113; ++col)
            m_level.setTile(9, col, 'B');
        for (int col = 133; col <= 138; ++col)
            m_level.setTile(9, col, 'B');
    }
}

void Game::setupDungeonStage(DungeonStage stage, bool movePlayer)
{
    m_dungeonStage = stage;
    m_pendingDungeonStage = DungeonStage::Outside;
    m_dungeonStageTransitionTimer = 0.0f;
    m_dungeonMessageTimer = 0.0f;
    m_bossItemTimer = 6.0f;
    m_bossFightStarted = stage == DungeonStage::BossFight;
    clearDungeonGameplayObjects(true);
    buildDungeonStageLayout(stage);

    if (Boss* boss = bossEnemy()) {
        const BossState state = stage == DungeonStage::BossFight ? BossState::Awakening : BossState::Sleeping;
        boss->resetForDungeon(dungeonBossStart(), state);
    }

    if (stage == DungeonStage::Stars) {
        addItemSpawn(makeSpawn("star", 109, 10));
        addItemSpawn(makeSpawn("star", 116, 8));
        addItemSpawn(makeSpawn("star", 125, 6));
    } else if (stage == DungeonStage::Monsters) {
        // Koniec nietykalnosci z gwiazdek zebranych w etapie 1 - walka ma byc uczciwa.
        m_player.clearStarPower();
        const std::array<SpawnRequest, 6> mobs{{
            makeSpawn("goomba", 107, 13),
            makeSpawn("koopa", 114, 13),
            makeSpawn("runner", 121, 13),
            makeSpawn("goomba", 129, 13),
            makeSpawn("koopa", 136, 13),
            makeSpawn("flyer", 122, 9),
        }};
        for (const auto& mob : mobs)
            if (auto enemy = makeEnemy(mob))
                m_enemies.push_back(std::move(enemy));
    }

    m_checkpoint = dungeonStageStart(stage);
    if (movePlayer)
        m_player.setPosition(m_checkpoint);
}

void Game::startDungeonStageTransition(DungeonStage nextStage)
{
    if (m_dungeonStageTransitionTimer > 0.0f)
        return;
    m_pendingDungeonStage = nextStage;
    m_dungeonStageTransitionTimer = 1.05f;
    m_dungeonMessageTimer = 1.05f;
    spawnText("Etap ukonczony!", m_player.center() + sf::Vector2f(0.0f, -74.0f), sf::Color(255, 232, 120));
    AudioManager::instance().play("quest");
}

std::string Game::dungeonStageText() const
{
    switch (m_dungeonStage) {
    case DungeonStage::Stars:
        return "ETAP 1: Zbierz 3 gwiazdki i przejdz parkur";
    case DungeonStage::Monsters:
        return "ETAP 2: Pokonaj wszystkie potwory";
    case DungeonStage::BossFight:
        return "ETAP 3: Pokonaj bossa";
    default:
        return "";
    }
}

void Game::updateDungeonStages(float dt)
{
    if (m_currentLevel != 5 || !m_castleInterior || m_dungeonStage == DungeonStage::Outside)
        return;

    if (m_dungeonMessageTimer > 0.0f)
        m_dungeonMessageTimer = std::max(0.0f, m_dungeonMessageTimer - dt);
    if (m_dungeonStageTransitionTimer > 0.0f) {
        m_dungeonStageTransitionTimer = std::max(0.0f, m_dungeonStageTransitionTimer - dt);
        if (m_dungeonStageTransitionTimer == 0.0f && m_pendingDungeonStage != DungeonStage::Outside)
            setupDungeonStage(m_pendingDungeonStage, true);
        return;
    }

    if (m_dungeonStage == DungeonStage::Stars) {
        const int starsLeft = static_cast<int>(std::count_if(m_items.begin(), m_items.end(), [](const auto& item) {
            return item->alive() && item->type() == ItemType::Star;
        }));
        if (starsLeft == 0)
            startDungeonStageTransition(DungeonStage::Monsters);
    } else if (m_dungeonStage == DungeonStage::Monsters) {
        const int monstersLeft = static_cast<int>(std::count_if(m_enemies.begin(), m_enemies.end(), [](const auto& enemy) {
            return enemy->alive() && !enemy->isBoss();
        }));
        if (monstersLeft == 0)
            startDungeonStageTransition(DungeonStage::BossFight);
    } else if (m_dungeonStage == DungeonStage::BossFight) {
        m_bossItemTimer -= dt;
        const Boss* boss = bossEnemy();
        if (boss && boss->state() == BossState::ActiveFight && boss->alive() && m_bossItemTimer <= 0.0f) {
            const int bonusItems = static_cast<int>(std::count_if(m_items.begin(), m_items.end(), [](const auto& item) {
                return item->alive() && item->type() != ItemType::Coin;
            }));
            if (bonusItems < 2) {
                const std::array<sf::Vector2i, 4> spots{{{109, 12}, {110, 8}, {135, 8}, {138, 12}}};
                const std::array<std::string, 4> types{{"heart", "fireflower", "star", "coin"}};
                const auto spot = spots[static_cast<std::size_t>(m_rng() % spots.size())];
                const auto& type = types[static_cast<std::size_t>(m_rng() % types.size())];
                addItemSpawn(makeSpawn(type, spot.x, spot.y));
                spawnBurst({spot.x * Tile + 16.0f, spot.y * Tile + 16.0f}, sf::Color(255, 214, 96), 10);
            }
            m_bossItemTimer = 8.0f + static_cast<float>(m_rng() % 5);
        }
    }
}

void Game::update(float dt)
{
    AudioManager::instance().update();
    if (m_state == AppState::Playing || m_state == AppState::Editor)
        updatePlaying(dt);
    updateParticles(dt);
}

void Game::updatePlaying(float dt)
{
    m_level.update(dt);

    Enemy* boss = nullptr;
    for (const auto& enemy : m_enemies) {
        if (enemy->isBoss()) {
            boss = enemy.get();
            break;
        }
    }
    const bool bossAlive = boss && boss->alive();
    if (m_currentLevel == 5) {
        if (bossAlive && !m_bossFightStarted && m_player.center().x > 92.0f * Tile && m_checkpoint.x < 90.0f * Tile) {
            m_checkpoint = bossCheckpointPosition();
            spawnText("CHECKPOINT", m_player.center() + sf::Vector2f(0.0f, -60.0f), sf::Color(115, 210, 255));
        }
        if (bossAlive && !m_castleInterior && m_castlePhase == CastleCutscenePhase::None && m_player.center().x > 100.0f * Tile) {
            m_castlePhase = CastleCutscenePhase::Opening;
            m_castleTimer = 0.0f;
            m_castleDoorOpen = 0.0f;
            m_checkpoint = bossCheckpointPosition();
        }
        for (int row = 8; row <= 12; ++row) {
            m_level.setTile(row, 100, bossAlive && m_castleInterior ? 'L' : '.');
            m_level.setTile(row, 144, bossAlive && m_castleInterior ? 'L' : '.');
        }
    }

    std::vector<SpawnRequest> blockSpawns;
    auto playerKeys = m_keys;
    if (m_currentLevel == 5 && m_castlePhase != CastleCutscenePhase::None) {
        playerKeys.fill(false);
        m_castleTimer += dt;
        if (m_castlePhase == CastleCutscenePhase::Opening) {
            m_castleDoorOpen = std::clamp(m_castleTimer / 0.9f, 0.0f, 1.0f);
            if (m_castleTimer >= 1.0f) {
                m_castlePhase = CastleCutscenePhase::Entering;
                m_castleTimer = 0.0f;
                m_castleDoorOpen = 1.0f;
            }
        } else if (m_castlePhase == CastleCutscenePhase::Entering) {
            playerKeys[static_cast<std::size_t>(sf::Keyboard::D)] = true;
            if (m_castleTimer >= 1.15f || m_player.center().x > 101.1f * Tile) {
                m_castlePhase = CastleCutscenePhase::FadeOut;
                m_castleTimer = 0.0f;
            }
        } else if (m_castlePhase == CastleCutscenePhase::FadeOut) {
            if (m_castleTimer >= 0.55f) {
                m_castleInterior = true;
                setupDungeonStage(DungeonStage::Stars, true);
                m_castlePhase = CastleCutscenePhase::FadeIn;
                m_castleTimer = 0.0f;
            }
        } else if (m_castlePhase == CastleCutscenePhase::FadeIn && m_castleTimer >= 0.55f) {
            m_castlePhase = CastleCutscenePhase::None;
            m_castleTimer = 0.0f;
        }
    }
    m_player.update(m_level, playerKeys, m_events, m_world, false, blockSpawns, dt);
    for (const auto& spawn : blockSpawns) {
        if (spawn.type == "burst")
            spawnBurst(spawn.pos, sf::Color(210, 120, 70), 12);
        else
            addItemSpawn(spawn);
    }

    for (auto& enemy : m_enemies)
        enemy->update(m_level, m_player, m_projectiles, m_events, m_world, dt);

    for (auto& projectile : m_projectiles)
        projectile.update(m_level, dt);

    for (auto& item : m_items) {
        item->update(m_player, dt);
        const float magnet = item->type() == ItemType::Coin ? m_player.coinMagnetRadius() : 0.0f;
        if (magnet > 0.0f && distance(rectCenter(item->rect()), m_player.center()) < magnet) {
            const sf::Vector2f dir = m_player.center() - rectCenter(item->rect());
            (void)dir;
        }
        if (item->rect().intersects(m_player.rect())) {
            item->collect(m_player, m_events);
            spawnBurst(rectCenter(item->rect()), sf::Color(255, 220, 80), 7);
        }
    }

    updateCollisions();

    m_items.erase(std::remove_if(m_items.begin(), m_items.end(), [](const auto& item) { return !item->alive(); }), m_items.end());
    m_enemies.erase(std::remove_if(m_enemies.begin(), m_enemies.end(), [](const auto& enemy) { return enemy->readyToRemove(); }), m_enemies.end());
    m_projectiles.erase(std::remove_if(m_projectiles.begin(), m_projectiles.end(), [](const Projectile& p) { return !p.alive(); }), m_projectiles.end());
    updateDungeonStages(dt);

    if (m_player.deadAnimationFinished())
        respawnOrGameOver();

    if (m_player.rect().intersects({m_level.flagPosition().x - 20.0f, m_level.flagPosition().y, 80.0f, 360.0f}) && !(m_currentLevel == 5 && bossAlive))
        completeLevel();

    const int checkCol = static_cast<int>(m_player.center().x / Tile);
    if (m_level.tileAt(12, checkCol) == 'C' && checkCol != m_activeCheckpointCol) {
        m_activeCheckpointCol = checkCol;
        m_checkpoint = {checkCol * Tile, 9.0f * Tile};
        m_level.setActiveCheckpoint(checkCol);
        spawnText("CHECKPOINT", m_player.center() + sf::Vector2f(0.0f, -60.0f), sf::Color(115, 210, 255));
    }

    m_autoSaveTimer += dt;
    if (m_autoSaveTimer > 20.0f) {
        m_autoSaveTimer = 0.0f;
        m_saveManager.save(m_save);
    }

    if (m_screenShakeTimer > 0.0f) {
        m_screenShakeTimer = std::max(0.0f, m_screenShakeTimer - dt);
        m_screenShakeStrength = approach(m_screenShakeStrength, 0.0f, 28.0f * dt);
    }

    const sf::Vector2f viewSize = m_worldView.getSize();
    m_camera.x = std::clamp(m_player.center().x - viewSize.x * 0.42f, 0.0f, std::max(0.0f, m_level.width() - viewSize.x));
    m_camera.y = std::clamp(m_player.center().y - viewSize.y * 0.58f, 0.0f, std::max(0.0f, m_level.height() - viewSize.y));
    if (m_currentLevel == 5 && m_castleInterior && bossAlive) {
        const float arenaLeft = 100.0f * Tile;
        const float arenaRight = 145.0f * Tile;
        m_camera.x = std::clamp(m_camera.x, arenaLeft, std::max(arenaLeft, arenaRight - viewSize.x));
    }
    sf::Vector2f shake;
    if (m_screenShakeTimer > 0.0f && m_screenShakeStrength > 0.1f) {
        shake.x = (static_cast<float>(m_rng() % 201) / 100.0f - 1.0f) * m_screenShakeStrength;
        shake.y = (static_cast<float>(m_rng() % 201) / 100.0f - 1.0f) * m_screenShakeStrength * 0.7f;
    }
    m_worldView.setCenter(m_camera.x + viewSize.x * 0.5f + shake.x, m_camera.y + viewSize.y * 0.5f + shake.y);
}

void Game::updateParticles(float dt)
{
    for (auto& particle : m_particles) {
        particle.life -= dt;
        particle.velocity.y += 360.0f * dt;
        particle.pos += particle.velocity * dt;
    }
    m_particles.erase(std::remove_if(m_particles.begin(), m_particles.end(), [](const Particle& particle) { return particle.life <= 0.0f; }), m_particles.end());
}

void Game::updateCollisions()
{
    for (auto& enemy : m_enemies) {
        if (!enemy->alive())
            continue;
        for (auto& projectile : m_projectiles) {
            if (projectile.fromPlayer() && projectile.rect().intersects(enemy->rect())) {
                if (enemy->isBoss()) {
                    if (auto* boss = dynamic_cast<Boss*>(enemy.get())) {
                        if (boss->state() == BossState::ActiveFight) {
                            boss->damage(1, m_events);
                            spawnBurst(rectCenter(projectile.rect()), sf::Color(255, 190, 90), 8);
                        } else {
                            spawnBurst(rectCenter(projectile.rect()), sf::Color(120, 130, 150), 4);
                        }
                    }
                    projectile.destroy();
                    continue;
                }
                enemy->damage(projectile.damage(), m_events);
                projectile.destroy();
            }
        }

        if (enemy->isBoss()) {
            const auto* boss = dynamic_cast<const Boss*>(enemy.get());
            if (boss && boss->state() != BossState::ActiveFight)
                continue;
        }

        if (!enemy->rect().intersects(m_player.rect()))
            continue;
        if (m_player.isInvincible()) {
            enemy->damage(3, m_events);
            m_player.addXp(enemy->isBoss() ? 120 : 25);
            continue;
        }
        const bool stomp = m_player.isFalling() && rectBottom(m_player.rect()) <= enemy->rect().top + enemy->rect().height * 0.62f;
        if (stomp) {
            if (enemy->stomp(m_player, m_events)) {
                m_player.addXp(enemy->isBoss() ? 300 : 40);
                spawnBurst(rectCenter(enemy->rect()), sf::Color(170, 255, 110), 10);
            }
        } else if (enemy->harmful()) {
            if (enemy->isBoss() && !m_player.isInvincible())
                m_player.knockbackFrom(rectCenter(enemy->rect()));
            m_player.hurt(m_events);
        }
    }

    for (auto& projectile : m_projectiles) {
        if (!projectile.fromPlayer() && projectile.rect().intersects(m_player.rect())) {
            if (!m_player.isInvincible())
                m_player.knockbackFrom(rectCenter(projectile.rect()), 220.0f, 230.0f);
            m_player.hurt(m_events);
            projectile.destroy();
        }
    }

    if (const Boss* boss = bossEnemy()) {
        if (boss->fireActive() && boss->fireRect().intersects(m_player.rect())) {
            if (!m_player.isInvincible())
                m_player.knockbackFrom(rectCenter(boss->rect()), 360.0f, 250.0f);
            m_player.hurt(m_events);
        }
    }
}

void Game::completeLevel()
{
    m_achievements.evaluate(m_player.stats(), m_currentLevel);
    m_save.completedLevels.insert(m_currentLevel);
    m_save.unlockedLevel = std::max(m_save.unlockedLevel, std::min(5, m_currentLevel + 1));
    m_save.bankCoins += m_player.coins();
    m_save.bestScore = std::max(m_save.bestScore, m_player.xp() + m_player.coins() * 10 + m_player.stats().kills * 100);
    const float oldTime = m_save.bestTimes.count(m_currentLevel) ? m_save.bestTimes[m_currentLevel] : 9999.0f;
    if (m_player.stats().time < oldTime)
        m_save.bestTimes[m_currentLevel] = m_player.stats().time;
    m_save.bestCoins[m_currentLevel] = std::max(m_save.bestCoins[m_currentLevel], m_player.coins());
    m_saveManager.save(m_save);
    m_events.publish({EventType::LevelCompleted, m_currentLevel, "level"});
    m_ending = m_quests.completedCount() >= 3 ? "Hero Ending" : "Balanced Ending";
    setState(AppState::Newspaper);
}

void Game::respawnOrGameOver()
{
    if (!m_player.alive()) {
        setState(AppState::GameOver);
        return;
    }

    if (m_currentLevel == 5 && m_castleInterior && m_dungeonStage != DungeonStage::Outside) {
        const DungeonStage stage = m_dungeonStage;
        m_castlePhase = CastleCutscenePhase::None;
        m_castleDoorOpen = 0.0f;
        m_castleTimer = 0.0f;
        m_screenShakeTimer = 0.0f;
        m_screenShakeStrength = 0.0f;
        m_projectiles.clear();
        m_particles.clear();
        setupDungeonStage(stage, false);
    } else if (m_currentLevel == 5 && m_bossFightStarted) {
        m_level.build(m_currentLevel);
        m_checkpoint = bossCheckpointPosition();
        m_bossFightStarted = false;
        m_castleInterior = false;
        m_dungeonStage = DungeonStage::Outside;
        m_pendingDungeonStage = DungeonStage::Outside;
        m_castlePhase = CastleCutscenePhase::None;
        m_castleDoorOpen = 0.0f;
        m_castleTimer = 0.0f;
        m_screenShakeTimer = 0.0f;
        m_screenShakeStrength = 0.0f;
        m_enemies.clear();
        m_items.clear();
        m_projectiles.clear();
        m_particles.clear();
        spawnFromLevel();
    }

    m_player.respawn(m_checkpoint);
}

void Game::buyShopItem(int index)
{
    if (index == 4) {
        setState(AppState::Title);
        return;
    }
    const std::array<int, 4> costs = {25, 35, 50, 60};
    if (m_save.bankCoins < costs[static_cast<std::size_t>(index)]) {
        m_shopMessage = "Za malo monet";
        AudioManager::instance().play("hurt");
        return;
    }
    m_save.bankCoins -= costs[static_cast<std::size_t>(index)];
    if (index == 0) {
        m_save.upgrades.healthLevel += 1;
        m_shopMessage = "Kupiono dodatkowe serce";
    }
    if (index == 1) {
        m_shopMushroomNextRun = true;
        m_shopMessage = "Grzybek aktywny w nastepnym poziomie";
    }
    if (index == 2) {
        m_shopShieldNextRun = true;
        m_shopMessage = "Tarcza aktywna w nastepnym poziomie";
    }
    if (index == 3) {
        m_shopKeyNextRun = true;
        m_shopMessage = "Klucz bonusowy gotowy";
    }
    AudioManager::instance().play("chest");
    m_saveManager.save(m_save);
}

void Game::editorPaint(sf::Vector2i pixel, char tile)
{
    const sf::Vector2f world = m_window.mapPixelToCoords(pixel, m_worldView);
    const int col = static_cast<int>(world.x / Tile);
    const int row = static_cast<int>(world.y / Tile);
    m_level.setTile(row, col, tile);
}

void Game::render()
{
    m_window.clear();
    if (m_state == AppState::Playing || m_state == AppState::Paused || m_state == AppState::Editor)
        drawWorld();
    else {
        m_window.setView(m_uiView);
        sf::RectangleShape bg(m_uiView.getSize());
        bg.setFillColor(sf::Color(20, 28, 48));
        m_window.draw(bg);
    }

    m_window.setView(m_uiView);
    if (m_state == AppState::Title)
        m_menu.draw(m_window, m_assets, m_uiView.getSize(), "Slot " + std::to_string(m_selectedSlot) + "  |  Monety w banku: " + std::to_string(m_save.bankCoins) + (m_adminMode ? "  |  TEST MODE ON  |  F5 Level 5" : "  |  F9 Test Mode"));
    else if (m_state == AppState::LevelSelect)
        drawLevelSelect();
    else if (m_state == AppState::Settings)
        drawSettings();
    else if (m_state == AppState::Shop)
        drawShop();
    else if (m_state == AppState::Achievements)
        drawAchievements();
    else if (m_state == AppState::Paused)
        m_menu.draw(m_window, m_assets, m_uiView.getSize(), "ENTER wybiera | P Resume | R Restart | Q Exit");
    else if (m_state == AppState::Newspaper)
        drawNewspaper();
    else if (m_state == AppState::GameOver)
        drawGameOver();
    else if (m_state == AppState::Victory)
        drawVictory();
    else if (m_state == AppState::Editor)
        drawEditor();

    m_window.display();
}

void Game::drawWorld()
{
    m_window.setView(m_worldView);
    m_level.draw(m_window, m_assets, m_world, false, m_time, m_castleInterior);
    drawCastleGate();
    for (const auto& item : m_items)
        item->draw(m_window, m_assets, m_time);
    for (const auto& enemy : m_enemies) {
        if (m_currentLevel == 5 && enemy->isBoss() && !m_castleInterior)
            continue;
        enemy->draw(m_window, m_assets, m_time);
    }
    for (const auto& projectile : m_projectiles)
        projectile.draw(m_window, m_assets, m_time);
    m_player.draw(m_window, m_assets, m_time);
    drawParticles();
    drawSecretRoomDarkness();
    m_window.setView(m_uiView);
    drawHud();
    drawDungeonStagePanel();
    drawMinimap();
    drawBossBar();
    drawCastleFade();
}

void Game::ensureLightMask()
{
    if (m_lightMask.getSize().x != 0)
        return;
    constexpr unsigned S = 256;
    sf::Image img;
    img.create(S, S, sf::Color(255, 255, 255, 0));
    const float c = (S - 1) * 0.5f;
    for (unsigned y = 0; y < S; ++y) {
        for (unsigned x = 0; x < S; ++x) {
            const float dx = (static_cast<float>(x) - c) / c;
            const float dy = (static_cast<float>(y) - c) / c;
            float a = 1.0f - std::sqrt(dx * dx + dy * dy);
            a = std::clamp(a, 0.0f, 1.0f);
            a = a * a; // ostrzejszy spadek - widac tylko najblizsze otoczenie
            img.setPixel(x, y, sf::Color(255, 255, 255, static_cast<sf::Uint8>(a * 255.0f)));
        }
    }
    m_lightMask.loadFromImage(img);
    m_lightMask.setSmooth(true);
}

void Game::drawSecretRoomDarkness()
{
    const sf::Vector2f playerCenter = m_player.center();
    bool inside = false;
    for (const auto& room : m_level.secretRooms()) {
        if (room.contains(playerCenter)) {
            inside = true;
            break;
        }
    }
    if (!inside)
        return;

    ensureLightMask();
    const sf::Vector2u winSize = m_window.getSize();
    if (m_darkness.getSize() != winSize)
        m_darkness.create(winSize.x, winSize.y);

    // Pelna ciemnosc, z ktorej maska swiatla "wycina" miekka dziure wokol gracza.
    m_darkness.clear(sf::Color(2, 3, 10, 255));
    const float maskR = static_cast<float>(m_lightMask.getSize().x) * 0.5f;
    constexpr float lightRadius = 150.0f; // promien poswiaty w pikselach
    const sf::Vector2i pixel = m_window.mapCoordsToPixel(playerCenter, m_worldView);
    sf::Sprite light(m_lightMask);
    light.setOrigin(maskR, maskR);
    light.setPosition(static_cast<float>(pixel.x), static_cast<float>(pixel.y));
    light.setScale(lightRadius / maskR, lightRadius / maskR);
    // Mnozy alfe ciemnosci przez (1 - alfa maski): srodek maski -> przezroczysto, brzeg -> ciemno.
    sf::RenderStates states;
    states.blendMode = sf::BlendMode(sf::BlendMode::Zero, sf::BlendMode::One, sf::BlendMode::Add,
                                     sf::BlendMode::Zero, sf::BlendMode::OneMinusSrcAlpha, sf::BlendMode::Add);
    m_darkness.draw(light, states);
    m_darkness.display();

    // Nakladka ciemnosci na caly ekran.
    m_window.setView(m_window.getDefaultView());
    m_window.draw(sf::Sprite(m_darkness.getTexture()));

    // Ciepla poswiata latarni nad odsloniety obszarem.
    m_window.setView(m_worldView);
    for (int i = 0; i < 3; ++i) {
        const float radius = 110.0f - i * 28.0f;
        sf::CircleShape glow(radius, 30);
        glow.setOrigin(radius, radius);
        glow.setPosition(playerCenter);
        glow.setFillColor(sf::Color(255, 210, 140, 20));
        m_window.draw(glow);
    }
}

void Game::drawHud()
{
    const sf::Vector2f size = m_uiView.getSize();
    sf::RectangleShape shadow({size.x - 36.0f, 44.0f});
    shadow.setPosition(20.0f, 12.0f);
    shadow.setFillColor(sf::Color(0, 0, 0, 70));
    m_window.draw(shadow);

    sf::RectangleShape panel({size.x - 36.0f, 44.0f});
    panel.setPosition(18.0f, 9.0f);
    panel.setFillColor(sf::Color(34, 54, 116, 220));
    panel.setOutlineColor(sf::Color(252, 225, 92, 190));
    panel.setOutlineThickness(2.0f);
    m_window.draw(panel);

    if (!drawTextureIcon(m_window, m_assets, "coin", {42.0f, 31.0f}, 23.0f)) {
        sf::CircleShape coin(10.0f, 24);
        coin.setOrigin(10.0f, 10.0f);
        coin.setScale(0.72f, 1.0f);
        coin.setPosition(42.0f, 31.0f);
        coin.setFillColor(sf::Color(255, 214, 68));
        coin.setOutlineColor(sf::Color(122, 78, 24));
        coin.setOutlineThickness(2.0f);
        m_window.draw(coin);
    }
    drawText("x " + std::to_string(m_player.coins()), 18, {58.0f, 18.0f}, sf::Color(255, 244, 192));

    const int hearts = std::max(0, m_player.lives());
    const float heartSize = 23.0f;
    const float heartStep = 27.0f;
    for (int i = 0; i < hearts; ++i)
        drawHeartIcon(m_window, m_assets, {134.0f + i * heartStep, 31.0f}, heartSize, true);
    if (m_player.hasKey())
        drawTextureIcon(m_window, m_assets, "key", {134.0f + hearts * heartStep + 18.0f, 31.0f}, 25.0f);

    drawText("WORLD " + std::to_string(m_currentLevel) + "-1", 17, {std::round(size.x * 0.5f), 31.0f}, sf::Color(255, 244, 192), true);

    const int seconds = static_cast<int>(m_player.stats().time);
    const int timeLeft = std::max(0, 400 - seconds);
    drawText("TIME " + std::to_string(timeLeft), 17, {size.x - 142.0f, 18.0f}, sf::Color(255, 244, 192));
}

void Game::drawBossBar()
{
    if (m_currentLevel != 5 || !m_castleInterior || m_dungeonStage != DungeonStage::BossFight)
        return;

    for (const auto& enemy : m_enemies) {
        if (!enemy->isBoss() || !enemy->alive())
            continue;
        const auto* boss = dynamic_cast<const Boss*>(enemy.get());
        if (boss && boss->state() == BossState::Sleeping)
            continue;
        const sf::Vector2f size = m_uiView.getSize();
        const float frameW = std::clamp(size.x - 300.0f, 420.0f, 560.0f);
        const float frameH = 54.0f;
        const sf::Vector2f pos(std::round(size.x * 0.5f - frameW * 0.5f), 98.0f);

        sf::RectangleShape shadow({frameW + 6.0f, frameH + 6.0f});
        shadow.setPosition(pos.x + 4.0f, pos.y + 5.0f);
        shadow.setFillColor(sf::Color(0, 0, 0, 95));
        m_window.draw(shadow);

        sf::RectangleShape frame({frameW, frameH});
        frame.setPosition(pos);
        frame.setFillColor(sf::Color(30, 18, 28, 232));
        frame.setOutlineColor(sf::Color(255, 224, 92, 220));
        frame.setOutlineThickness(2.0f);
        m_window.draw(frame);

        drawText("BOSS", 20, {pos.x + 56.0f, pos.y + 26.0f}, sf::Color(255, 242, 180), true, 2.0f);

        const float barX = pos.x + 108.0f;
        const float barY = pos.y + 15.0f;
        const float barW = frameW - 130.0f;
        const float barH = 24.0f;
        sf::RectangleShape barBack({barW, barH});
        barBack.setPosition(barX, barY);
        barBack.setFillColor(sf::Color(58, 30, 38));
        barBack.setOutlineColor(sf::Color(16, 10, 18));
        barBack.setOutlineThickness(2.0f);
        m_window.draw(barBack);

        const float hp = std::clamp(enemy->healthPercent(), 0.0f, 1.0f);
        sf::RectangleShape barFill({std::max(0.0f, barW * hp - 4.0f), barH - 4.0f});
        barFill.setPosition(barX + 2.0f, barY + 2.0f);
        barFill.setFillColor(sf::Color(222, 50, 48));
        m_window.draw(barFill);

        sf::RectangleShape shine({std::max(0.0f, barW * hp - 8.0f), 4.0f});
        shine.setPosition(barX + 4.0f, barY + 4.0f);
        shine.setFillColor(sf::Color(255, 164, 116, 125));
        m_window.draw(shine);

        for (int i = 1; i < 6; ++i) {
            sf::RectangleShape tick({1.0f, barH});
            tick.setPosition(barX + barW * (static_cast<float>(i) / 6.0f), barY);
            tick.setFillColor(sf::Color(20, 12, 18, 150));
            m_window.draw(tick);
        }
        return;
    }
}

void Game::drawDungeonStagePanel()
{
    if (m_currentLevel != 5 || !m_castleInterior || m_dungeonStage == DungeonStage::Outside)
        return;

    const sf::Vector2f size = m_uiView.getSize();
    const float panelW = std::clamp(size.x - 360.0f, 470.0f, 620.0f);
    const float panelH = 34.0f;
    const sf::Vector2f pos(std::round(size.x * 0.5f - panelW * 0.5f), 60.0f);

    sf::RectangleShape shadow({panelW + 5.0f, panelH + 5.0f});
    shadow.setPosition(pos.x + 3.0f, pos.y + 4.0f);
    shadow.setFillColor(sf::Color(0, 0, 0, 82));
    m_window.draw(shadow);

    sf::RectangleShape panel({panelW, panelH});
    panel.setPosition(pos);
    panel.setFillColor(sf::Color(12, 16, 30, 215));
    panel.setOutlineColor(sf::Color(255, 225, 105, 180));
    panel.setOutlineThickness(2.0f);
    m_window.draw(panel);

    const std::string text = m_dungeonMessageTimer > 0.0f ? "Etap ukonczony!" : dungeonStageText();
    drawText(text, 16, {pos.x + panelW * 0.5f, pos.y + panelH * 0.52f}, sf::Color(255, 242, 185), true, 1.6f);
}

void Game::drawCastleGate()
{
    if (m_currentLevel != 5 || m_castleInterior || m_castlePhase == CastleCutscenePhase::None)
        return;

    const float centerX = 101.0f * Tile;
    const float groundY = 13.0f * Tile;
    const float open = std::clamp(m_castleDoorOpen, 0.0f, 1.0f);

    const sf::Texture* castleTexture = m_assets.texture("castle_exterior");
    if (castleTexture && castleTexture->getSize().x > 0 && castleTexture->getSize().y > 0) {
        const sf::Vector2u texSize = castleTexture->getSize();
        const float castleW = 52.0f * Tile;
        const float castleScale = castleW / static_cast<float>(texSize.x);
        const float castleH = static_cast<float>(texSize.y) * castleScale;
        const sf::Vector2f castlePos(centerX - castleW * 0.5f, groundY - castleH * 0.88f);

        const int srcX = static_cast<int>(texSize.x * 0.444f);
        const int srcY = static_cast<int>(texSize.y * 0.650f);
        const int srcW = static_cast<int>(texSize.x * 0.112f);
        const int srcH = static_cast<int>(texSize.y * 0.232f);
        const int leftW = std::max(1, srcW / 2);
        const int rightW = std::max(1, srcW - leftW);

        const sf::FloatRect doorRect(
            castlePos.x + static_cast<float>(srcX) * castleScale,
            castlePos.y + static_cast<float>(srcY) * castleScale,
            static_cast<float>(srcW) * castleScale,
            static_cast<float>(srcH) * castleScale);
        const float slide = open * doorRect.width * 0.68f;

        sf::RectangleShape shadow({doorRect.width + 18.0f, doorRect.height + 10.0f});
        shadow.setPosition(doorRect.left - 9.0f, doorRect.top - 4.0f);
        shadow.setFillColor(sf::Color(2, 3, 8, 245));
        m_window.draw(shadow);

        sf::RectangleShape depth({doorRect.width * 0.74f, doorRect.height * 0.92f});
        depth.setOrigin(depth.getSize().x * 0.5f, 0.0f);
        depth.setPosition(doorRect.left + doorRect.width * 0.5f, doorRect.top + doorRect.height * 0.05f);
        depth.setFillColor(sf::Color(0, 0, 0, 255));
        m_window.draw(depth);

        sf::Sprite leftPanel(*castleTexture);
        leftPanel.setTextureRect({srcX, srcY, leftW, srcH});
        leftPanel.setPosition(doorRect.left - slide, doorRect.top);
        leftPanel.setScale((doorRect.width * 0.5f) / static_cast<float>(leftW), doorRect.height / static_cast<float>(srcH));
        m_window.draw(leftPanel);

        sf::Sprite rightPanel(*castleTexture);
        rightPanel.setTextureRect({srcX + leftW, srcY, rightW, srcH});
        rightPanel.setPosition(doorRect.left + doorRect.width * 0.5f + slide, doorRect.top);
        rightPanel.setScale((doorRect.width * 0.5f) / static_cast<float>(rightW), doorRect.height / static_cast<float>(srcH));
        m_window.draw(rightPanel);

        sf::RectangleShape sill({doorRect.width + 22.0f, 5.0f});
        sill.setPosition(doorRect.left - 11.0f, doorRect.top + doorRect.height - 2.0f);
        sill.setFillColor(sf::Color(42, 35, 32, 210));
        m_window.draw(sill);
    } else {
        sf::RectangleShape frame({4.4f * Tile, 5.0f * Tile});
        frame.setPosition(centerX - 2.2f * Tile, groundY - 5.0f * Tile);
        frame.setFillColor(sf::Color(86, 80, 78));
        frame.setOutlineColor(sf::Color(42, 38, 44));
        frame.setOutlineThickness(2.0f);
        m_window.draw(frame);
    }

    if (m_castlePhase == CastleCutscenePhase::Opening || m_castlePhase == CastleCutscenePhase::Entering)
        drawText("WEJSCIE DO ZAMKU", 15, {centerX, groundY - 7.0f * Tile}, sf::Color(255, 232, 150), true, 2.0f);
}

void Game::drawCastleFade()
{
    if (m_castlePhase != CastleCutscenePhase::FadeOut && m_castlePhase != CastleCutscenePhase::FadeIn)
        return;

    float alpha = 0.0f;
    if (m_castlePhase == CastleCutscenePhase::FadeOut)
        alpha = std::clamp(m_castleTimer / 0.55f, 0.0f, 1.0f);
    else
        alpha = 1.0f - std::clamp(m_castleTimer / 0.55f, 0.0f, 1.0f);

    sf::RectangleShape fade(m_uiView.getSize());
    fade.setPosition(0.0f, 0.0f);
    fade.setFillColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(alpha * 255.0f)));
    m_window.draw(fade);
}

void Game::drawMinimap()
{
    const sf::Vector2f size = m_uiView.getSize();
    const float mapW = std::clamp(size.x * 0.26f, 178.0f, 220.0f);
    const float mapH = 28.0f;
    const sf::Vector2f pos(std::round(size.x - mapW - 22.0f), 64.0f);

    sf::RectangleShape shadow({mapW, mapH});
    shadow.setPosition(pos.x + 3.0f, pos.y + 4.0f);
    shadow.setFillColor(sf::Color(0, 0, 0, 75));
    m_window.draw(shadow);

    sf::RectangleShape bg({mapW, mapH});
    bg.setPosition(pos);
    bg.setFillColor(sf::Color(12, 20, 44, 178));
    bg.setOutlineColor(sf::Color(255, 225, 105, 185));
    bg.setOutlineThickness(1.0f);
    m_window.draw(bg);

    const float trackX = pos.x + 11.0f;
    const float trackY = pos.y + 14.0f;
    const float trackW = mapW - 22.0f;
    const float levelWidth = std::max(1.0f, m_level.width());
    const float playerX = std::clamp(m_player.center().x / levelWidth, 0.0f, 1.0f);
    const float flagX = std::clamp(m_level.flagPosition().x / levelWidth, 0.0f, 1.0f);

    sf::RectangleShape track({trackW, 5.0f});
    track.setPosition(trackX, trackY);
    track.setFillColor(sf::Color(58, 82, 126, 210));
    m_window.draw(track);

    sf::RectangleShape progress({trackW * playerX, 5.0f});
    progress.setPosition(trackX, trackY);
    progress.setFillColor(sf::Color(104, 194, 255, 225));
    m_window.draw(progress);
    if (m_activeCheckpointCol >= 0) {
        const float checkpointWorldX = (static_cast<float>(m_activeCheckpointCol) + 0.5f) * Tile;
        const float checkpointX = std::clamp(checkpointWorldX / levelWidth, 0.0f, 1.0f);
        const sf::Vector2f checkpointPos(trackX + checkpointX * trackW, trackY + 2.5f);

        sf::CircleShape checkpoint(5.0f, 4);
        checkpoint.setOrigin(5.0f, 5.0f);
        checkpoint.setPosition(checkpointPos);
        checkpoint.setRotation(45.0f);
        checkpoint.setFillColor(sf::Color(115, 210, 255));
        checkpoint.setOutlineColor(sf::Color(20, 35, 55));
        checkpoint.setOutlineThickness(1.0f);
        m_window.draw(checkpoint);
    }

    sf::ConvexShape flag(3);
    const float flagPosX = trackX + flagX * trackW;
    flag.setPoint(0, {flagPosX, trackY - 8.0f});
    flag.setPoint(1, {flagPosX + 10.0f, trackY - 4.0f});
    flag.setPoint(2, {flagPosX, trackY});
    flag.setFillColor(sf::Color(255, 230, 92));
    m_window.draw(flag);
    sf::RectangleShape pole({2.0f, 17.0f});
    pole.setPosition(flagPosX, trackY - 8.0f);
    pole.setFillColor(sf::Color(245, 245, 230));
    m_window.draw(pole);

    for (const auto& enemy : m_enemies) {
        if (!enemy->isBoss() || !enemy->alive())
            continue;
        const float bossX = std::clamp(rectCenter(enemy->rect()).x / levelWidth, 0.0f, 1.0f);
        sf::ConvexShape marker(4);
        const sf::Vector2f c(trackX + bossX * trackW, trackY + 2.5f);
        marker.setPoint(0, {c.x, c.y - 6.0f});
        marker.setPoint(1, {c.x + 6.0f, c.y});
        marker.setPoint(2, {c.x, c.y + 6.0f});
        marker.setPoint(3, {c.x - 6.0f, c.y});
        marker.setFillColor(sf::Color(228, 52, 58));
        marker.setOutlineColor(sf::Color(255, 232, 180));
        marker.setOutlineThickness(1.0f);
        m_window.draw(marker);
        break;
    }

    sf::CircleShape dot(4.0f, 12);
    dot.setOrigin(4.0f, 4.0f);
    dot.setPosition(trackX + playerX * trackW, trackY + 2.5f);
    dot.setFillColor(sf::Color(255, 250, 160));
    dot.setOutlineColor(sf::Color(45, 26, 20));
    dot.setOutlineThickness(1.0f);
    m_window.draw(dot);
}

void Game::drawParticles()
{
    for (const auto& particle : m_particles) {
        const float alpha = std::clamp(particle.life / particle.total, 0.0f, 1.0f);
        sf::Color color = particle.color;
        color.a = static_cast<sf::Uint8>(255.0f * alpha);
        if (!particle.text.empty()) {
            drawText(particle.text, 14, particle.pos, color, true, 2.0f);
        } else {
            sf::CircleShape shape(particle.radius * alpha, 16);
            shape.setOrigin(particle.radius * alpha, particle.radius * alpha);
            shape.setPosition(particle.pos);
            shape.setFillColor(color);
            m_window.draw(shape);
        }
    }
}

void Game::drawLevelSelect()
{
    sf::Vector2f size = m_uiView.getSize();
    const int unlockedLimit = m_adminMode ? 5 : m_save.unlockedLevel;
    drawText("WYBOR POZIOMU", 42, {size.x * 0.5f, 90.0f}, sf::Color(255, 232, 150), true, 3.0f);
    for (int i = 1; i <= 5; ++i) {
        const float x = size.x * 0.5f - 240.0f + i * 80.0f;
        sf::RectangleShape card({62.0f, 72.0f});
        card.setOrigin(31.0f, 36.0f);
        card.setPosition(x, 240.0f);
        card.setFillColor(i <= unlockedLimit ? sf::Color(42, 82, 124) : sf::Color(50, 50, 60));
        card.setOutlineColor(i == m_selectedLevel ? sf::Color(255, 220, 90) : sf::Color(120, 145, 180));
        card.setOutlineThickness(3.0f);
        m_window.draw(card);
        drawText(std::to_string(i), 30, {x, 232.0f}, sf::Color::White, true);
        drawText(i <= unlockedLimit ? "OPEN" : "LOCK", 11, {x, 266.0f}, sf::Color(230, 235, 245), true);
    }
    drawText(m_adminMode ? "TEST MODE ON | A/D wybiera | ENTER start | F5 Level 5 | F9 off" : "A/D wybiera, ENTER start, ESC powrot | F9 Test Mode", 18, {size.x * 0.5f, 360.0f}, sf::Color(210, 230, 255), true);
}

void Game::drawSettings()
{
    drawText("USTAWIENIA", 42, {m_uiView.getSize().x * 0.5f, 120.0f}, sf::Color(255, 232, 150), true);
    drawText("Glosnosc: " + std::to_string(static_cast<int>(m_save.volume)) + "%", 26, {m_uiView.getSize().x * 0.5f, 230.0f}, sf::Color::White, true);
    drawText("A/D zmienia, ESC powrot. Auto-save wlaczony.", 17, {m_uiView.getSize().x * 0.5f, 300.0f}, sf::Color(210, 230, 255), true);
}

void Game::drawShop()
{
    m_menu.draw(m_window, m_assets, m_uiView.getSize(), "Bank monet: " + std::to_string(m_save.bankCoins) + " | ENTER kupuje | ESC powrot");
    const std::string status = m_shopMessage.empty() ? "Kupione bonusy jednorazowe zadzialaja po starcie poziomu." : m_shopMessage;
    drawText(status, 15, {m_uiView.getSize().x * 0.5f, m_uiView.getSize().y - 45.0f}, sf::Color(255, 232, 150), true);
}
void Game::drawAchievements()
{
    const sf::Vector2f size = m_uiView.getSize();
    drawText("OSIAGNIECIA", 42, {size.x * 0.5f, 78.0f}, sf::Color(255, 232, 150), true, 3.0f);
    drawText("Odblokowane: " + std::to_string(m_save.achievements.size()) + " / 8", 18, {size.x * 0.5f, 126.0f}, sf::Color(210, 230, 255), true);

    sf::RectangleShape panel({std::min(620.0f, size.x - 96.0f), 310.0f});
    panel.setOrigin(panel.getSize().x * 0.5f, panel.getSize().y * 0.5f);
    panel.setPosition(size.x * 0.5f, size.y * 0.53f);
    panel.setFillColor(sf::Color(24, 38, 74, 220));
    panel.setOutlineColor(sf::Color(252, 225, 92, 190));
    panel.setOutlineThickness(2.0f);
    m_window.draw(panel);

    const std::array<std::string, 8> achievements = {
        "Pierwsza moneta",
        "Pogromca potworow",
        "Lowca bossow",
        "Zbierz 100 monet",
        "Bez smierci",
        "Akrobata",
        "Koniec wyprawy",
        "Sekretny pokoj"
    };

    const float left = panel.getPosition().x - panel.getSize().x * 0.5f + 48.0f;
    const float top = panel.getPosition().y - panel.getSize().y * 0.5f + 42.0f;
    for (int i = 0; i < static_cast<int>(achievements.size()); ++i) {
        const std::string& name = achievements[static_cast<std::size_t>(i)];
        const bool unlocked = m_save.achievements.count(name) > 0;
        const sf::Color color = unlocked ? sf::Color(255, 244, 180) : sf::Color(132, 150, 178);
        const std::string prefix = unlocked ? "[X] " : "[ ] ";
        drawText(prefix + name, 19, {left, top + i * 31.0f}, color, false, 1.4f);
    }

    drawText("ENTER lub ESC - powrot", 17, {size.x * 0.5f, size.y - 54.0f}, sf::Color(210, 230, 255), true);
}
void Game::drawNewspaper()
{
    sf::RectangleShape paper({m_uiView.getSize().x - 160.0f, m_uiView.getSize().y - 120.0f});
    paper.setPosition(80.0f, 60.0f);
    paper.setFillColor(sf::Color(239, 229, 196));
    paper.setOutlineColor(sf::Color(80, 62, 40));
    paper.setOutlineThickness(3.0f);
    m_window.draw(paper);
    drawText("GAZETA POZIOMU " + std::to_string(m_currentLevel), 34, {m_uiView.getSize().x * 0.5f, 105.0f}, sf::Color(55, 42, 32), true, 0.0f);
    drawText(m_level.headline(), 20, {120.0f, 165.0f}, sf::Color(55, 42, 32), false, 0.0f);
    drawText("Monety: " + std::to_string(m_player.coins()) + "  Wrogowie: " + std::to_string(m_player.stats().kills) + "  Czas: " + std::to_string(static_cast<int>(m_player.stats().time)) + "s", 18, {120.0f, 220.0f}, sf::Color(55, 42, 32), false, 0.0f);
    const int score = m_player.coins() * 10 + m_player.stats().kills * 100 - static_cast<int>(m_player.stats().time) * 2 - m_player.stats().deaths * 120;
    const std::string rank = score > 2200 ? "S" : score > 1600 ? "A" : score > 1000 ? "B" : score > 550 ? "C" : "D";
    drawText("Ranga: " + rank + "  Sekret: " + std::string(m_player.hasKey() ? "znaleziony" : "-") + "  Zgony: " + std::to_string(m_player.stats().deaths), 18, {120.0f, 260.0f}, sf::Color(55, 42, 32), false, 0.0f);
    drawText("Zakonczenie teraz: " + m_ending, 18, {120.0f, 300.0f}, sf::Color(55, 42, 32), false, 0.0f);
    drawText("ENTER - dalej", 22, {m_uiView.getSize().x * 0.5f, m_uiView.getSize().y - 100.0f}, sf::Color(55, 42, 32), true, 0.0f);
}

void Game::drawGameOver()
{
    drawText("GAME OVER", 54, {m_uiView.getSize().x * 0.5f, 190.0f}, sf::Color(255, 110, 110), true, 4.0f);
    drawText("ENTER - menu", 22, {m_uiView.getSize().x * 0.5f, 270.0f}, sf::Color::White, true);
}

void Game::drawVictory()
{
    drawText("VICTORY", 56, {m_uiView.getSize().x * 0.5f, 150.0f}, sf::Color(255, 232, 150), true, 4.0f);
    drawText(m_ending, 30, {m_uiView.getSize().x * 0.5f, 225.0f}, sf::Color(210, 240, 255), true);
    drawText("Osiagniecia: " + std::to_string(m_save.achievements.size()) + "  Najlepszy wynik: " + std::to_string(m_save.bestScore), 20, {m_uiView.getSize().x * 0.5f, 285.0f}, sf::Color::White, true);
    drawText("ENTER - menu", 20, {m_uiView.getSize().x * 0.5f, 340.0f}, sf::Color::White, true);
}

void Game::drawEditor()
{
    drawText("EDITOR: lewy klik stawia, prawy usuwa, 1-7 tile, F2 gra. Tile: " + std::string(1, m_editorTile), 15, {18.0f, m_uiView.getSize().y - 28.0f}, sf::Color(255, 245, 170));
}

void Game::drawText(const std::string& text, unsigned size, sf::Vector2f pos, sf::Color color, bool center, float outline)
{
    if (!m_assets.hasFont())
        return;
    sf::Text label(text, m_assets.font(), size);
    label.setFillColor(color);
    label.setOutlineColor(sf::Color(10, 14, 24));
    label.setOutlineThickness(outline);
    if (center) {
        const auto bounds = label.getLocalBounds();
        label.setOrigin(bounds.left + bounds.width * 0.5f, bounds.top + bounds.height * 0.5f);
    }
    label.setPosition(pos);
    m_window.draw(label);
}

void Game::spawnBurst(sf::Vector2f pos, sf::Color color, int count)
{
    for (int i = 0; i < count; ++i) {
        const float angle = static_cast<float>(m_rng() % 628) / 100.0f;
        const float speed = 80.0f + static_cast<float>(m_rng() % 160);
        m_particles.push_back({pos, {std::cos(angle) * speed, std::sin(angle) * speed - 80.0f}, color, "", 0.55f, 0.55f, 3.0f + static_cast<float>(m_rng() % 3)});
    }
}

void Game::spawnText(const std::string& text, sf::Vector2f pos, sf::Color color)
{
    m_particles.push_back({pos, {0.0f, -42.0f}, color, text, 1.0f, 1.0f, 2.0f});
}

std::filesystem::path Game::findAssetRoot() const
{
    const auto cwd = std::filesystem::current_path();
    const std::vector<std::filesystem::path> candidates = {
        cwd / "assets",
        cwd / "LanternRun_SFML" / "assets",
        cwd / ".." / "assets",
        cwd / ".." / "LanternRun_SFML" / "assets",
        cwd / ".." / ".." / "assets"
    };
    for (const auto& candidate : candidates) {
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec) && std::filesystem::is_directory(candidate, ec))
            return std::filesystem::weakly_canonical(candidate, ec);
    }
    return cwd / "assets";
}

#include "AchievementManager.hpp"

void AchievementManager::attach(SaveData& save, EventSystem& events)
{
    m_save = &save;
    events.subscribe(EventType::CoinCollected, [this](const GameEvent&) { unlock("Pierwsza moneta"); });
    events.subscribe(EventType::EnemyKilled, [this](const GameEvent&) { unlock("Pogromca potworow"); });
    events.subscribe(EventType::BossDefeated, [this](const GameEvent&) { unlock("Lowca bossow"); });
    events.subscribe(EventType::NpcRescued, [this](const GameEvent&) { unlock("Pomocna dlon"); });
    events.subscribe(EventType::LanternFuelFound, [this](const GameEvent&) { unlock("Straznik latarni"); });
}

void AchievementManager::evaluate(const RunStats& stats, int levelNumber)
{
    if (stats.coins >= 100)
        unlock("Zbierz 100 monet");
    if (stats.deaths == 0 && stats.time > 1.0f)
        unlock("Bez smierci");
    if (stats.jumps >= 50)
        unlock("Akrobata");
    if (levelNumber >= 5)
        unlock("Koniec wyprawy");
}

std::vector<std::string> AchievementManager::popUnlocked()
{
    auto copy = m_recent;
    m_recent.clear();
    return copy;
}

void AchievementManager::unlock(const std::string& id)
{
    if (!m_save)
        return;
    if (m_save->achievements.insert(id).second)
        m_recent.push_back(id);
}

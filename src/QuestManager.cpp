#include "QuestManager.hpp"

#include <algorithm>

void QuestManager::resetForLevel(int level)
{
    m_quests.clear();
    m_quests.push_back({"key", "Znajdz klucz", "Otwiera skrzynie i sekretne przejscia.", false});
    m_quests.push_back({"coins", "Zbierz 100 monet", "Bonus za dokladne przeszukanie mapy.", false});
    if (level >= 3)
        m_quests.push_back({"boss", "Pokonaj bossa", "Boss ma 3 fazy i pasek zdrowia.", false});
}

void QuestManager::onEvent(const GameEvent& event, const RunStats& stats)
{
    if (event.type == EventType::KeyFound)
        complete("key");
    if (event.type == EventType::BossDefeated)
        complete("boss");
    if (stats.coins >= 100)
        complete("coins");
}

const std::vector<Quest>& QuestManager::quests() const
{
    return m_quests;
}

int QuestManager::completedCount() const
{
    return static_cast<int>(std::count_if(m_quests.begin(), m_quests.end(), [](const Quest& quest) {
        return quest.completed;
    }));
}

void QuestManager::complete(const std::string& id)
{
    for (auto& quest : m_quests) {
        if (quest.id == id)
            quest.completed = true;
    }
}

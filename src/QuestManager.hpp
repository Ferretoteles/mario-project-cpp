#pragma once

#include "Core.hpp"

struct Quest {
    std::string id;
    std::string title;
    std::string detail;
    bool completed = false;
};

class QuestManager {
public:
    void resetForLevel(int level);
    void onEvent(const GameEvent& event, const RunStats& stats);
    const std::vector<Quest>& quests() const;
    int completedCount() const;

private:
    void complete(const std::string& id);

    std::vector<Quest> m_quests;
};

#pragma once

#include "Core.hpp"

struct Quest {
    std::string id;
    std::string title;
    bool completed = false;
};

class QuestManager {
public:
    void resetForLevel(int level);
    void onEvent(const GameEvent& event, const RunStats& stats);
    int completedCount() const;

private:
    void complete(const std::string& id);

    std::vector<Quest> m_quests;
};

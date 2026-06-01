#pragma once

#include "Core.hpp"
#include "EventSystem.hpp"

class AchievementManager {
public:
    void attach(SaveData& save, EventSystem& events);
    void evaluate(const RunStats& stats, int levelNumber);
    std::vector<std::string> popUnlocked();

private:
    void unlock(const std::string& id);

    SaveData* m_save = nullptr;
    std::vector<std::string> m_recent;
};

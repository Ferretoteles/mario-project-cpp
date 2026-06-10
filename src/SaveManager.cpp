#include "SaveManager.hpp"

#include <fstream>
#include <regex>
#include <sstream>

SaveManager::SaveManager(std::filesystem::path root)
    : m_root(std::move(root))
{
}

void SaveManager::setRoot(std::filesystem::path root)
{
    m_root = std::move(root);
}

SaveData SaveManager::load(int slot) const
{
    SaveData data;
    data.saveSlot = slot;

    std::ifstream file(slotPath(slot));
    if (!file)
        return data;

    const std::string json((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    data.unlockedLevel = readInt(json, "unlockedLevel", data.unlockedLevel);
    data.bankCoins = readInt(json, "bankCoins", data.bankCoins);
    data.bestScore = readInt(json, "bestScore", data.bestScore);
    data.volume = readFloat(json, "volume", data.volume);
    data.upgrades.jumpLevel = readInt(json, "jumpLevel", 0);
    data.upgrades.speedLevel = readInt(json, "speedLevel", 0);
    data.upgrades.healthLevel = readInt(json, "healthLevel", 0);
    data.upgrades.magnetLevel = readInt(json, "magnetLevel", 0);
    data.upgrades.activeSkin = readString(json, "activeSkin", data.upgrades.activeSkin);
    if (data.upgrades.activeSkin != "Luigi")
        data.upgrades.activeSkin = "Mario";

    std::regex completedRegex(R"("completedLevels"\s*:\s*\[([^\]]*)\])");
    std::smatch match;
    if (std::regex_search(json, match, completedRegex)) {
        std::stringstream ss(match[1].str());
        while (ss.good()) {
            int level = 0;
            char comma = 0;
            ss >> level;
            if (level > 0)
                data.completedLevels.insert(level);
            ss >> comma;
        }
    }

    std::regex achRegex(R"("achievements"\s*:\s*\[([^\]]*)\])");
    if (std::regex_search(json, match, achRegex)) {
        std::regex itemRegex("\"([^\"]+)\"");
        const std::string block = match[1].str();
        for (auto it = std::sregex_iterator(block.begin(), block.end(), itemRegex); it != std::sregex_iterator(); ++it)
            data.achievements.insert((*it)[1].str());
    }

    for (int level = 1; level <= 5; ++level) {
        data.bestTimes[level] = readFloat(json, "bestTime" + std::to_string(level), 9999.0f);
        data.bestCoins[level] = readInt(json, "bestCoins" + std::to_string(level), 0);
    }

    return data;
}

void SaveManager::save(const SaveData& data) const
{
    std::filesystem::create_directories(m_root / "saves");
    std::ofstream file(slotPath(data.saveSlot), std::ios::trunc);
    if (!file)
        return;

    file << "{\n";
    file << "  \"unlockedLevel\": " << data.unlockedLevel << ",\n";
    file << "  \"bankCoins\": " << data.bankCoins << ",\n";
    file << "  \"bestScore\": " << data.bestScore << ",\n";
    file << "  \"volume\": " << data.volume << ",\n";
    file << "  \"jumpLevel\": " << data.upgrades.jumpLevel << ",\n";
    file << "  \"speedLevel\": " << data.upgrades.speedLevel << ",\n";
    file << "  \"healthLevel\": " << data.upgrades.healthLevel << ",\n";
    file << "  \"magnetLevel\": " << data.upgrades.magnetLevel << ",\n";
    file << "  \"activeSkin\": \"" << (data.upgrades.activeSkin == "Luigi" ? "Luigi" : "Mario") << "\",\n";

    for (int level = 1; level <= 5; ++level) {
        const auto time = data.bestTimes.count(level) ? data.bestTimes.at(level) : 9999.0f;
        const auto coins = data.bestCoins.count(level) ? data.bestCoins.at(level) : 0;
        file << "  \"bestTime" << level << "\": " << time << ",\n";
        file << "  \"bestCoins" << level << "\": " << coins << ",\n";
    }

    file << "  \"completedLevels\": [";
    bool first = true;
    for (int level : data.completedLevels) {
        if (!first)
            file << ", ";
        first = false;
        file << level;
    }
    file << "],\n";

    file << "  \"achievements\": [";
    first = true;
    for (const auto& achievement : data.achievements) {
        if (!first)
            file << ", ";
        first = false;
        file << "\"" << achievement << "\"";
    }
    file << "]\n";
    file << "}\n";
}

std::filesystem::path SaveManager::slotPath(int slot) const
{
    return m_root / "saves" / ("slot" + std::to_string(slot) + ".json");
}

int SaveManager::readInt(const std::string& json, const std::string& key, int fallback)
{
    std::regex regex("\"" + key + R"("\s*:\s*(-?\d+))");
    std::smatch match;
    if (std::regex_search(json, match, regex))
        return std::stoi(match[1].str());
    return fallback;
}

float SaveManager::readFloat(const std::string& json, const std::string& key, float fallback)
{
    std::regex regex("\"" + key + R"("\s*:\s*(-?\d+(?:\.\d+)?))");
    std::smatch match;
    if (std::regex_search(json, match, regex))
        return std::stof(match[1].str());
    return fallback;
}
std::string SaveManager::readString(const std::string& json, const std::string& key, const std::string& fallback)
{
    std::regex regex("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    if (std::regex_search(json, match, regex))
        return match[1].str();
    return fallback;
}

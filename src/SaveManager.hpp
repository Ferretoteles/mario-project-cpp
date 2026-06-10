#pragma once

#include "Core.hpp"

#include <filesystem>

class SaveManager {
public:
    explicit SaveManager(std::filesystem::path root = {});

    void setRoot(std::filesystem::path root);
    SaveData load(int slot) const;
    void save(const SaveData& data) const;

private:
    std::filesystem::path slotPath(int slot) const;
    static int readInt(const std::string& json, const std::string& key, int fallback);
    static std::string readString(const std::string& json, const std::string& key, const std::string& fallback);

    std::filesystem::path m_root;
};

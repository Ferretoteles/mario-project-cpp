#pragma once

#include "Core.hpp"

#include <SFML/Audio.hpp>

class AudioManager {
public:
    static AudioManager& instance();

    void initialize(float volume);
    void setVolume(float volume);
    void play(const std::string& id);
    void update();

private:
    AudioManager() = default;

    void createTone(const std::string& id, float startHz, float endHz, float seconds, float gain);

    std::unordered_map<std::string, sf::SoundBuffer> m_buffers;
    std::vector<sf::Sound> m_sounds;
    float m_volume = 60.0f;
    bool m_ready = false;
};

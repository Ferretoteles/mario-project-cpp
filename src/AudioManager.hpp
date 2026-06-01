#pragma once

#include "Core.hpp"

#include <SFML/Audio.hpp>

class AudioManager {
public:
    static AudioManager& instance();

    void initialize(float volume);
    void setVolume(float volume);
    float volume() const;
    void play(const std::string& id);
    void startMusic();
    void stopMusic();
    void update();

private:
    AudioManager() = default;

    void createTone(const std::string& id, float startHz, float endHz, float seconds, float gain);
    void createMusic();

    std::unordered_map<std::string, sf::SoundBuffer> m_buffers;
    std::vector<sf::Sound> m_sounds;
    sf::SoundBuffer m_musicBuffer;
    sf::Sound m_music;
    float m_volume = 60.0f;
    bool m_ready = false;
};

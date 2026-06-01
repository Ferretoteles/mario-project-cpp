#include "AudioManager.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
constexpr float Pi = 3.14159265358979323846f;
constexpr unsigned SampleRate = 44100;
}

AudioManager& AudioManager::instance()
{
    static AudioManager manager;
    return manager;
}

void AudioManager::initialize(float volume)
{
    if (m_ready) {
        setVolume(volume);
        return;
    }

    m_volume = volume;
    createTone("jump", 430.0f, 720.0f, 0.12f, 0.35f);
    createTone("double_jump", 650.0f, 1050.0f, 0.13f, 0.34f);
    createTone("coin", 980.0f, 1320.0f, 0.10f, 0.28f);
    createTone("death", 180.0f, 65.0f, 0.42f, 0.44f);
    createTone("hurt", 260.0f, 95.0f, 0.22f, 0.38f);
    createTone("stomp", 260.0f, 130.0f, 0.12f, 0.34f);
    createTone("power", 520.0f, 1080.0f, 0.30f, 0.30f);
    createTone("boss", 130.0f, 360.0f, 0.26f, 0.35f);
    createTone("quest", 760.0f, 1180.0f, 0.20f, 0.32f);
    createTone("chest", 440.0f, 900.0f, 0.28f, 0.30f);
    createTone("lantern", 300.0f, 780.0f, 0.20f, 0.28f);
    createTone("fuel", 420.0f, 920.0f, 0.24f, 0.26f);
    createTone("menu_move", 380.0f, 520.0f, 0.055f, 0.18f);
    createTone("menu_select", 520.0f, 860.0f, 0.11f, 0.22f);
    createMusic();
    setVolume(volume);
    m_ready = true;
}

void AudioManager::setVolume(float volume)
{
    m_volume = std::clamp(volume, 0.0f, 100.0f);
    for (auto& sound : m_sounds)
        sound.setVolume(m_volume);
    m_music.setVolume(m_volume * 0.36f);
}

float AudioManager::volume() const
{
    return m_volume;
}

void AudioManager::play(const std::string& id)
{
    const auto found = m_buffers.find(id);
    if (found == m_buffers.end())
        return;

    update();
    m_sounds.emplace_back();
    m_sounds.back().setBuffer(found->second);
    m_sounds.back().setVolume(m_volume);
    m_sounds.back().play();
}

void AudioManager::startMusic()
{
    if (m_music.getBuffer() == nullptr && m_musicBuffer.getSampleCount() > 0)
        m_music.setBuffer(m_musicBuffer);
    m_music.setLoop(true);
    m_music.setVolume(m_volume * 0.36f);
    if (m_music.getStatus() != sf::Sound::Playing)
        m_music.play();
}

void AudioManager::stopMusic()
{
    m_music.stop();
}

void AudioManager::update()
{
    m_sounds.erase(std::remove_if(m_sounds.begin(), m_sounds.end(), [](const sf::Sound& sound) {
        return sound.getStatus() == sf::Sound::Stopped;
    }), m_sounds.end());
}

void AudioManager::createTone(const std::string& id, float startHz, float endHz, float seconds, float gain)
{
    const std::size_t count = static_cast<std::size_t>(SampleRate * seconds);
    std::vector<sf::Int16> samples(count);
    float phase = 0.0f;

    for (std::size_t i = 0; i < count; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(SampleRate);
        const float u = seconds > 0.0f ? t / seconds : 0.0f;
        const float freq = startHz + (endHz - startHz) * u;
        phase += 2.0f * Pi * freq / static_cast<float>(SampleRate);
        const float wave = std::sin(phase) * 0.65f + (std::sin(phase) > 0.0f ? 0.35f : -0.35f);
        const float attack = std::clamp(t / 0.012f, 0.0f, 1.0f);
        const float release = std::clamp((seconds - t) / std::max(0.015f, seconds * 0.22f), 0.0f, 1.0f);
        samples[i] = static_cast<sf::Int16>(wave * gain * attack * release * std::numeric_limits<sf::Int16>::max());
    }

    sf::SoundBuffer buffer;
    if (buffer.loadFromSamples(samples.data(), samples.size(), 1, SampleRate))
        m_buffers[id] = std::move(buffer);
}

void AudioManager::createMusic()
{
    constexpr float seconds = 6.0f;
    const std::array<float, 8> notes = {261.63f, 329.63f, 392.0f, 523.25f, 392.0f, 329.63f, 293.66f, 392.0f};
    const std::size_t count = static_cast<std::size_t>(SampleRate * seconds);
    std::vector<sf::Int16> samples(count);

    for (std::size_t i = 0; i < count; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(SampleRate);
        const int step = static_cast<int>(t / 0.375f) % static_cast<int>(notes.size());
        const float f = notes[static_cast<std::size_t>(step)];
        const float lead = std::sin(2.0f * Pi * f * t) * 0.26f;
        const float bass = std::sin(2.0f * Pi * (f * 0.5f) * t) * 0.18f;
        const float pulse = std::sin(2.0f * Pi * 3.0f * t) > 0.0f ? 0.08f : -0.08f;
        const float sample = (lead + bass + pulse) * 0.34f;
        samples[i] = static_cast<sf::Int16>(sample * std::numeric_limits<sf::Int16>::max());
    }

    m_musicBuffer.loadFromSamples(samples.data(), samples.size(), 1, SampleRate);
    m_music.setBuffer(m_musicBuffer);
}

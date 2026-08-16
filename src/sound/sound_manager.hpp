// sound/sound_manager.hpp
#pragma once
#include <SFML/Audio.hpp>
#include <unordered_map>
#include <string>
#include <memory>

class SoundManager {
public:

    struct SoundData {
        std::unique_ptr<sf::SoundBuffer> buffer;
        float defaultVolume = 1.0f;
    };

    static SoundManager& get();

    static void initAllSounds();

    // Load sound with a default volume (0..1). This volume is relative to the SFX category.
    void loadSound(const std::string& key, const std::string& filepath, float defaultVolume = 1.0f);

    // Play a sound effect. 'volumeMultiplier' is an extra factor for this specific call.
    void playSound(const std::string& key, float volumeMultiplier = 1.0f, float pitch = 1.0f, bool loop = false);
    void playDelayedSound(const std::string& key, float delaySeconds, float volumeMultiplier = 1.0f, float pitch = 1.0f, bool loop = false);
    void playFootStep(int speed = 20, float volumeMultiplier = 0.6f, float pitch = 1.0f);
    // Music: play with optional default volume for this track (applied on top of music volume)
    void playMusic(const std::string& key, bool loop = true, float defaultVolume = 1.0f);
    void stopMusic();
    void pauseMusic();
    void resumeMusic();

    // Global volume controls (all in range 0..1)
    void setMasterVolume(float vol);
    void setSFXVolume(float vol);
    void setMusicVolume(float vol);
    void update(float dt); // Call this periodically to clean up finished sounds

private:
    SoundManager() = default;

    std::unordered_map<std::string, SoundData> m_soundData;
    std::vector<std::unique_ptr<sf::Sound>> m_activeSounds;
    std::vector<std::pair<std::unique_ptr<sf::Sound>, int>> m_delayQueue;

    std::unique_ptr<sf::Music> m_currentMusic;
    std::unique_ptr<sf::Sound> m_footstepSound;

    int timer = 0;
    int footstepTimer = 0;
    bool playingFootstep = false;

    float m_masterVol = 1.0f;
    float m_sfxVol   = 1.0f;
    float m_musicVol = 1.0f;
};
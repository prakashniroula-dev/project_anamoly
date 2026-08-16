#include "sound_manager.hpp"
#include <debug/logs.hpp>
#include <algorithm>
#include <vector>

namespace {
  static const std::string MUSIC_DIR = "assets/sound/music/";
  static const std::string SFX_DIR   = "assets/sound/sfx/";
  static const std::unordered_map<std::string, std::string> sfxList = {
    {"ui_click", "ui_click.wav"},
    {"heavy_door_open", "door.wav"},
    {"shot", "shot.wav"},
    {"footsteps", "footsteps.wav"}
  };
  static const std::unordered_map<std::string, std::string> musicList = {
    {"main_menu", "main_menu.wav"},
    {"main_theme", "main_theme.wav"},
  };
};

void SoundManager::initAllSounds() {
  for (const auto& [key, filename] : sfxList) {
    SoundManager::get().loadSound(key, SFX_DIR + filename, 0.8f);
  }
}

SoundManager& SoundManager::get() {
    static SoundManager instance;
    return instance;
}

void SoundManager::playFootStep(int speed, float volumeMultiplier, float pitch) {
  if (playingFootstep) {
    return; // Don't play another footstep sound if one is already playing
  }
  playingFootstep = true;
  if (!m_footstepSound) {
    auto it = m_soundData.find("footsteps");
    if (it == m_soundData.end()) {
      Log::warn << "Footstep sound not loaded.\n";
      return;
    }
    m_footstepSound = std::make_unique<sf::Sound>(*it->second.buffer);
  }
  float finalVol = m_masterVol * m_sfxVol * volumeMultiplier;
  m_footstepSound->setLooping(true);
  m_footstepSound->setVolume(finalVol * 100.f);
  m_footstepSound->setPitch(pitch);
  m_footstepSound->play();
}

void SoundManager::loadSound(const std::string& key, const std::string& filepath, float defaultVolume) {
    SoundData data;
    data.buffer = std::make_unique<sf::SoundBuffer>();
    if (!data.buffer->loadFromFile(filepath)) {
        Log::error << "Failed to load sound: " << filepath << "\n";
        return;
    }
    data.defaultVolume = std::clamp(defaultVolume, 0.0f, 1.0f);
    m_soundData[key] = std::move(data);
}

void SoundManager::playSound(const std::string& key, float volumeMultiplier, float pitch, bool loop) {
    auto it = m_soundData.find(key);
    if (it == m_soundData.end()) {
        Log::warn << "Sound not loaded: " << key << "\n";
        return;
    }

    auto sound = std::make_unique<sf::Sound>(*it->second.buffer);
    float finalVol = m_masterVol * m_sfxVol * it->second.defaultVolume * volumeMultiplier;
    sound->setVolume(finalVol * 100.f);
    sound->setPitch(pitch);
    sound->setLooping(loop);
    sound->play();

    m_activeSounds.push_back(std::move(sound));
}

void SoundManager::playDelayedSound(const std::string& key, float delaySeconds, float volumeMultiplier, float pitch, bool loop) {
    // Create a new thread to handle the delayed playback
    auto it = m_soundData.find(key);
    if (it == m_soundData.end()) {
        Log::warn << "Sound not loaded: " << key << "\n";
        return;
    }
    auto sound = std::make_unique<sf::Sound>(*it->second.buffer);
    float finalVol = m_masterVol * m_sfxVol * it->second.defaultVolume * volumeMultiplier;
    sound->setVolume(finalVol * 100.f);
    sound->setPitch(pitch);
    sound->setLooping(loop);
    // Store the sound in the delay queue with a timer
    m_delayQueue.push_back(std::make_pair(std::move(sound), static_cast<int>(delaySeconds * 1000)));
}

void SoundManager::playMusic(const std::string& key, bool loop, float defaultVolume) {
    if (m_currentMusic) {
        m_currentMusic->stop();
        m_currentMusic.reset();
    }
    m_currentMusic = std::make_unique<sf::Music>();
    auto it = musicList.find(key);
    if (it == musicList.end()) {
        Log::error << "Music not found: " << key << "\n";
        return;
    }
    std::string path = MUSIC_DIR + it->second; // adjust extension
    if (!m_currentMusic->openFromFile(path)) {
        Log::error << "Failed to load music: " << key << "\n";
        return;
    }
    float finalVol = m_masterVol * m_musicVol * defaultVolume;
    m_currentMusic->setVolume(finalVol * 100.f);
    m_currentMusic->setLooping(loop);
    m_currentMusic->play();
}

void SoundManager::stopMusic() {
    if (m_currentMusic) m_currentMusic->stop();
}

void SoundManager::pauseMusic() {
    if (m_currentMusic) m_currentMusic->pause();
}

void SoundManager::resumeMusic() {
    if (m_currentMusic) m_currentMusic->play();
}

void SoundManager::setMasterVolume(float vol) {
    m_masterVol = std::clamp(vol, 0.0f, 1.0f);
    // Update currently playing music volume if needed
    if (m_currentMusic && m_currentMusic->getStatus() == sf::SoundSource::Status::Playing) {
        float finalVol = m_masterVol * m_musicVol * 1.0f; // track default stored elsewhere
        // You can store track default volume in a map if you want per‑track control.
        m_currentMusic->setVolume(finalVol * 100.f);
    }
}

void SoundManager::update(float dt) {
    timer += 1;

    
    // --- Footstep timer: stop only "footsteps" sounds ---
    const float footstepInterval = 0.25f; // seconds
    footstepTimer += static_cast<int>(dt * 1000);
    if (footstepTimer >= static_cast<int>(footstepInterval * 1000)) {
        footstepTimer = 0;
        playingFootstep = false;
        Log::info << "Footstep sound timer reset. Stopping footstep sound if playing.\n";
        if (m_footstepSound && m_footstepSound->getStatus() == sf::SoundSource::Status::Playing) {
            m_footstepSound->pause();
        }
    }

    auto it = m_activeSounds.begin();
    while (it != m_activeSounds.end()) {
        if ((*it)->getStatus() == sf::SoundSource::Status::Stopped) {
            it = m_activeSounds.erase(it);
        } else {
            ++it;
        }
    }
    // Update the delay queue
    auto delayIt = m_delayQueue.begin();
    while (delayIt != m_delayQueue.end()) {
        delayIt->second -= static_cast<int>(dt * 1000); // dt is in seconds, convert to milliseconds
        if (delayIt->second <= 0) {
            delayIt->first->play();
            m_activeSounds.push_back(std::move(delayIt->first));
            delayIt = m_delayQueue.erase(delayIt);
        } else {
            ++delayIt;
        }
    }
}

void SoundManager::setSFXVolume(float vol) {
    m_sfxVol = std::clamp(vol, 0.0f, 1.0f);
}

void SoundManager::setMusicVolume(float vol) {
    m_musicVol = std::clamp(vol, 0.0f, 1.0f);
    if (m_currentMusic && m_currentMusic->getStatus() == sf::SoundSource::Status::Playing) {
        float finalVol = m_masterVol * m_musicVol * 1.0f;
        m_currentMusic->setVolume(finalVol * 100.f);
    }
}
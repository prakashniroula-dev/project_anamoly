// settings/settings.hpp
#pragma once
#include <string>
#include <fstream>
#include <debug/logs.hpp>
#include <sound/sound_manager.hpp>

class Settings {
public:
    static Settings& get() {
        static Settings instance;
        return instance;
    }

    static constexpr const char* SETTINGS_FILE = "assets/settings.txt";

    void load() {
        std::ifstream file(SETTINGS_FILE);
        if (!file.is_open()) {
            // Use defaults
            return;
        }
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            if (key == "master_volume") masterVolume = std::stof(val);
            else if (key == "music_volume") musicVolume = std::stof(val);
            else if (key == "sfx_volume") sfxVolume = std::stof(val);
            else if (key == "max_fps") maxFps = std::stoi(val);
            // add other settings as needed
        }
    }

    void apply() {
        // Apply settings to relevant systems
        SoundManager::get().setMasterVolume(masterVolume);
        SoundManager::get().setMusicVolume(musicVolume);
        SoundManager::get().setSFXVolume(sfxVolume);
        // Apply FPS limit if needed
    }

    void save() {
        std::ofstream file(SETTINGS_FILE);
        if (!file.is_open()) {
            Log::error << "Failed to save " << SETTINGS_FILE << "\n";
            return;
        }
        file << "master_volume=" << masterVolume << "\n";
        file << "music_volume=" << musicVolume << "\n";
        file << "sfx_volume=" << sfxVolume << "\n";
        file << "max_fps=" << maxFps << "\n";
    }

    static void init() {
        get().load();
    }

    // Public members (or use getters/setters)
    float masterVolume = 1.0f;
    float musicVolume  = 1.0f;   // 0..1
    float sfxVolume    = 1.0f;   // 0..1
    int   maxFps       = 0;      // 0 = unlimited
};

// In main.cpp or Game constructor, call: Settings::get().load();
// game/save_game.hpp
#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <SFML/System/Vector2.hpp>

struct NPCState {
    bool autoTalked = false;
    bool talked = false;
    // extend later if needed (e.g., script progress)
};

struct SaveGame {
    std::string mapName;
    sf::Vector2f playerPos;          // unscaled world coordinates
    std::string playerCharacter;     // key like "fighter_detective"
    std::unordered_map<std::string, NPCState> npcStates;

    // Story state
    std::unordered_map<std::string, bool> flags;
    std::unordered_map<std::string, bool> items;
    std::vector<std::string> choicesMade;

    // Save/load to file
    bool save(const std::string& filepath) const;
    bool load(const std::string& filepath);
};
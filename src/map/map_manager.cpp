#include "map_manager.hpp"
#include <map/terrain.hpp>
#include <entities/npc_manager.hpp>
#include <entities/player.hpp>
#include <core/constants.hpp>
#include <debug/logs.hpp>
#include <fstream>
#include <sstream>
#include <cmath>
#include <ui/transition_screen.hpp>
#include <ui/ui_manager.hpp>
#include <game/game.hpp>

MapManager& MapManager::get() {
    static MapManager instance;
    return instance;
}

std::filesystem::path MapManager::getPath(const std::string& filename) const {
    return std::filesystem::path(baseDir) / currentMap / filename;
}

void MapManager::clearCurrentMapData() {
    Terrain::setMap(TileMap{});
    Terrain::clearObjects();
    Terrain::clearSolidMap();
    Terrain::clearSpawns();
    transitions.clear();
    NPCManager::get().clearAll();
}

// map/map_manager.cpp - loadMap()
void MapManager::loadMap(const std::string& mapName) {
    currentMap = mapName;
    Log::info << "Loading map: " << currentMap << "\n";

    // Load terrain layers
    Terrain::loadFromFile(getPath("map.txt").string());
    Terrain::loadObjectsFromFile(getPath("objects.txt").string());
    Terrain::loadSolidFromFile(getPath("solid_tiles.txt").string());
    Terrain::loadSpawnsFromFile(getPath("spawns.txt").string());

    // Load transitions
    transitions.clear();
    std::ifstream file(getPath("transitions.txt").string());
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream iss(line);
            float x, y, x2, y2;
            std::string target, label;
            if (iss >> x >> y >> target >> label) {
                Transition tr;
                tr.triggerPos = sf::Vector2f(x, y);
                tr.targetMap = target;
                tr.label = label;
                if (iss >> x2 >> y2) {
                    tr.spawnPosition = sf::Vector2f(x2, y2);
                }
                transitions.push_back(tr);
            } else {
                Log::warn << "Invalid transition line: " << line << "\n";
            }
        }
        file.close();
        Log::info << "Loaded " << transitions.size() << " transitions.\n";
    } else {
        Log::info << "No transitions file found.\n";
    }
}

// ---------- Fixed saveCurrentMap ----------
void MapManager::saveCurrentMap() const {
    Log::info << "Saving map: " << currentMap << "\n";

    Terrain::saveToFile(getPath("map.txt").string());
    Terrain::saveObjectsToFile(getPath("objects.txt").string());
    Terrain::saveSolidToFile(getPath("solid_tiles.txt").string());
    Terrain::saveSpawnsToFile(getPath("spawns.txt").string());

    std::ofstream file(getPath("transitions.txt").string());
    if (file.is_open()) {
        for (const auto& tr : transitions) {
            file << tr.triggerPos.x << " " << tr.triggerPos.y << " "
                 << tr.targetMap << " " << tr.label;
            if (tr.spawnPosition.has_value()) {
                file << " " << tr.spawnPosition->x << " " << tr.spawnPosition->y;
            }
            file << "\n";
        }
        file.close();
    } else {
        Log::error << "Failed to save transitions.txt\n";
    }
}

std::optional<Transition> MapManager::getTransitionAt(const sf::Vector2f& position, float threshold) const {
    for (auto& tr : transitions) {
        sf::Vector2f triggerPos = tr.triggerPos * Scale::get(); // Scale trigger position to world coordinates
        sf::Vector2f diff = triggerPos - position;
        float dY = std::abs(diff.y);
        float dX = std::abs(diff.x);
        if (dX <= threshold && dY <= threshold) {
            return tr;
        }
    }
    return std::nullopt;
}

void MapManager::spawnNPCs() {
    Log::Scope scope("MapManager::spawnNPCs");
    scope.info << "Spawning NPCs for map: " << currentMap << "\n";
    NPCManager::spawnAllNPCs();
    for (NPC* npc : NPCManager::get().getAllNPCs()) {
        npc->snapToGround();
    }
}

void MapManager::switchToMap(const std::string& mapName, std::optional<sf::Vector2f> spawnPos) {
    if (mapName == currentMap) {
        Log::warn << "Already on map " << mapName << ", ignoring transition.\n";
        return;
    }

    Log::info << "Switching from " << currentMap << " to " << mapName << "\n";

    auto action = [this, mapName, spawnPos](TransitionScreen& screen) {
        Log::info << "TransitionScreen: action called for map switch to " << mapName << "\n";
        // 1. Clear old data
        clearCurrentMapData();

        // 2. Load new map (without spawning NPCs)
        loadMap(mapName);

        spawnNPCs();
        // 3. Reposition player
        if (m_player) {
            if (spawnPos.has_value()) {
                m_player->resetToPosition(*spawnPos);
                Log::info << "Player placed at custom spawn (" << spawnPos->x << ", " << spawnPos->y << ")\n";
            } else {
                m_player->resetToSpawn();
                m_player->snapToGround();
                Log::info << "Player reset to map default spawn.\n";
            }
        } else {
            Log::warn << "No player pointer in MapManager; cannot reposition player.\n";
        }
        if (m_game) {
            m_game->snapCameraToPlayer();
        }

        Log::info << "continueTransition() called after map switch.\n";
        screen.continueTransition();
    };

    UIManager::get().pushScreen(std::make_unique<TransitionScreen>(action, 1.f));
}
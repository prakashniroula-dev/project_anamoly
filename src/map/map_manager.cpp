#include <map/terrain.hpp>
#include <entities/npc_manager.hpp>
#include <entities/player.hpp>
#include <core/constants.hpp>
#include <debug/logs.hpp>
#include <ui/transition_screen.hpp>
#include <ui/ui_manager.hpp>
#include <game/game.hpp>
#include <map/types.hpp>   // for Transition
#include <map/map_manager.hpp>
#include <map/map_data.hpp>
#include <story/story_manager.hpp>

MapManager& MapManager::get() {
    static MapManager instance;
    return instance;
}

void MapManager::clearCurrentMapData() {
    m_data.clear();
    NPCManager::get().clearAll();
}

// map/map_manager.cpp
void MapManager::checkCutsceneTriggers(const sf::Vector2f& playerPos) {
    // Log::info << "Checking for triggers from : " << m_data.cutsceneTriggers.size() << "\n";
    for (const auto& trigger : m_data.cutsceneTriggers) {
        // Already triggered?
        if (m_data.triggeredCutscenes.find(trigger.id) != m_data.triggeredCutscenes.end())
            continue;

        // Distance check (unscaled coordinates)
        sf::Vector2f worldPos = trigger.position * Scale::get();
        float dist = std::hypot(playerPos.x - worldPos.x, playerPos.y - worldPos.y);
        if (dist > trigger.radius * Scale::get())
            continue;

        // Look up the NPC
        NPC* npc = NPCManager::get().getNPC(trigger.npcId);
        if (!npc) {
            Log::warn << "Cutscene trigger " << trigger.id << ": NPC '" << trigger.npcId << "' not found.\n";
            continue;
        }

        // Get script
        auto it = ScriptRegistry::scripts.find(trigger.scriptName);
        if (it == ScriptRegistry::scripts.end()) {
            Log::warn << "Cutscene trigger " << trigger.id << ": script '" << trigger.scriptName << "' not found.\n";
            continue;
        }
        bool complete = npc->runSequence(it->second);
        Log::info << "NPC Runsequence result: " << complete << "\n";
        if (complete) {
            m_data.triggeredCutscenes.insert(trigger.id);
        }
    }
}

void MapManager::loadMap(const std::string& mapName) {
    currentMap = mapName;
    Log::info << "Loading map: " << currentMap << "\n";
    clearCurrentMapData();
    std::string mapDir = baseDir + currentMap;
    m_data.loadFromDirectory(mapDir);
    // DO NOT spawn NPCs here – that is done later by spawnNPCs()
}

void MapManager::spawnNPCs() {
    Log::Scope scope("MapManager::spawnNPCs");
    for (const auto& [pos, props] : m_data.spawns) {
        if (props.npcTypeId == "player") continue;

        // Skip if this NPC is marked dead
        if (!props.uniqueID.empty()) {
            std::string deadFlag = "npc_dead_" + props.uniqueID;
            if (StoryManager::get().hasFlag(deadFlag)) {
                Log::info << "Skipping dead NPC: " << props.uniqueID << "\n";
                continue;
            }
        }

        Log::info << "Spawning NPC: " << props.npcTypeId << " at (" << pos.first << ", " << pos.second << ")\n";
        

        NPC* npc = NPCManager::get().createNPC(props, sf::Vector2f(pos.first, pos.second));
        if (npc) {
            if (!props.waypoints.empty()) npc->setWaypoints(props.waypoints);
            if (!props.scriptName.empty()) {
                auto it = ScriptRegistry::scripts.find(props.scriptName);
                if (it != ScriptRegistry::scripts.end()) npc->runSequence(it->second);
            }
        }
    }
    if (m_data.playerSpawnPos != sf::Vector2f(0.f,0.f)) {
        Terrain::setPlayerSpawnPosition(m_data.playerSpawnPos);
    }
}

void MapManager::saveCurrentMap() const {
    std::string mapDir = baseDir + currentMap;
    m_data.saveToDirectory(mapDir);
}

std::optional<Transition> MapManager::getTransitionAt(const sf::Vector2f& position, float threshold) const {
    for (const auto& tr : m_data.transitions) {
        sf::Vector2f trigger = tr.triggerPos * Scale::get();
        if (std::abs(trigger.x - position.x) < threshold && std::abs(trigger.y - position.y) < threshold) {
            return tr;
        }
    }
    return std::nullopt;
}

void MapManager::switchToMap(const std::string& mapName, std::optional<sf::Vector2f> spawnPos) {
    if (m_game) m_game->autoSave();

    if (mapName == currentMap || mapName == "self") {
        auto action = [this, spawnPos](TransitionScreen& screen) {
            auto* player = Player::get().getPlayer();
            if (player) {
                if (spawnPos) player->resetToPosition(*spawnPos);
                else {
                    player->resetToSpawn();
                    player->snapToGround();
                }
            }
            if (m_game) m_game->snapCameraToPlayer();
            screen.continueTransition();
        };
        UIManager::get().pushScreen(std::make_unique<TransitionScreen>(action, 1.f));
        return;
    }

    // we need to clear old NPCs, load new map, reposition player
    auto action = [this, mapName, spawnPos](TransitionScreen& screen) {
        // clear old world state (NPCs, etc.) – but keep player?
        // We'll clear everything and respawn
        clearCurrentMapData();
        loadMap(mapName);
        spawnNPCs();
        // reposition player
        auto* player = Player::get().getPlayer();
        if (player) {
            if (spawnPos) player->resetToPosition(*spawnPos);
            else {
                player->resetToSpawn();
                player->snapToGround();
            }
        }
        if (m_game) {
            m_game->snapCameraToPlayer();
            m_game->autoSave();
        }
        screen.continueTransition();
    };
    UIManager::get().pushScreen(std::make_unique<TransitionScreen>(action, 1.f));
}
#include "npc_manager.hpp"
#include <debug/logs.hpp>
#include <story/script_registry.hpp>
#include <story/story_manager.hpp>

NPCManager& NPCManager::get() {
    static NPCManager instance;
    return instance;
}

void NPCManager::registerType(const NPCType& type) {
    typeRegistry[type.id] = type;
}

NPC* NPCManager::createNPC(const SpawnProps& props, const sf::Vector2f& pos) {

    Log::Scope scope("NPCManager::createNPC");
    // Use props.npcTypeId as typeId
    auto it = typeRegistry.find(props.npcTypeId);
    if (it == typeRegistry.end()) {
        scope.error << "Unknown NPC type: " << props.npcTypeId << std::endl;
        return nullptr;
    }
    const NPCType& type = it->second;
    std::string id = props.uniqueID.empty() ? (props.npcTypeId + "_" + std::to_string(npcStorage.size())) : props.uniqueID;
    auto npc = std::make_unique<NPC>(type, pos, id);
    NPC* raw = npc.get();
    npcStorage.push_back(std::move(npc));
    npcList.push_back(raw);
    npcMap[id] = raw;

    // Apply transform properties
    // You may want to add setScale, setRotation to Character or NPC
    // For now, ignore flips.

    // Run script if present
    if (!props.scriptName.empty()) {
        auto scriptIt = ScriptRegistry::scripts.find(props.scriptName);
        if (scriptIt != ScriptRegistry::scripts.end()) {
            raw->runSequence(scriptIt->second);
            scope.info << "Finished running script '" << props.scriptName << "' for NPC " << id << std::endl;
        } else {
            scope.warn << "Script '" << props.scriptName << "' not found in registry." << std::endl;
        }
    }
    if (!props.waypoints.empty()) {
        scope.info << "Setting waypoints for NPC " << id << std::endl;
        raw->setWaypoints(props.waypoints);
    }

    return raw;
}

void NPCManager::clearAll() {
    npcStorage.clear();
    npcList.clear();
    npcMap.clear();
    pendingAutoTalks.clear();
}

void guardShoot(NPC* npc) {
    if (!npc) return;
    Log::info << "guardShoot() called for NPC at (" << npc->getPosition().x << ", " << npc->getPosition().y << ")\n";
    // Set the character state to Shooting so the animation plays.
    npc->shoot();   // Character::shoot() sets state to Shooting and resets timer
    // Optionally spawn a visual bullet or flash here.
}

void NPCManager::loadDefinitions() {
    // ---- Detective with choices ----
    NPCType detective;
    detective.id = "detective_explainer";
    detective.characterKey = Characters::Fighter_Detective;
    detective.behaviorType = "idle";
    detective.autoStartDialogue = true;
    detective.autoStartDelay = 0.5f;

    
    detective.dialogue = {
      {"detective_mission", "Detective", "Let's catch the anamoly.", "hasFlag(detective_intro)", "", -1, {}},
      {"detective_intro", "Detective", "Ready for the first mission?", "!hasFlag(detective_intro)", "", 1, {
        {"choice1", "Player", "Yes, I'm ready, Let's do this.", "", "setFlag(detective_intro); setFlag(mission_start)", 3, {}},
        {"choice2", "Player", "What mission?", "", "setFlag(mission_info)", 4, {}},
        {"choice3", "Player", "No, I need more time.", "", "setFlag(mission_delay)", 2, {}},
      }},
      {"detective_hurry", "Detective", "We don't have much time, are you ready yet?", "hasFlag(mission_info) && !hasFlag(mission_start)", "", -1, {
        {"choice1", "Player", "Yes, I'm ready, Let's do this.", "", "setFlag(mission_start)", 3, {}},
        {"choice2", "Player", "No, I need more time.", "", "setFlag(mission_delay)", 2, {}},
      }}, // Placeholder for choice branches
      {"detective_not_ready", "Detective", "Come back when you're ready.", "", "", -1, {}},
      {"detective_mission", "Detective", "Let's catch the anamoly.", "", "", -1, {}},
      {"detective_confused", "Detective", "Ah, is that your headaches acting up ?\nI was told about them but didn't expect it to be this bad.", "", "", 5, {}},
      {"detective_confused2", "Detective", "The fugitive anamoly ... remember ?", "", "", 6, {
        {"choice1", "Player", "Yes, somewhat...", "", "", 6, {}},
        {"choice2", "Player", "(Stay silent)", "", "", 6, {}}
      }},
      {"detective_confused3", "Detective",
        "That bastard who has wrecked havoc in the town...\nHe went on a killing spree and has been on the run, laying low for a while.\nWe need to catch him and make him pay for his crimes.",
        "", "", -1, {
          {"choice1", "Player", "I understand, let's catch him.", "", "setFlag(detective_intro)", -1, {}},
          {"choice2", "Player", "(Stay silent)", "", "setFlag(detective_intro)", -1, {}}
        }
      },
    };
    registerType(detective);

    // ---- Guard ----
    NPCType guard;
    guard.id = "generic_guard";
    guard.characterKey = Characters::Fighter_Boss;
    guard.behaviorType = "patrol";
    guard.waypoints = {{5.f,5.f}, {10.f,5.f}, {10.f,10.f}};
    guard.dialogue = {
        {"guard_halt", "Guard", "Halt! Who goes there?", "!hasFlag(met_guard)", "", 1, {
          {"choice1", "Player", "I'm Detective Smith.", "", "setFlag(met_guard)", 2, {}},
          {"choice2", "Player", "None of your business.", "", "setFlag(met_guard); setFlag(angry_guard)", 1, {}}
        }},
        {"guard_angry", "Guard", "Fuck off!", "hasFlag(angry_guard)", "", 2, {}},
        {"guard_friendly", "Guard", "Ah, welcome Detective Smith, You've changed your attire.", "!hasFlag(angry_guard)", "", -1, {}}
    };
    registerType(guard);

    NPCType guardCutscene;
    guardCutscene.id = "guard_cutscene";
    guardCutscene.characterKey = Characters::Fighter_Boss;  // same sprite as generic guard
    guardCutscene.behaviorType = "idle";                   // or "patrol" if you want
    guardCutscene.dialogue = {
        {"guard_shoot_intro", "Guard", "Stop right there, criminal! You're under arrest!", "", "", -1, {}},
        {"guard_shoot_2", "Guard", "Take this!", "", "", -1, {}}
    };
    registerType(guardCutscene);

    FunctionRegistry::registerFunction("guardShoot", guardShoot);
    FunctionRegistry::registerFunction("setFlag_guardshoot_started", [](NPC* npc) {
        StoryManager::get().setFlag("guardshoot_started");
    });

}


const NPCType* NPCManager::getType(const std::string& typeId) const {
    auto it = typeRegistry.find(typeId);
    return (it != typeRegistry.end()) ? &it->second : nullptr;
}

NPC* NPCManager::createNPC(const std::string& typeId, const sf::Vector2f& pos, const std::string& uniqueID) {
    auto it = typeRegistry.find(typeId);
    if (it == typeRegistry.end()) {
        Log::error << "Unknown NPC type: " << typeId << std::endl;
        return nullptr;
    }
    const NPCType& type = it->second;
    std::string id = uniqueID.empty() ? (typeId + "_" + std::to_string(npcStorage.size())) : uniqueID;
    auto npc = std::make_unique<NPC>(type, pos, id);
    NPC* raw = npc.get();
    npcStorage.push_back(std::move(npc));
    npcList.push_back(raw);
    npcMap[id] = raw;

    if (type.autoStartDialogue) {
        pendingAutoTalks.push_back(raw);
    }
    return raw;
}

NPC* NPCManager::getNPC(const std::string& uniqueID) {
    auto it = npcMap.find(uniqueID);
    return (it != npcMap.end()) ? it->second : nullptr;
}

void NPCManager::processAutoTalks() {
    for (NPC* npc : pendingAutoTalks) {
        // Set a callback to mark talked when dialogue ends (optional)
        npc->talk();
    }
    pendingAutoTalks.clear();
}

void NPCManager::update(sf::RenderWindow& win, float dt) {
    for (NPC* npc : npcList) {
        npc->update(win, dt);

        // Auto‑start dialogue if enabled and not already talked
        if (npc->getType().autoStartDialogue && !npc->hasAutoTalked()) {
            if (m_player) {
                sf::Vector2f playerPos = m_player->getPosition();
                sf::Vector2f npcPos = npc->getPosition();
                float dist = std::hypot(playerPos.x - npcPos.x, playerPos.y - npcPos.y);
                if (dist <= npc->getType().talkRadius) {
                    npc->talk();
                    npc->setAutoTalked(true);
                }
            }
        }
    }
}

void NPCManager::draw(sf::RenderWindow& win, float dt) {
    for (NPC* npc : npcList) {
        npc->draw(win, dt);
    }
}

// entities/npc_manager.cpp
NPC* NPCManager::getNearestInteractable(const sf::Vector2f& playerPos) const {
    for (NPC* npc : npcList) {
        sf::Vector2f npcPos = npc->getPosition();
        float dist = std::hypot(playerPos.x - npcPos.x, playerPos.y - npcPos.y);
        bool isAutoTalk = npc->getType().autoStartDialogue && !npc->hasAutoTalked();
        if (dist <= npc->getType().talkRadius && !isAutoTalk) {
            return npc;   // return the first one found (you could also pick the closest)
        }
    }
    return nullptr;
}

void NPCManager::spawnAllNPCs() {
    Log::Scope scope("NPCManager::spawnAllNPCs");
    for (const auto& [pos, props] : Terrain::getSpawnMap()) {
        sf::Vector2f worldPos(pos.first, pos.second);
        if (props.characterKey == "Player" || props.npcTypeId == "player") {
            Terrain::setPlayerSpawnPosition(worldPos);
        } else {
            // NPC spawn
            NPC* npc = NPCManager::get().createNPC(props, worldPos);   // new overload
            if (npc) {
                // Additional setup if needed
            }
        }
    }
    scope.info << "Finished spawning all NPCs." << std::endl;
}

void NPCManager::setPlayer(Character* player) {
    m_player = player;
}

void NPCManager::interact() {
    if (!m_player) return;  // player not set

    sf::Vector2f playerPos = m_player->getPosition();
    for (NPC* npc : npcList) {
        sf::Vector2f npcPos = npc->getPosition();
        float dist = std::hypot(playerPos.x - npcPos.x, playerPos.y - npcPos.y);
        if (dist <= npc->getType().talkRadius) {
            npc->talk();   // start dialogue
            return;        // only interact with the first NPC found
        }
    }
}
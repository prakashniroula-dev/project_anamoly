#include "npc_manager.hpp"
#include <debug/logs.hpp>

NPCManager& NPCManager::get() {
    static NPCManager instance;
    return instance;
}

void NPCManager::registerType(const NPCType& type) {
    typeRegistry[type.id] = type;
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

void NPCManager::spawnAllNPCs() {
    for (const auto& [pos, props] : Terrain::getSpawnMap()) {
    sf::Vector2f worldPos(pos.first, pos.second);
    if (props.characterKey == "Player" || props.npcTypeId == "player") {
        Terrain::setPlayerSpawnPosition(worldPos);
    } else {
        // NPC spawn
        std::string npcType = props.npcTypeId;
        if (!npcType.empty()) {
            NPC* npc = NPCManager::get().createNPC(npcType, worldPos, props.uniqueID);
            if (npc) {
                // Apply transform properties
                // npc->setScale(props.scale); // you need to add setScale to Character
                // npc->setRotation(props.rotation);
                // flipX/flipY handled in draw (or you can apply to sprite)
                // If you need to apply flips, you can store them in NPC or Character.
                // For now, you can just ignore flips for NPCs.
            }
        } else {
            // Backward compatibility: if npcTypeId is empty, treat as old-style NPC?
            // Maybe just ignore or create a generic one.
        }
    }
}
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
#include "npc_manager.hpp"
#include <debug/logs.hpp>
#include <story/script_registry.hpp>
#include <story/story_manager.hpp>
#include <entities/player.hpp>

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

void NPCManager::clearAll(bool keepPlayer) {
    if (!keepPlayer) {
        npcStorage.clear();
        npcList.clear();
        npcMap.clear();
        pendingAutoTalks.clear();
        return;
    }
    auto it = npcStorage.begin();
    while (it != npcStorage.end()) {
        if ((*it)->getUniqueID() == "player") {
            ++it;
            continue;
        }
        NPC* raw = it->get();
        auto listIt = std::find(npcList.begin(), npcList.end(), raw);
        if (listIt != npcList.end()) npcList.erase(listIt);
        npcMap.erase(raw->getUniqueID());
        it = npcStorage.erase(it);
    }
    pendingAutoTalks.clear();
}

void NPCManager::removeNPC(NPC* npc) {
    if (!npc) return;
    // Remove from npcList
    auto listIt = std::find(npcList.begin(), npcList.end(), npc);
    if (listIt != npcList.end()) npcList.erase(listIt);
    // Remove from npcMap
    npcMap.erase(npc->getUniqueID());
    // Remove from npcStorage (unique_ptr)
    auto storageIt = std::find_if(npcStorage.begin(), npcStorage.end(),
        [npc](const std::unique_ptr<NPC>& ptr) { return ptr.get() == npc; });
    if (storageIt != npcStorage.end()) npcStorage.erase(storageIt);
    // Also remove from pendingAutoTalks if present
    auto autoIt = std::find(pendingAutoTalks.begin(), pendingAutoTalks.end(), npc);
    if (autoIt != pendingAutoTalks.end()) pendingAutoTalks.erase(autoIt);
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
    Character* playerChar = Player::get().getPlayer();
    for (NPC* npc : npcList) {
        if (npc == playerChar) continue;   // player controls this NPC – skip its AI
        npc->update(win, dt);
    }
    for (NPC* npc : npcList) {
        if (npc == playerChar) continue;   // skip player
        // Auto‑start dialogue if enabled and not already talked
        if (npc->getType().autoStartDialogue && !npc->hasAutoTalked()) {
            NPC* playerNPC = NPCManager::get().getNPC("player");
            if (playerNPC) {
                sf::Vector2f playerPos = playerNPC->getPosition();
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
    Character* playerChar = Player::get().getPlayer();
    for (NPC* npc : npcList) {
        if (npc == playerChar) continue;
        npc->draw(win, dt);
    }
}

// entities/npc_manager.cpp
NPC* NPCManager::getNearestInteractable(const sf::Vector2f& playerPos, const NPC* exclude) const {
    NPC* best = nullptr;
    float bestDist = std::numeric_limits<float>::max();
    for (NPC* npc : npcList) {
        if (npc == exclude) continue;
        sf::Vector2f diff = npc->getPosition() - playerPos;
        float dist = std::hypot(diff.x, diff.y);
        if (dist <= npc->getType().talkRadius && dist < bestDist) {
            bestDist = dist;
            best = npc;
        }
    }
    return best;
}

void NPCManager::spawnAllNPCs() {
    Log::Scope scope("NPCManager::spawnAllNPCs");
    for (const auto& [pos, props] : Terrain::getSpawnMap()) {
        sf::Vector2f worldPos(pos.first, pos.second);
        if (props.characterKey == "Player" || props.npcTypeId == "player") {
            Terrain::setPlayerSpawnPosition(worldPos);
            continue;
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

void NPCManager::interact() {
    NPC* playerNPC = NPCManager::get().getNPC("player");
    if (!playerNPC) return;  // player not set

    sf::Vector2f playerPos = playerNPC->getPosition();
    for (NPC* npc : npcList) {
        sf::Vector2f npcPos = npc->getPosition();
        float dist = std::hypot(playerPos.x - npcPos.x, playerPos.y - npcPos.y);
        if (dist <= npc->getType().talkRadius) {
            npc->talk();   // start dialogue
            return;        // only interact with the first NPC found
        }
    }
}
#pragma once
#include "npc.hpp"
#include <unordered_map>
#include <vector>
#include <memory>


class NPCManager {
public:
    static NPCManager& get();

    static void spawnAllNPCs();
    
    void interact();
    void setPlayer(Character* player);

    // ---- Load definitions ----
    void loadDefinitions(); // hardcoded for now

    // ---- Create / lookup ----
    NPC* createNPC(const std::string& typeId, const sf::Vector2f& pos, const std::string& uniqueID = "");
    NPC* createNPC(const SpawnProps& props, const sf::Vector2f& pos);
    NPC* getNPC(const std::string& uniqueID);
    const std::vector<NPC*>& getAllNPCs() const { return npcList; }
    const NPCType* getType(const std::string& typeId) const;

    // ---- Update / draw ----
    void update(sf::RenderWindow& win, float dt);
    void draw(sf::RenderWindow& win, float dt);

    // ---- Auto‑talk ----
    void processAutoTalks();

    std::vector<std::string> getTypeIds() const {
        std::vector<std::string> ids;
        for (const auto& pair : typeRegistry) ids.push_back(pair.first);
        return ids;
    }

    void clearAll(); // clears npcStorage, npcList, npcMap, pendingAutoTalks
    NPC* getNearestInteractable(const sf::Vector2f& playerPos) const; // new

private:
    NPCManager() = default;
    std::vector<std::unique_ptr<NPC>> npcStorage;
    std::vector<NPC*> npcList;
    std::unordered_map<std::string, NPC*> npcMap;
    std::unordered_map<std::string, NPCType> typeRegistry;
    std::vector<NPC*> pendingAutoTalks;
    Character* m_player = nullptr;

    void registerType(const NPCType& type);
};
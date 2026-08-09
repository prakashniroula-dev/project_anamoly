#pragma once
#include "characters.hpp"
#include "npc_types.hpp"
#include <functional>

class NPC : public Character {
public:
    NPC(const NPCType& type, const sf::Vector2f& spawnPos, const std::string& uniqueID = "");

    void update(sf::RenderWindow& win, float dt) override;

    // ---- Dialogue ----
    void talk();                          // starts the dialogue
    void setTalkEndCallback(std::function<void(NPC&)> cb) { talkEndCB = cb; }

    // ---- State ----
    const std::string& getUniqueID() const { return uniqueID; }
    const NPCType& getType() const { return type; }
    bool hasTalked() const { return talked; }
    void setTalked(bool t) { talked = t; }

    // ---- Custom callbacks (for special NPCs) ----
    void setUpdateCallback(std::function<void(NPC&, float)> cb) { updateCB = cb; }
    void setTalkStartCallback(std::function<void(NPC&)> cb) { talkStartCB = cb; }
    bool hasAutoTalked() const { return autoTalked; }
    void setAutoTalked(bool val) { autoTalked = val; }

private:
    NPCType type;
    std::string uniqueID;
    bool talked = false;
    bool autoTalked = false;   // new flag

    // Behavior state
    struct BehaviorState {
        enum class Type { Idle, Patrol, Follow, Scripted, Talking } type = Type::Idle;
        sf::Vector2f targetPos;
        float timer = 0.f;
    };
    BehaviorState behaviorState;
    size_t nextWaypoint = 0;

    std::function<void(NPC&)> talkStartCB;
    std::function<void(NPC&)> talkEndCB;
    std::function<void(NPC&, float)> updateCB;

    void updateBehavior(float dt);
    void patrolUpdate(float dt);
    void idleUpdate(float dt);
    void followUpdate(float dt);
    void scriptedUpdate(float dt);
};
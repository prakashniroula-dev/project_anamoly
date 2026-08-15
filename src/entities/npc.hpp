#pragma once
#include "characters.hpp"
#include "npc_types.hpp"
#include <functional>
#include <story/script_registry.hpp>

class NPC : public Character
{
public:
    NPC(const NPCType &type, const sf::Vector2f &spawnPos, const std::string &uniqueID = "");

    void update(sf::RenderWindow &win, float dt) override;

    // ---- Dialogue ----
    void talk(); // starts the dialogue
    void setTalkEndCallback(std::function<void(NPC &)> cb) { talkEndCB = cb; }

    // ---- State ----
    const std::string &getUniqueID() const { return uniqueID; }
    const NPCType &getType() const { return type; }
    bool hasTalked() const { return talked; }
    void setTalked(bool t) { talked = t; }

    // ---- Custom callbacks (for special NPCs) ----
    void setUpdateCallback(std::function<void(NPC &, float)> cb) { updateCB = cb; }
    void setTalkStartCallback(std::function<void(NPC &)> cb) { talkStartCB = cb; }
    bool hasAutoTalked() const { return autoTalked; }
    void setAutoTalked(bool val) { autoTalked = val; }
    void pauseAI(bool paused) { m_aiPaused = paused; }

    // Override waypoints for this NPC instance
    void setWaypoints(const std::vector<sf::Vector2f> &wps)
    {
        m_waypoints = wps;
        m_waypointIndex = 0;
        nextWaypoint = 0;
        if (!wps.empty() && behaviorState.type == BehaviorState::Type::Patrol)
        {
            behaviorState.targetPos = wps[0];
        }
    }

    const std::vector<sf::Vector2f> &getWaypoints() const { return m_waypoints.empty() ? type.waypoints : m_waypoints; }

    // Run a script (sequence)
    void runSequence(const std::vector<Action> &actions);
    void dialogueEnded(); 

private:
private:
    bool m_waitingForDialogue = false;
    bool m_aiPaused = false;
    float m_blockedTimer = 0.f;       // cooldown to prevent rapid direction changes
    bool m_directionReversed = false; // to avoid flipping multiple times per frame
    NPCType type;
    std::string uniqueID;
    bool talked = false;
    bool autoTalked = false; // new flag

    std::vector<Action> m_script; // current script
    size_t m_currentActionIndex = 0;
    float m_actionTimer = 0.f;
    bool m_scriptRunning = false;
    std::string m_scriptedAnim;

    std::vector<sf::Vector2f> m_waypoints; // if non‑empty, used instead of type.waypoints
    size_t m_waypointIndex = 0;
    float m_waitTime = 2.0f;        // how long to wait between waypoints (seconds)
    bool  m_waiting = false;        // true while waiting
    float m_waitTimer = 0.f;        // current remaining wait time
    int   m_nextWaypointAfterWait = 0;

    // ---- Helper for script execution ----
    void executeCurrentAction();
    void advanceScript();

    // Behavior state
    struct BehaviorState
    {
        enum class Type
        {
            Idle,
            Patrol,
            Follow,
            Scripted,
            Talking
        } type = Type::Idle;
        sf::Vector2f targetPos;
        float timer = 0.f;
    };
    BehaviorState behaviorState;
    size_t nextWaypoint = 0;

    std::function<void(NPC &)> talkStartCB;
    std::function<void(NPC &)> talkEndCB;
    std::function<void(NPC &, float)> updateCB;

    void updateBehavior(float dt);
    void patrolUpdate(float dt);
    void idleUpdate(float dt);
    void followUpdate(float dt);
    void scriptedUpdate(float dt);
};
#pragma once
#include <SFML/System/Vector2.hpp>
#include <string>
#include <vector>
#include <optional>
#include <map>

// Forward declare if needed, but we define all here.

struct SpawnProps {
    std::string characterKey;
    float scale = 1.f;
    float rotation = 0.f;
    bool flipX = false;
    bool flipY = false;
    std::string npcTypeId;
    std::string uniqueID;
    std::string scriptName;
    std::vector<sf::Vector2f> waypoints;
};

// map/types.hpp
struct Transition {
    sf::Vector2f triggerPos;
    std::string targetMap;
    std::optional<sf::Vector2f> spawnPosition;
    std::string label;          // e.g., "Door", "Elevator"
    std::string condition;      // e.g., "hasItem(doorKey)"   (empty = always true)
    std::string action;         // executed if condition passes (e.g., "takeItem(doorKey)")
    std::string failMessage;    // shown if condition fails
};


using SolidMap = std::map<std::pair<int, int>, int>;
struct NpcSpawn {
    sf::Vector2f position;
    std::string characterKey;   // matches Characters::... constants
};

// map/types.hpp
struct CutsceneTrigger {
    std::string id;                // unique ID to avoid re-triggering
    sf::Vector2f position;         // world position (unscaled)
    float radius = 100.f;          // detection radius
    std::string scriptName;        // name of script in ScriptRegistry
    std::string npcId;             // which NPC executes the script (must exist)
};

// (Optionally also move ObjectProps here? But it's already in map/objects.hpp, so we keep it there.)
// We'll include map/objects.hpp where needed.
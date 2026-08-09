#pragma once
#include <string>
#include <vector>
#include <SFML/System/Vector2.hpp>

struct DialogueLine {
    std::string id;               // unique ID for this node (optional, but useful for referencing)
    std::string speaker;
    std::string text;
    std::string condition;        // e.g., "hasFlag(met_guard)" or "hasItem(key)"
    std::string action;           // e.g., "setFlag(met_guard)" or "giveItem(key)"
    int nextIndex = -1;           // if >=0, automatically advance to this index in the dialogue vector (only when no options)
    std::vector<DialogueLine> options; // child choices (if non‑empty, these are displayed as selectable options)
};

struct NPCType {
    std::string id;                  // unique type key, e.g. "detective_explainer"
    std::string characterKey;        // sprite key from Characters
    std::string behaviorType;        // "idle", "patrol", "follow", "scripted"
    std::vector<DialogueLine> dialogue;
    std::string scriptName;          // optional, for special logic (e.g., "detective")
    std::vector<sf::Vector2f> waypoints;
    float talkRadius = 100.f;
    bool autoStartDialogue = false;
    float autoStartDelay = 0.f;
};
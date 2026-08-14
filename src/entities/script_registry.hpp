// ScriptRegistry.hpp
#pragma once

#include <vector>
#include <unordered_map>
#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <SFML/System/Vector2.hpp>

class NPC; // forward

enum class ActionType {
    MoveTo,
    Wait,
    LockPlayer,
    UnlockPlayer,
    PlayAnimation,
    ShowDialogue,
    SwapPlayer,
    CallFunction,
    EndSequence
};

struct Action {
    ActionType type;
    float duration = 0.f;
    sf::Vector2f targetPos;
    std::string animKey;
    std::string dialogueId;
    std::string functionName;
    NPC* npc = nullptr;   // for SwapPlayer
};

using Script = std::vector<Action>;

namespace ScriptRegistry {
  extern std::unordered_map<std::string, Script> scripts;
  void init();
}
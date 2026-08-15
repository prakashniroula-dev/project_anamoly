// ScriptRegistry.hpp
#pragma once

#include <vector>
#include <unordered_map>
#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <SFML/System/Vector2.hpp>
#include <functional>
#include <unordered_map>

class NPC; // forward

enum class ActionType {
    MoveTo,
    MoveRelative,
    Wait,
    LockPlayer,
    UnlockPlayer,
    PlayAnimation,
    ShowDialogue,
    SwapPlayer,
    FacePlayer,
    JustToReload,
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

namespace FunctionRegistry {
    using Function = std::function<void(NPC*)>;
    extern std::unordered_map<std::string, Function> functions;
    void registerFunction(const std::string& name, Function func);
}
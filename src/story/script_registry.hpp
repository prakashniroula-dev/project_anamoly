// ScriptRegistry.hpp
#pragma once

#include <vector>
#include <unordered_map>
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
    CheckFlag,
    CheckFlagModeOR,
    CheckFlagModeAND,
    SetFlag,
    EndSequence
};

namespace Action {
    struct Action {
        ActionType type;
        float duration = 0.f;
        sf::Vector2f targetPos;
        std::string animKey;
        std::string labelId;
        std::string extName;
        NPC* npc = nullptr;   // for SwapPlayer
    };

    Action MoveTo(float x, float y);
    Action MoveRelative(float dx, float dy);
    Action Wait(float seconds);
    Action LockPlayer();
    Action UnlockPlayer();
    Action PlayAnimation(const std::string& animKey);
    Action ShowDialogue(const std::string& dialogueId);
    Action SwapPlayer(NPC* npc);
    Action FacePlayer();
    Action CallFunction(const std::string& functionName);
    Action CheckFlag(const std::string& flagName, bool expected);
    Action SetFlag(const std::string& flagName, bool value);
    Action EndSequence();
    inline Action CheckFlagModeOR() {
        return {ActionType::CheckFlagModeOR, 0.f, {}, "", "", "", nullptr};
    }
    inline Action CheckFlagModeAND() {
        return {ActionType::CheckFlagModeAND, 0.f, {}, "", "", "", nullptr};
    }
}


using Script = std::vector<Action::Action>;
namespace ScriptRegistry {
  extern std::unordered_map<std::string, Script> scripts;
  void init();
}

namespace FunctionRegistry {
    using Function = std::function<void(NPC*)>;
    extern std::unordered_map<std::string, Function> functions;
    void registerFunction(const std::string& name, Function func);
}
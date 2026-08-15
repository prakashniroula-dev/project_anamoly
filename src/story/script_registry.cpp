#include <story/script_registry.hpp>

namespace ScriptRegistry {
    std::unordered_map<std::string, Script> scripts;
  }
  
void ScriptRegistry::init() {
  scripts["detective_intro"] = {
    {ActionType::MoveTo, 0, {10.f, 5.f}},   // approach player
    {ActionType::LockPlayer},
    {ActionType::PlayAnimation, 0, {}, "talk"},
    {ActionType::ShowDialogue, 0, {}, "detective_intro"},
    {ActionType::Wait, 1.0f},               // pause after dialogue
    {ActionType::UnlockPlayer},
    {ActionType::EndSequence}
  };

  scripts["guard_cutscene"] = {
    Action::CheckFlag("guardCutsceneDone", false), // ensure the cutscene can run
    Action::LockPlayer(),                       // freeze player input
    Action::FacePlayer(),
    Action::Wait(0.5f),
    Action::MoveRelative(-80.f, 0.0f), // move slightly to the left
    Action::ShowDialogue("guard_shoot_intro"), // show dialogue (ID not used here)
    Action::CallFunction("guardShoot"),
    Action::Wait(0.5f),                       // brief pause after shooting
    Action::SetFlag("guardCutsceneDone", true), // mark cutscene as done
    Action::UnlockPlayer(),                     // restore player control
    Action::EndSequence()
  };
}

namespace FunctionRegistry {
    std::unordered_map<std::string, Function> functions;
    void registerFunction(const std::string& name, Function func) {
        functions[name] = func;
    }
}

namespace Action {
    Action MoveTo(float x, float y) {
        return {ActionType::MoveTo, 0.f, {x, y}};
    }
    Action MoveRelative(float dx, float dy) {
        return {ActionType::MoveRelative, 0.f, {dx, dy}};
    }
    Action Wait(float seconds) {
        return {ActionType::Wait, seconds, {}};
    }
    Action LockPlayer() {
        return {ActionType::LockPlayer, 0.f, {}};
    }
    Action UnlockPlayer() {
        return {ActionType::UnlockPlayer, 0.f, {}};
    }

    Action PlayAnimation(const std::string& animKey) {
        return {ActionType::PlayAnimation, 0.f, {}, animKey};
    }
    
    Action ShowDialogue(const std::string& dialogueId) {
        return {ActionType::ShowDialogue, 0.f, {}, dialogueId};
    }
    
    Action SwapPlayer(const std::string& swapNPCId) {
        return {ActionType::SwapPlayer, 0.f, {}, swapNPCId};
    }
    
    Action FacePlayer() {
        return {ActionType::FacePlayer, 0.f, {}};
    }

    Action CallFunction(const std::string& functionName) {
        return {ActionType::CallFunction, 0.f, {}, "", "", functionName};
    }

    Action SetFlag(const std::string& flagName, bool value) {
        return {ActionType::SetFlag, value == true ? 0.f: 1.f, {}, "", flagName};
    }

    Action CheckFlag(const std::string& flagName, bool value) {
        return {ActionType::CheckFlag, value == true ? 0.f: 1.f, {}, "", flagName};
    }

    Action EndSequence() {
        return {ActionType::EndSequence, 0.f, {}};
    }
}
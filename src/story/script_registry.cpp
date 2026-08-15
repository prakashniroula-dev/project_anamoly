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
    {ActionType::LockPlayer},                       // freeze player input
    {ActionType::FacePlayer},
    {ActionType::Wait, 0.5f},
    {ActionType::MoveRelative, 0.5f, {-80.f, 0.0f}}, // move slightly to the left
    {ActionType::ShowDialogue, 0, {}, "guard_shoot_intro"}, // show dialogue (ID not used here)
    {ActionType::CallFunction, 0.f, {}, "", "", "guardShoot"},
    {ActionType::Wait, 0.5f},                       // brief pause after shooting
    {ActionType::UnlockPlayer},                     // restore player control
    {ActionType::EndSequence}
  };
}

namespace FunctionRegistry {
    std::unordered_map<std::string, Function> functions;
    void registerFunction(const std::string& name, Function func) {
        functions[name] = func;
    }
}
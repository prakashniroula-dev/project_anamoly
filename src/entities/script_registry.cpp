#include <entities/script_registry.hpp>

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
}
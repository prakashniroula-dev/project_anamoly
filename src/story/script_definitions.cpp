#include <story/script_registry.hpp>
#include <ui/flash_screen.hpp>
#include <ui/ui_manager.hpp>
#include <ui/notification.hpp>
#include <sound/sound_manager.hpp>

void ScriptRegistry::init() {
  FunctionRegistry::addFunctions({
      {
          "showDialogTip", [](NPC* npc) {
          Notification::show("Tip", "Select choices by mouse or keyboard (arrow keys + Enter/Space).");
      }},
      {
          "showMovementTip", [](NPC* npc) {
          Notification::show("Tip", "Move with WASD or arrow keys.");
      }},
      {
          "showInspectTip", [](NPC* npc) {
          Notification::show("Tip", "Inspect objects by pressing `F`.");
      }},
      {
        "showFlash", [](NPC* npc) {
          SoundManager::get().playSound("thunder");
          UIManager::get().pushScreen(std::make_unique<FlashScreen>(1.5f, sf::Color::White));
        }},
        {
          "endFlash", [](NPC* npc) {
            SoundManager::get().playSound("thunder");
      }}
  });

  scripts["bunker_cutscene"] = {
    Action::Falseful(),
    Action::EvaluateState("hasDiscoveredClue(bunker_photos) && !hasFlag(bunker_cutscene)"),
    Action::Truthful(),
    Action::LockPlayer(),
    Action::ExecuteState("setFlag(bunker_cutscene)"),
    Action::CallFunction("showFlash"),
    Action::ShowDialog("bunker_cutscene"),
    Action::CallFunction("endFlash"),
    Action::Wait(0.5f),
    Action::UnlockPlayer(),
  };

  scripts["other_photos"] = {
    Action::Falseful(),
    Action::EvaluateState("hasDiscoveredClue(other_photos) && !hasFlag(other_photos)"),
    Action::Truthful(),
    Action::LockPlayer(),
    Action::ExecuteState("setFlag(other_photos)"),
    Action::CallFunction("showFlash"),
    Action::ShowDialog("other_photos"),
    Action::CallFunction("endFlash"),
    Action::Wait(0.5f),
    Action::UnlockPlayer(),
  };

  scripts["memory_flood"] = {
    Action::Falseful(),
    Action::EvaluateState("hasDiscoveredClue(memory_flood) && !hasFlag(memory_flood)"),
    Action::Truthful(),
    Action::LockPlayer(),
    Action::ExecuteState("setFlag(memory_flood)"),
    Action::CallFunction("showFlash"),
    Action::ShowDialog("memory_flood"),
    Action::CallFunction("endFlash"),
    Action::Wait(0.5f),
    Action::UnlockPlayer(),
  };

  scripts["family_photo"] = {
    Action::Falseful(),
    Action::EvaluateState("hasDiscoveredClue(family_photo) && !hasFlag(family_photo)"),
    Action::Truthful(),
    Action::LockPlayer(),
    Action::ExecuteState("setFlag(family_photo)"),
    Action::CallFunction("showFlash"),
    Action::ShowDialog("family_photo"),
    Action::CallFunction("endFlash"),
    Action::Wait(0.5f),
    Action::ShowDialog("family_photo_end_flash"),
    Action::UnlockPlayer(),
    Action::EndSequence()
  };

  scripts["first_phase_end"] = {
    Action::Falseful(),
    Action::EvaluateState("hasFlag(first_phase_end)"),
    Action::Truthful(),
    Action::ShowDialog("first_phase_end"),
    Action::EndSequence()
  };

  scripts["bunker_memory"] = {
    Action::Falseful(),
    Action::EvaluateState("hasDiscoveredClue(bunker_photos) && !hasFlag(bunker_memory)"),
    Action::Truthful(),
    Action::LockPlayer(),
    Action::ExecuteState("setFlag(bunker_memory)"),
    Action::CallFunction("showFlash"),
    Action::ShowDialog("bunker_memory"),
    Action::CallFunction("endFlash"),
    Action::Wait(0.5f),
    Action::UnlockPlayer(),
  };

  scripts["memory_flood"] = {
    Action::Falseful(),
    Action::EvaluateState("hasDiscoveredClue(memory_flood) && !hasFlag(memory_flood)"),
    Action::Truthful(),
    Action::LockPlayer(),
    Action::ExecuteState("setFlag(memory_flood)"),
    Action::CallFunction("showFlash"),
    Action::ShowDialog("memory_flood"),
    Action::CallFunction("endFlash"),
    Action::Wait(0.5f),
    Action::UnlockPlayer(),
    Action::EndSequence()
  };

  scripts["flash"] = {
      Action::Falseful(),
      Action::EvaluateState("hasDiscoveredClue(photos_001) && !hasFlag(flash)"),
      Action::Truthful(),
      Action::LockPlayer(),
      Action::ExecuteState("setFlag(flash)"),
      Action::CallFunction("showFlash"),
      Action::ShowDialog("flash"),
      Action::CallFunction("endFlash"),
      Action::Wait(0.5f),
      Action::ShowDialog("flash_end"),
      Action::UnlockPlayer(),
  };

  scripts["intro"] = {
      Action::Falseful(),
      Action::EvaluateState("!hasFlag(intro) || hasFlag(case0)"),
      Action::Truthful(),
      Action::EvaluateState("!hasFlag(intro)"),
      Action::LockPlayer(),
      Action::ShowDialog("intro"),
      Action::ExecuteState("setFlag(intro)"),
      Action::UnlockPlayer(),
  };

  scripts["case0"] = {
      Action::Falseful(),
      Action::EvaluateState("hasFlag(intro) && !hasFlag(case0)"),
      Action::Truthful(),
      Action::LockPlayer(),
      Action::MoveTowardsPlayer(),
      Action::CallFunction("showDialogTip"),
      Action::ShowDialog("case0"),
      Action::ExecuteState("setFlag(case0)"),
      Action::UnlockPlayer(),
      Action::CallFunction("showMovementTip"),
      Action::Wait(5.f),
      Action::CallFunction("showInspectTip"),
      Action::EndSequence()
  };
}
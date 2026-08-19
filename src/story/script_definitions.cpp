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

  scripts["flash"] = {
      Action::Falseful(),
      Action::EvaluateState("hasDiscoveredClue(photos_001) && !hasFlag(flash)"),
      Action::Truthful(),
      Action::LockPlayer(),
      Action::ExecuteState("setFlag(flash)"),
      Action::CallFunction("showFlash"),
      Action::ShowDialogue("flash"),
      Action::CallFunction("endFlash"),
      Action::Wait(0.5f),
      Action::ShowDialogue("flash_end"),
      Action::UnlockPlayer(),
  };

  scripts["intro"] = {
      Action::Falseful(),
      Action::EvaluateState("!hasFlag(intro)"),
      Action::Truthful(),
      Action::LockPlayer(),
      Action::ShowDialogue("intro"),
      Action::ExecuteState("setFlag(intro)"),
      Action::UnlockPlayer(),
  };

  scripts["case0"] = {
      Action::Falseful(),
      Action::EvaluateState("hasFlag(intro)"),
      Action::Truthful(),
      Action::LockPlayer(),
      Action::MoveTowardsPlayer(),
      Action::CallFunction("showDialogTip"),
      Action::ShowDialogue("case0"),
      Action::UnlockPlayer(),
      Action::CallFunction("showMovementTip"),
      Action::Wait(5.f),
      Action::CallFunction("showInspectTip"),
      Action::EndSequence()
  };
}
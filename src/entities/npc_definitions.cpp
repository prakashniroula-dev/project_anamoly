#include <entities/npc_types.hpp>
#include <entities/npc_manager.hpp>
#include <entities/characters.hpp>

static DialogLine makeDialogLine(const std::string& id) {
    return DialogLine(id);
}

void NPCManager::loadDefinitions() {

  NPCType playerType;
  playerType.id = "player";
  playerType.characterKey = Characters::Fighter_Detective; // default
  playerType.behaviorType = "idle";
  registerType(playerType);

  NPCType dummy;
  dummy.id = "dummy";
  dummy.characterKey = Characters::Fighter_Detective;
  dummy.behaviorType = "idle";
  registerType(dummy);

  NPCType explainer;
  explainer.id = "explainer_npc";
  explainer.characterKey = Characters::Fighter_Boss;
  explainer.behaviorType = "idle";
  explainer.dialogue = {
    makeDialogLine("case0")
    .exchange("David", "Detective Mathew... how's it going with the case ?")
    .setCondition("!hasFlag(case0)")
    .setAction("setFlag(case0)")
    .setOptions({
      makeDialogLine("choice1").exchange("Player", "The case.. ?").next(1),
      makeDialogLine("choice2").exchange("Player", "What case.. ?").next(1)
    }).next(2),

    makeDialogLine("case0_1")
    .exchange("David", "Yeah.. the Anamoly case, that was assigned to you.").next(2),

    makeDialogLine("case0_2")
    .exchange("David", "The deadline is coming near, is it not ?")
    .setCondition("!hasFlag(case0_2)")
    .setAction("setFlag(case0_2)")
    .setOptions({
      makeDialogLine("choice1").exchange("Player", "Oh.. that.. I'm working on it.").next(3),
      makeDialogLine("choice2").exchange("Player", "I need more time.").next(3)
    }).next(4),

    makeDialogLine("case0_3")
    .exchange("David", "Well, the chief is expecting it by this week.").next(4),

    makeDialogLine("case0_4")
    .exchange("David", "Good luck on your case!")
    ,
  };
  registerType(explainer);

  NPCType null;
  null.id = "null";
  null.characterKey = Characters::Fighter_Boss;
  null.behaviorType = "scripted";
  null.cutsceneRadius = 999999.f; // always trigger
  null.dialogue = {
    makeDialogLine("intro").exchange("(You)", ".. Why is my head hurting so much..").next(1)
    .sound("null"),
    makeDialogLine("intro_1").exchange("(You)", "Wait... what was I doing again ?")
    .sound("null"),

    makeDialogLine("flash").exchange("(...)", "No... This can't be happening...")
    .sound("null").next(3),
    makeDialogLine("flash2").exchange("(...)", "Did you enjoy the surprise ..?")
    .sound("npc_talk_angry").next(4),
    makeDialogLine("flash3").exchange("(...)", "I.. I need to get out-")
    .sound("null").next(-1),
    
    makeDialogLine("flash_end").exchange("(You)", "W-What was that..?").next(6)
    .sound("null"),
    makeDialogLine("flash_end2").exchange("(You)", "A hallucination ?").next(7)
    .sound("null"),
    makeDialogLine("flash_end3").exchange("(You)", "Maybe I'm not having good sleep these days..")
    .sound("null").next(8),
    makeDialogLine("flash_end3").exchange("(You)", "I.. need to focus.. I have a deadline.")
    .sound("null"),

  };

  registerType(null);

}


// old

/* 


    // ---- Detective with choices ----
    NPCType detective;
    detective.id = "detective_explainer";
    detective.characterKey = Characters::Fighter_Detective;
    detective.behaviorType = "idle";
    detective.autoStartDialogue = true;
    detective.autoStartDelay = 0.5f;

    
    detective.dialogue = {
      {"detective_intro", "Detective", "Ready for the first mission?", "!hasFlag(detective_intro)", "", 1, {
        {"choice1", "Player", "Yes, I'm ready", "", "setFlag(detective_intro); setFlag(mission_start)", 3, {}},
        {"choice2", "Player", "What mission?", "", "setFlag(mission_info)", 4, {}},
        {"choice3", "Player", "No, I need more time.", "", "setFlag(mission_delay)", 2, {}},
      }, "npc_talk_start1"},
      {"detective_hurry", "Detective", "We don't have much time, are you ready yet?", "hasFlag(mission_info) && !hasFlag(mission_start)", "", 3, {
        {"choice1", "Player", "Yes, I'm ready", "", "setFlag(mission_start)", 3, {}},
        {"choice2", "Player", "No, I need more time.", "", "setFlag(mission_delay)", 2, {}},
      }, "npc_talk_start0"}, // Placeholder for choice branches
      {"detective_not_ready", "Detective", "Come back when you're ready.", "", "", -1, {}, "npc_talk_short1"},
      {"detective_mission", "Detective", "Let's catch the anamoly.", "", "", -1, {}, "npc_talk_short2"},
      {"detective_confused", "Detective", "Ah, is that your headaches acting up ?\nI was told about them but didn't expect it to be this bad.", "", "", 5, {}, "npc_talk_continue2"},
      {"detective_confused2", "Detective", "The fugitive anamoly ... remember ?", "", "", 6, {
        {"choice1", "Player", "Yes, somewhat...", "", "", 6, {}},
        {"choice2", "Player", "(Stay silent)", "", "", 6, {}}
      }, "npc_talk_excited"},
      {"detective_confused3", "Detective",
        "That bastard who has wrecked havoc in the town...\nHe went on a killing spree and has been on the run, laying low for a while.\nWe need to catch him and make him pay for his crimes.",
        "", "", -1, {
          {"choice1", "Player", "I understand, let's catch him.", "", "setFlag(detective_intro)", -1, {}},
          {"choice2", "Player", "(Stay silent)", "", "setFlag(detective_intro)", -1, {}}
        }, "npc_talk_angry"
      },
    };
    registerType(detective);

    // ---- Guard ----
    NPCType guard;
    guard.id = "generic_guard";
    guard.characterKey = Characters::Fighter_Boss;
    guard.behaviorType = "patrol";
    guard.waypoints = {{5.f,5.f}, {10.f,5.f}, {10.f,10.f}};
    guard.dialogue = {
        {"guard_halt", "Guard", "Halt! Who goes there?", "!hasFlag(met_guard)", "", 1, {
          {"choice1", "Player", "I'm Detective Smith.", "", "setFlag(met_guard)", 2, {}},
          {"choice2", "Player", "None of your business.", "", "setFlag(met_guard); setFlag(angry_guard)", 1, {}}
        }},
        {"guard_angry", "Guard", "Fuck off!", "hasFlag(angry_guard)", "", 2, {}},
        {"guard_friendly", "Guard", "Ah, welcome Detective Smith, You've changed your attire.", "!hasFlag(angry_guard)", "", -1, {}},
        {"guard_shoot", "Guard", "Stop right there, criminal! You're under arrest!", "", "", -1, {}},
    };
    registerType(guard);

    NPCType guardCutscene;
    guardCutscene.id = "guard_cutscene";
    guardCutscene.characterKey = Characters::Fighter_Boss;  // same sprite as generic guard
    guardCutscene.behaviorType = "idle";                   // or "patrol" if you want
    guardCutscene.dialogue = {
        {"guard_shoot_intro", "Guard", "Stop right there, criminal! You're under arrest!", "", "", -1, {}},
        {"guard_shoot_2", "Guard", "Take this!", "", "", -1, {}}
    };
    registerType(guardCutscene);

*/
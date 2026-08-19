#include <clue/clue_manager.hpp>


static ClueInfo makeClue(const std::string& id, const std::string& title) {
  ClueInfo info;
  info.id = id;
  info.title = title;
  return info;
}

void ClueManager::registerClues() {
  ClueManager& cm = ClueManager::get();

  cm.addClues({
    makeClue("photos_001", "Anamoly - Old photos")
    .action("setFlag(found_photos)")
    .addPlain("Photo #1")
    .addBullet("An old photo, half torn, the face is missing.")
    .addBullet("wedding clothes, holding hands.")
    .addBullet("Looks about 20 yrs old")
    .addPlain("Photo #2")
    .addBullet("blurry, body is similar to photo #1")
    .addBullet("about 26, black mask and red eyes")
    .addBullet("[color=red]The light reflects red blooded hands[/color]")
    .addPlain("")
    .addHighlight("A gap of 6 years ...? What could've happened?"),

    makeClue("outsideKey", "Old rusty key")
    .addPlain("Looks like it could open a certain door.")
    .type("item"),

    
    makeClue("clue_001", "Mysterious Note")
    .condition("hasFlag(found_photos)")
    .addPlain("A handwritten note, slightly torn.")
    .addBullet("Symbols: [b]three circles[/b] and a [color=#FFA500]line[/color].")
    .addHighlight("[color=#00AA00][u]Could be a map[/u] to the old well.[/color]")
    .addPlain("Also [i]italic[/i] and [b][i]bold-italic[/i][/b] are supported."),

    makeClue("clue_002", "Old Key")
    .addPlain("Rusty iron key.")
    .addBullet("Engraved: 'Gate No. 7'.")
  });
}


// old

/* 


void ClueManager::registerClues() {
    ClueManager& cm = ClueManager::get();

    cm.addClues({
      makeClue("photos_001", "Anamoly - Old photos")
        .action("setFlag(found_photos)")
        .addPlain("A set of old photographs")
        .addBullet("Half torn photo")
        .addBullet("Holding hands with women")
        .addBullet("Looks about 20 yrs old")
        .addPlain("")
        .addBullet("Blurry photo")
        .addBullet("Black mask")
        .addBullet("[color=red]Red eyes and hand[/color]")
        .addPlain("")
        .addHighlight("A gap of 6 years ...? What could've happened?"),

      
      makeClue("clue_001", "Mysterious Note")
        .condition("hasFlag(found_photos)")
        .addPlain("A handwritten note, slightly torn.")
        .addBullet("Symbols: [b]three circles[/b] and a [color=#FFA500]line[/color].")
        .addHighlight("[color=#00AA00][u]Could be a map[/u] to the old well.[/color]")
        .addPlain("Also [i]italic[/i] and [b][i]bold-italic[/i][/b] are supported."),

      makeClue("clue_002", "Old Key")
        .addPlain("Rusty iron key.")
        .addBullet("Engraved: 'Gate No. 7'.")
    });

  //   cm.registerClue({
  //   "clue_001",
  //   "Mysterious Note", "", "",
  //     {
  //         { ParagraphType::Plain, "A handwritten note, slightly torn." },
  //         { ParagraphType::Bullet, "Symbols: [b]three circles[/b] and a [color=#FFA500]line[/color]." },
  //         { ParagraphType::Highlight, "[color=#00AA00][u]Could be a map[/u] to the old well.[/color]" },
  //         { ParagraphType::Plain, "Also [i]italic[/i] and [b][i]bold-italic[/i][/b] are supported." }
  //     }
  // });

  //   cm.registerClue({
  //       "clue_002",
  //       "Old Key", "", "",
  //       {
  //           { ParagraphType::Plain, "Rusty iron key." },
  //           { ParagraphType::Underline, "Engraved: 'Gate No. 7'." }
  //       }
  //   });

    // ... add more as needed
}
*/
#include <clue/clue_manager.hpp>


static ClueInfo makeClue(const std::string& id, const std::string& title) {
  ClueInfo info;
  info.id = id;
  info.title = title;
  return info;
}

void ClueManager::registerClues() {
  ClueManager& cm = ClueManager::get();

  /* Bunker */
  cm.addClues({
    makeClue("other_photos", "Anamoly - Photos")
    .condition("hasDiscoveredClue(bunker_1)")
    .addPlain("Photo #1")
    .addBullet("Half torn wedding photo, faces visible")
    .addBullet("Matches with the photo in the photo album")
    .addPlain("Photo #2")
    .addBullet("Looks like a funeral")
    .addBullet("The face matches women in wedding photos")
    ,

    makeClue("bunker_door", "Bunker door")
    .condition("!hasItem(bunkerKey)")
    .addPlain("I remember this door... the keys..\nI have them in my office back there..")
    .type("item")
    ,

    makeClue("bunker_1", "Anamoly - Bunker")
    .addPlain("This bunker feels awfully familiar, but I can't remember why.")
    .addPlain("I remember something like pictures I laid around here")
    ,

    makeClue("memory_flood", "Workplace")
    .condition("hasDiscoveredClue(other_photos)")
    .addPlain("I.. I remember this workplace")
    .addPlain("I used.. to.. work here..")
  });

  /* Organization Clues */
  cm.addClues({
    makeClue("org_secret_recording1", "Anamoly - Voice recording #1")
    .addPlain("A secret voice exchange between Lucid, Inc. members")
    .addBullet("A: That anamoly is a threat to Lucid, Inc. and must be eliminated.")
    .addBullet("B: I agree, but we need to be careful, he's got the `anamoly prototype`")
    .addBullet("A: Yes, fortunately we installed a tracker.")
    .addHighlight("[i]The prototype?[/i]"),

    makeClue("family_photo", "Anamoly - Photo album")
    .condition("hasDiscoveredClue(org_secret_recording1) && hasDiscoveredClue(org_secret_recording2)")
    .addBullet("Half torn wedding photo - face visible but slightly blurred")
    .addBullet("Photo of a newborn")
    .addBullet("A photo of a child, looks about 2 years old")
    .addBullet("Vacation photo - family of 3, looks happy")
    .addPlain("Missing pages in the photo album, some photos are missing")
    .addHighlight("[i]This looks familiar...[/i]"),

    makeClue("bunker_photos", "Anamoly - Bunker photos")
    .condition("hasDiscoveredClue(family_photo)")
    .action("setFlag(bunker_photos)")
    .addPlain("Photos of a secret bunker")
    .addBullet("A secret bunker.. looks like a makeshift lab")
    .addBullet("A warning sign is visible, but the text is blurred")
    .addHighlight("[i]A secret lab?, but this feels familiar...[/i]"),

    makeClue("org_secret_recording2", "Anamoly - Voice recording #2")
    .addPlain("A secret voice exchange between Lucid, Inc. members")
    .addPlain("Time : 4 years ago")
    .addBullet("A: That bastard, has been snooping around on things")
    .addBullet("B: Has he found out about the prototype?")
    .addBullet("A: Seems like it, we need to eliminate him as quickly as possible.")
    .addBullet("B: WAIT! I think he's on the run, we need to get his family, quick!")
    .addHighlight("[i]His family?[/i]"),

    makeClue("org_outsideKey", "Anamoly - Old rusty key")
    .condition("hasDiscoveredClue(bunker_photos) && !hasItem(org_outsideKey)")
    .addPlain("It unlocks a certain door")
    .type("item"),

  });

  /* Detective Office clues */

  cm.addClues({

    /* Later stage key */
    makeClue("bunkerKey", "Anamoly - Bunker key")
    .condition("hasFlag(bunker_memory)")
    .addPlain("A key to the secret bunker")
    ,

    makeClue("photos_001", "Anamoly - Old photos")
    .addPlain("Photo #1")
    .addBullet("An old photo, half torn, the face is missing.")
    .addBullet("wedding clothes, holding hands.")
    .addBullet("Looks about 20 yrs old")
    .addPlain("Photo #2")
    .addBullet("blurry, body is similar to photo #1")
    .addBullet("age about 26, black mask and red eyes")
    .addBullet("[color=red]The light reflects red blooded hands[/color]")
    .addPlain("")
    .addHighlight("A gap of 6 years ...? What could've happened?"),

    makeClue("photos_002", "Anamoly - Photos")
    .addPlain("Photo #1")
    .addBullet("Face is blurred and unrecognizable.")
    .addBullet("In a lab-coat, looks like a researcher.")
    .addBullet("Looks about 23 yrs old")
    .addPlain("Photo #2")
    .addBullet("A photo with some research lab equipment.")
    .addBullet("Lucid, Inc. logo is visible on the equipment.")
    .addPlain("")
    .addHighlight("[i]You mean, the anamoly is related to Lucid, Inc. ?[/i]"),
    
    makeClue("outsideKey", "Old rusty key")
    .condition(
      "hasFlag(first_phase_end) && !hasItem(outsideKey)"
    )
    .action("giveItem(outsideKey)")
    .addPlain("Looks like it could open a certain door.")
    .type("item"),

    
    makeClue("first_phase_end", "Anamoly - Report #1")
    .condition(
      "hasDiscoveredClue(photos_001) && hasDiscoveredClue(photos_002) && "
      "hasDiscoveredClue(clue_001) && hasDiscoveredClue(incident_216) && "
      "hasDiscoveredClue(incident_314) && hasDiscoveredClue(clue_002)"
    )
    .action("setFlag(first_phase_end)")
    .addPlain("A compiled report")
    .addBullet("Lucid, Inc. - A research company")
    .addBullet("Prototype - Highly classified, not much known about it")
    .addBullet("Anamoly - Theft & ties to the fire? incident")
    .addBullet("Anamoly - related to Lucid? - but Lucid denies.")
    ,
    
    makeClue("clue_002", "Anamoly - Robbery Aftermath")
    .condition("!hasFlag(first_phase_end)")
    .addPlain("[b]CLASSIFIED[/b]")
    .addBullet("The facility was abandoned.")
    .addBullet("Nobody is present these days, the place is a mess.")
    .addPlain("Known details")
    .addBullet("They say it was abandoned after a robbery.")
    .addHighlight("[i]But that doesn't make much sense...?[/i]"),

    makeClue("clue_001", "Anamoly - Case #314")
    .condition("!hasFlag(first_phase_end)")
    .addPlain("[b]CLASSIFIED[/b]")
    .addBullet("Face : [b]Unknwon[/b], Age : Estimated [color=#FFA500]28[/color].")
    .addBullet("Brilliant mastermind, leaving minimal traces")
    .addPlain("Known details")
    .addBullet("Stole from Lucid, Inc. - #216")
    .addBullet("Appearance at Incident `#314`")
    .addHighlight("[i]Hmm.. incident #314...?[/i]"),

    makeClue("incident_216", "Anamoly - Incident #216")
    .condition("hasDiscoveredClue(incident_314)")
    .addPlain("[b]CLASSIFIED[/b]")
    .addBullet("Location : [b]Lucid, Inc.[/b]")
    .addBullet("Time : [b]2 years ago[/b]")
    .addPlain("Known details")
    .addBullet("Stole a highly classified nanotech prototype from Lucid, Inc.")
    .addBullet("Cause of theft - Unknown, Risk Level : [color=#FFA500]High[/color]")
    .addHighlight("[i]High risk theft, must have had lots of information[/i]")
    ,

    makeClue("incident_314", "Anamoly - Incident #314")
    .condition("!hasDiscoveredClue(incident_314)")
    .addPlain("[b]CLASSIFIED[/b]")
    .addBullet("Location : [b]Neighborhood outside - Lucid, Inc.[/b]")
    .addBullet("Time : [b]1 month ago[/b]")
    .addPlain("Known details")
    .addBullet("30 people killed, 6 missing")
    .addBullet("Cause of death - Fire (as per reports)")
    .addHighlight("[i]What about the people missing ?? Fire ?[/i]")
    ,
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
#include "interaction_manager.hpp"
#include <entities/player.hpp>
#include <entities/npc_manager.hpp>
#include <map/map_manager.hpp>
#include <debug/logs.hpp>
#include <sound/sound_manager.hpp>
#include <map/map_manager.hpp>
#include <clue/clue_manager.hpp>
#include <ui/message_screen.hpp>
#include <ui/clue_screen.hpp>
#include <ui/ui_manager.hpp>
#include <ui/notification.hpp>
#include <story/story_helpers.hpp>
#include <ui/dialog_screen.hpp>

void InteractionManager::updateNearestInspectable(const sf::Vector2f& playerPos) {
    m_nearestInspectable.reset();
    const auto& inspectables = MapManager::get().getData().inspectables;
    float bestDist = std::numeric_limits<float>::max();
    const float threshold = 100.f;   // interaction radius in world pixels (scaled)

    for (const auto& [pos, clueId] : inspectables) {
        // Convert unscaled position to scaled world coordinates
        sf::Vector2f worldPos(pos.first * Scale::get(), pos.second * Scale::get());
        float dist = std::hypot(playerPos.x - worldPos.x, playerPos.y - worldPos.y);
        if (dist < threshold && dist < bestDist) {
            if (ClueManager::get().isInspectable(clueId)) {
                bestDist = dist;
                m_nearestInspectable = {worldPos, clueId};
            }
        }
    }
}

InteractionManager& InteractionManager::get() {
    static InteractionManager instance;
    return instance;
}

void InteractionManager::update(const sf::Vector2f& playerPos) {
    updateNearestNPC(playerPos);
    updateTransition(playerPos);
    updateNearestInspectable(playerPos);
    buildHints();

    // Push the hints to the Player for rendering
    Player::get().setHints(m_hints);
}

void InteractionManager::updateNearestNPC(const sf::Vector2f& playerPos) {
    Character* playerChar = Player::get().getPlayer();
    NPC* playerAsNPC = dynamic_cast<NPC*>(playerChar);
    m_nearestNPC = NPCManager::get().getNearestInteractable(playerPos, playerAsNPC);
}

void InteractionManager::updateTransition(const sf::Vector2f& playerPos) {
    m_nearTransition = MapManager::get().getTransitionAt(playerPos, 100.f);
}

void InteractionManager::buildHints() {
    m_hints.clear();
    m_canSwapBack = Player::get().canSwapBack();

    if (m_nearestInspectable) {
        ActionHint eHint;
        eHint.key = 'F';
        eHint.label = "Inspect";
        m_hints.push_back(eHint);
    }

    // ---- E action: transition has priority over NPC ----
    if (m_nearTransition) {
        ActionHint eHint;
        eHint.key = 'E';
        eHint.label = m_nearTransition->label;
        // ... priority logic (unchanged)
        if (!(eHint.priority == 0 && Player::get().getPlayer() != NPCManager::get().getNPC("player"))) {
            m_hints.push_back(eHint);
        }
    } else if (m_nearestNPC) {
        ActionHint eHint;
        eHint.key = 'E';
        eHint.label = "Talk";
        eHint.priority = 3;
        m_hints.push_back(eHint);
    }

    // ---- T action: swap back or swap to NPC ----
    if (m_canSwapBack) {
        ActionHint tHint;
        tHint.key = 'T';
        tHint.label = "Swap Back";
        m_hints.push_back(tHint);
    } else if (m_nearestNPC && Player::get().canSwap(m_nearestNPC)) {   // <-- NEW CONDITION
        ActionHint tHint;
        tHint.key = 'T';
        tHint.label = "Swap";
        m_hints.push_back(tHint);
    }
}

void InteractionManager::handleKeyPress(sf::Keyboard::Key key) {
    if (key == sf::Keyboard::Key::E) {
        interact();
    } else if (key == sf::Keyboard::Key::T) {
        swap();
    } else if (key == sf::Keyboard::Key::F) {
        inspect();
    } else if (key == sf::Keyboard::Key::J) {
      UIManager::get().pushScreen(std::make_unique<ClueScreen>());
    }
}

void InteractionManager::inspect() {
    if (!m_nearestInspectable) return;
    const auto& [pos, clueId] = *m_nearestInspectable;

    // Show feedback
    auto clueInfo = ClueManager::get().getClueInfo(clueId);
    ClueManager::get().discoverClue(clueId);
    if ( clueInfo->clue_type == "clue" ) {
      UIManager::get().pushScreen(std::make_unique<ClueScreen>(clueId));
    } else if (clueInfo->clue_type == "item") {
      DialogScreen::show(DialogLine("item").exchange(clueInfo->title, clueInfo->paragraphs.at(0).text));
    }


    // Clear cached state
    m_nearestInspectable.reset();
}

void InteractionManager::interact() {
    if (m_nearTransition) {
      // 1. Evaluate condition
          const auto& tr = *m_nearTransition;
          Log::info << "Checkingg: " << tr.condition << std::endl;
          if (!tr.condition.empty()) {
            Log::info << "Condition not empty: " << tr.condition << std::endl;
              if (!StoryHelpers::evaluateCondition(tr.condition)) {
                  // Show a message
                  if (!tr.failMessage.empty()) {
                      Notification::show("Locked", tr.failMessage, 2.5f);
                  } else {
                      Notification::show("Locked", "You can't use this yet.", 2.0f);
                  }
                  // Play a sound (e.g., "door_locked")
                  SoundManager::get().playSound("door_locked");
                  return;  // don't proceed
              }
          }

          // 2. Condition passed – execute action if any
          if (!tr.action.empty()) {
              StoryHelpers::executeAction(tr.action);
          }
          
        if (m_nearTransition->label == "Door") {
            SoundManager::get().playSound("heavy_door_open");
        } else if (m_nearTransition->label == "Ladder") {
            int idx = rand() % 2;
            SoundManager::get().playSound("ladder" + std::to_string(idx));
        }
        MapManager::get().switchToMap(m_nearTransition->targetMap, m_nearTransition->spawnPosition);
        m_nearTransition.reset();
    } else if (m_nearestNPC) {
        m_nearestNPC->talk();
    }
}



void InteractionManager::swap() {
    if (m_canSwapBack) {
        Player::get().swapBack();
    } else if (m_nearestNPC && Player::get().canSwap(m_nearestNPC)) {   // <-- CHECK BEFORE SWAPPING
        Player::get().swapTo(m_nearestNPC);
    }
}
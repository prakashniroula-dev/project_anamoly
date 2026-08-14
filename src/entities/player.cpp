#include <entities/player.hpp>

void Player::update(sf::RenderWindow &win, float dt)
{
  if (!player)
  {
    Log::error << "Player::update called but player is null (possibly not initialized)" << std::endl;
    return;
  }
  player->update(win, dt);
}

void Player::draw(sf::RenderWindow &win, float dt)
{
  if (!player)
  {
    Log::error << "Player::draw called but player is null (possibly not initialized)" << std::endl;
    return;
  }
  player->draw(win, dt);
}

void Player::init()
{
  if (!player)
  {
    static Character defaultPlayer;
    player = &defaultPlayer;
  }
  player->init();
}

#include <entities/npc_manager.hpp>   // to disable NPC AI

void Player::swapTo(Character* newChar) {
    if (!newChar) {
        Log::error << "Player::swapTo: null character\n";
        return;
    }

    // 1. If we already have a player, push it onto the stack and disable its controls.
    if (player) {
        // If the current player is an NPC, we might want to re‑enable its AI later,
        // but for now we just disable its controls.
        player->lockControls();

        // Also, if it's an NPC, we need to stop its autonomous behavior.
        if (NPC* npc = dynamic_cast<NPC*>(player)) {
            // We'll add a method to NPC to pause its AI.
            npc->pauseAI(true);
        }

        m_characterStack.push_back(player);
    }

    // 2. Set new player.
    player = newChar;
    player->unlockControls();

    // 3. Disable NPC AI for the new character (if it's an NPC).
    if (NPC* npc = dynamic_cast<NPC*>(player)) {
        npc->pauseAI(true);     // prevent automatic patrol/follow
    }

    // 4. (Optional) Notify NPCManager that this NPC is now player-controlled.
    //    NPCManager might need to skip updating this NPC.
    //    We'll handle that inside NPC::update by checking a flag.
}

void Player::swapBack() {
    if (m_characterStack.empty()) {
        Log::warn << "Player::swapBack: no previous character to swap to\n";
        return;
    }

    // 1. Disable current player's controls and re‑enable its AI (if it's an NPC)
    if (player) {
        player->lockControls();
        if (NPC* npc = dynamic_cast<NPC*>(player)) {
            npc->pauseAI(false);   // resume AI
        }
    }

    // 2. Pop the previous character from stack and set as player.
    Character* prev = m_characterStack.back();
    m_characterStack.pop_back();

    player = prev;
    player->unlockControls();

    // 3. Disable AI for the new player (prev was originally an NPC? Might need to pause it again)
    //    Actually, when we first swapped away from an NPC, we paused it; now we are swapping back
    //    to another character (could be original player or an NPC). If the new player is an NPC,
    //    we should pause its AI so it doesn't move autonomously.
    if (NPC* npc = dynamic_cast<NPC*>(player)) {
        npc->pauseAI(true);
    }
}
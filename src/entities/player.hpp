#pragma once
#include <entities/characters.hpp>
#include <debug/logs.hpp>
#include <entities/npc.hpp>
#include <game/interaction_manager.hpp>   // for ActionHint

class Player {
public:
    static Player& get() {
      static Player instance;
      return instance;
    }

    void setPlayer(Character& c) { player = &c; }
    void setPlayerNPC(NPC* npc) { player = static_cast<Character*>(npc); }
    Character* getPlayer() { return player; }
    void setCharacter(std::string c) {
      if (player) {
        player->setCharacter(c);
      } else {
        Log::error << "Player::setCharacter called but player is null" << std::endl;
      }
    }

    void update(sf::RenderWindow& win, float dt);
    void draw(sf::RenderWindow& win, float dt);
    void init();
    void lockControls() {
        if (player) player->lockControls();
    }

    void unlockControls() {
        if (player) player->unlockControls();
    }

    bool controlsLocked() const {
        return player ? player->controlsLocked() : true;
    }

    void handleEvents() {
      if (player) player->handleEvents();
    }

    
    bool canSwap(NPC* npc) const;
    void swapTo(Character* newChar);    // take control of an NPC (or any Character)
    void swapBack();                    // revert to the previous character
    bool canSwapBack() const { return m_characterStack.size() == 1; }

    
    void setHints(const std::vector<ActionHint>& hints) { m_hints = hints; }
    void clearHints() { m_hints.clear(); }
    void drawTooltip(sf::RenderWindow& win) const;   // now uses m_hints

    void clearStack() {
      m_characterStack.clear();
    }
    
    private:
    Player() = default;  // private constructor
    Character* player = nullptr;
     std::vector<Character*> m_characterStack;
     std::string m_tooltipText;
      std::vector<ActionHint> m_hints;
};
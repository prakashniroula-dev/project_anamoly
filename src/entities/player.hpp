#pragma once
#include <entities/characters.hpp>
#include <debug/logs.hpp>

class Player {
public:
    static Player& get() {
      static Player instance;
      return instance;
    }

    void setPlayer(Character& c) { player = &c; }
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
    
    private:
    Player() = default;  // private constructor
    Character* player = nullptr;
};
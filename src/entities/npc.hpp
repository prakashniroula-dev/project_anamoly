#pragma once

#include <entities/characters.hpp>
#include <string>

class NPC : public Character {
  sf::Vector2f patrolTarget;
  bool patrolling = false;
  int lastY = 0;
public:
  NPC(std::string c = Characters::Fighter_Boss) : Character(c, false) {}

  sf::Vector2f SPAWN_POS() {
    return sf::Vector2f(1.f * 32.f, 8.f * 32.f); // Example spawn position for NPC
  }

  void update(sf::RenderWindow& win, float dt) override;

  void patrolTo(sf::Vector2f targetPos);

};
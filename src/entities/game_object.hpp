// src/entities/GameObject.hpp
#pragma once
#include <SFML/Graphics/RenderWindow.hpp> // Tiny header instead of massive Graphics.hpp!

class GameObject {
  public:
    virtual ~GameObject() = default; // Always add a virtual destructor for interfaces!
    
    virtual void draw(sf::RenderWindow& win, float dt) = 0;
    virtual void update(float dt) {}
    virtual void update(sf::RenderWindow& win, float dt) {}
};
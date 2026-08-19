#pragma once
#include <SFML/Graphics.hpp>
#include <string>

namespace Notification {
    // Show a notification. Duration in seconds. 
    void show(const std::string& title, const std::string& description, float duration = 3.0f);

    // Called every frame from the main loop to update timer & animation.
    void update(float dt, const sf::RenderWindow& window);

    // Called every frame from the main loop to draw on top of everything.
    void draw(sf::RenderWindow& window);
}
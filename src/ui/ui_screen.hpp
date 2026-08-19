// ui_screen.hpp
#pragma once
#include <SFML/Graphics.hpp>

class UIScreen {
public:
    virtual ~UIScreen() = default;

    // Called when screen becomes active (pushed)
    virtual void onEnter() {}

    // Called when screen is popped
    virtual void onExit() {}

    // Handle SFML events; return true if event was consumed
    virtual bool handleEvent(const sf::Event& event, sf::RenderWindow& window) = 0;

    // Update logic (called every frame if screen is on top)
    virtual void update(float dt) {}

    // Draw to the window
    virtual void draw(sf::RenderWindow& window) = 0;

    // Whether this screen blocks input to screens below
    virtual bool blocksInput() const { return true; }
    virtual bool displayBelow() const { return true; } // Whether to display screens below this one

    // Whether the game should update while this screen is on top
    virtual bool blocksGameUpdate() const { return true; }
    // virtual bool isNotification() const { return false; }
};
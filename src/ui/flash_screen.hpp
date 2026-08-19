#pragma once
#include "ui_screen.hpp"
#include <SFML/Graphics.hpp>
#include <functional>

class FlashScreen : public UIScreen {
public:
    // duration: total time the flash lasts (seconds)
    // color: white (default) or black
    FlashScreen(float duration = 1.5f, sf::Color color = sf::Color::White);

    void onEnter() override;
    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;
    bool handleEvent(const sf::Event&, sf::RenderWindow&) override { return true; } // consume all input
    bool blocksGameUpdate() const override { return true; }
    bool blocksInput() const override { return true; }
    bool displayBelow() const override { return false; }

private:
    sf::RectangleShape m_overlay;
    sf::Color m_baseColor;
    float m_duration;
    float m_elapsed = 0.f;
    float m_flickerTimer = 0.f;
    float m_flickerInterval = 0.05f; // change alpha every 50ms
    bool m_fadingOut = false;
};
#pragma once
#include <SFML/Graphics/RenderWindow.hpp>
#include <string>
#include <entities/game_object.hpp>

namespace OverlayKeys {
    extern const std::string PowerStationOverlay;
}

class Overlay : public GameObject {
    std::string textureKey;
    sf::Color tintColor;

public:
    Overlay(const std::string& texKey = OverlayKeys::PowerStationOverlay, const sf::Color& color = sf::Color(255, 255, 255, 50));

    // Static load function for resources
    static void load(sf::RenderWindow& win);

    // GameObject interface
    void draw(sf::RenderWindow& win, float dt) override;
    void update(float dt) override;

    // Optional configuration methods
    void setTextureKey(const std::string& key);
    void setColor(const sf::Color& color);
};
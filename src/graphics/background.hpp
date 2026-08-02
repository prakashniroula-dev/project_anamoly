// src/graphics/Background.hpp
#pragma once
#include <SFML/Graphics/RenderWindow.hpp>
#include <string>
#include <entities/game_object.hpp>

// Just declare the namespace with the keys
namespace BG {
    // These are now just declarations (no static inline)
    extern const std::string PowerStationDay;
    extern const std::string PowerStationNight;
}

class Background : public GameObject {
    std::string currentBg;
    sf::Vector2f viewCenter;
    sf::Vector2f viewSize;
    sf::Vector2f pos;
    
public:
    Background();
    
    // Static load function (declaration only)
    static void load(sf::RenderWindow& win);
    
    // GameObject interface
    void draw(sf::RenderWindow& win, float dt) override;
    void update(float dt) override;
    
    // Background-specific methods
    void setBG(const std::string& bg_key);
    std::string getBG() const;
};
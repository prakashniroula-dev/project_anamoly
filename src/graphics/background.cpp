// src/graphics/Background.cpp
#include "Background.hpp"
#include "Textures.hpp"
#include <debug/logs.hpp>
#include <core/scale.hpp>
#include <core/position.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <string>

// The actual string constants live here (single copy!)
namespace BG {
    const std::string PowerStationDay = "power_station_day";
    const std::string PowerStationNight = "power_station_night";
}

Background::Background() 
    : currentBg(BG::PowerStationDay) {
}

void Background::load(sf::RenderWindow& win) {
    static Log::Scope scope("Background::Load()");
    
    // Load day textures
    scope.info << "Load PowerStationDay textures...\n";
    for (int i = 0; i < 5; i++) {
        std::string sidx = std::to_string(i + 1);
        Textures::load(
            BG::PowerStationDay + sidx,
            "power_station/bg/Day/" + sidx + ".png"
        );
    }

    sf::Texture& tex = Textures::get(BG::PowerStationDay + "1");
    tex.setRepeated(true);
    
    // Load night textures
    scope.info << "Load PowerStationNight textures...\n";
    for (int i = 0; i < 5; i++) {
        std::string sidx = std::to_string(i + 1);
        Textures::load(
            BG::PowerStationNight + sidx,
            "power_station/bg/Night/" + sidx + ".png"
        );
    }

}

void Background::draw(sf::RenderWindow& win, float dt) {
    auto winSize = win.getSize();
    const sf::View& view = win.getView();
    sf::Vector2f viewCenter = view.getCenter();
    sf::Vector2f viewSize = view.getSize();
    for (int i = 0; i < 5; i++) {
        std::string sidx = std::to_string(i + 1);
        const sf::Texture& tex = Textures::get(currentBg + sidx);

        // Scale this texture to fit the window height
        float local_scale = static_cast<float>(winSize.y) / static_cast<float>(tex.getSize().y);
        sf::Vector2f scaleVec(local_scale, local_scale);
        
        // First tile (left)
        sf::Sprite s(tex);
        s.setTextureRect(sf::IntRect(sf::Vector2i(pos.x * 0.5f, 0), sf::Vector2i(viewSize.x, viewSize.y)));
        s.setScale(scaleVec);
        float y = winSize.y - tex.getSize().y * local_scale; // bottom‑align
        
        float parallaxFactor = 0.5f;   // slower than the world (e.g., 0.5 for depth)
        float bgX = viewCenter.x * parallaxFactor;
        // ---- Vertical fixed position (e.g., always centred on screen) ----
        float bgY = 0.f;   // or some fixed world Y, e.g., viewCenter.y if you want it centred vertically, but you want fixed, so set to a constant
        pos = sf::Vector2f(bgX, bgY);
        s.setPosition(sf::Vector2f(viewCenter.x - viewSize.x / 2.f, y));
        win.draw(s);   
    }
}

void Background::update(float dt) {
    

        // // Set sprite position (in world coordinates)
        // pos = sf::Vector2f(bgX, bgY);

}

void Background::setBG(const std::string& bg_key) {
    currentBg = bg_key;
}

std::string Background::getBG() const {
    return currentBg;
}
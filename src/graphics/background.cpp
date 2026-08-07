// src/graphics/Background.cpp
#include <graphics/background.hpp>
#include <graphics/textures.hpp>
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
    float viewLeftEdge = viewCenter.x - viewSize.x / 2.f;
    for (int i = 0; i < 5; i++) {
        std::string sidx = std::to_string(i + 1);
        std::string texKey = currentBg + sidx;
        sf::Texture& tex = Textures::get(texKey);
        tex.setRepeated(true);
        sf::Sprite s(tex);
        float local_scale = static_cast<float>(winSize.y) / static_cast<float>(tex.getSize().y);
        float viewYBottom = viewCenter.y + viewSize.y / 2.f;
        float y = viewYBottom - tex.getSize().y * local_scale; // bottom‑align
        s.setScale(sf::Vector2f(local_scale, local_scale));

        float parallaxFactor = 0.14f * (i + 1);
        float pos = viewLeftEdge * (1.0f - parallaxFactor);
        float textureWidth = tex.getSize().x;
        sf::IntRect texRect(sf::Vector2i(0, 0), sf::Vector2i(tex.getSize().x * 500, tex.getSize().y));
        s.setTextureRect(texRect);
        s.setPosition(sf::Vector2f(pos, y));
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
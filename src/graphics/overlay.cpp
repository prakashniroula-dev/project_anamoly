#include <graphics/overlay.hpp>
#include <graphics/textures.hpp>
#include <debug/logs.hpp>
#include <SFML/Graphics/Sprite.hpp>

namespace OverlayKeys {
    const std::string PowerStationOverlay = "power_station_overlay";
}

Overlay::Overlay(const std::string& texKey, const sf::Color& color)
    : textureKey(texKey), tintColor(color) {}

void Overlay::load(sf::RenderWindow& win) {
    static Log::Scope scope("Overlay::Load()");
    scope.info << "Load PowerStationOverlay texture...\n";
    Textures::load(OverlayKeys::PowerStationOverlay, "power_station/bg/Overlay.png");
}

void Overlay::draw(sf::RenderWindow& win, float dt) {
    const sf::View& view = win.getView();
    sf::Vector2f viewCenter = view.getCenter();
    sf::Vector2f viewSize = view.getSize();
    float viewLeftEdge = viewCenter.x - viewSize.x / 2.f;
    float viewTopEdge = viewCenter.y - viewSize.y / 2.f;

    sf::Texture& tex = Textures::get(textureKey);
    sf::Sprite s(tex);

    // CSS 'cover' style behavior: scale independently to fill the entire view dimensions
    float scaleX = viewSize.x / static_cast<float>(tex.getSize().x);
    float scaleY = viewSize.y / static_cast<float>(tex.getSize().y);

    s.setScale(sf::Vector2f(scaleX, scaleY));
    s.setColor(tintColor);
    s.setTextureRect(sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(tex.getSize().x, tex.getSize().y)));
    
    // Lock position to the top-left of the current camera view
    s.setPosition(sf::Vector2f(viewLeftEdge, viewTopEdge));

    win.draw(s);
}

void Overlay::update(float dt) {
    // Logic updates for the overlay if needed (e.g., pulsing alpha)
}

void Overlay::setTextureKey(const std::string& key) {
    textureKey = key;
}

void Overlay::setColor(const sf::Color& color) {
    tintColor = color;
}
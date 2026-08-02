// core/position.hpp
#pragma once
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <core/scale.hpp>  // Still needed for Scale::get()

namespace Position {
    // Remove 'static inline' global variable.
    // If you really need a zero constant, use this:
    inline const sf::Vector2f Zero = sf::Vector2f(0, 0);

    // Keep the functions. 'inline' (without 'static') is cleaner.
    inline sf::Vector2f center(sf::RenderWindow& win, sf::Vector2f size) {
        return sf::Vector2f(
            win.getSize().x / 2.f - size.x / 2.f,
            win.getSize().y / 2.f - size.y / 2.f
        );
    }

    inline sf::Vector2f bottomLeft(sf::RenderWindow& win, sf::Vector2f size, sf::Vector2f initialPos = Zero) {
        return sf::Vector2f(
            initialPos.x,
            win.getSize().y - size.y * Scale::get()
        );
    }

    inline sf::Vector2f bottomLeft(sf::Vector2u bottom, sf::Vector2u size, sf::Vector2f initialPos = Zero) {
        return sf::Vector2f(
            initialPos.x,
            bottom.y - size.y * Scale::get()
        );
    }

    inline sf::Vector2f bottomLeft(sf::Vector2f bottom, sf::Vector2f size, sf::Vector2f initialPos = Zero) {
        return sf::Vector2f(
            initialPos.x,
            bottom.y - size.y * Scale::get()
        );
    }

    inline sf::Vector2f topRight(sf::RenderWindow& win) {
        return sf::Vector2f(win.getSize().x, 0.f);
    }

    inline sf::Vector2f X(float x) {
        return sf::Vector2f(x, 0.f);
    }

    inline sf::Vector2f Y(float y) {
        return sf::Vector2f(0.f, y);
    }
}
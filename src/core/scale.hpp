// src/core/scale.hpp
#pragma once
#include <SFML/System/Vector2.hpp> // Tiny header, just for Vector2f

namespace Scale {
    // Just declarations. No 'static inline' here!
    float get();
    sf::Vector2f getVec();
    void set(float s);
}
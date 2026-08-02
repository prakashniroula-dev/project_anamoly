// src/core/scale.cpp
#include "scale.hpp"

namespace {
    // This is the ONE and ONLY scale variable in your entire program.
    // Because it's inside this anonymous namespace, it's private to this .cpp file.
    float current_scale = 1.0f;
}

namespace Scale {
    float get() {
        return current_scale;
    }

    sf::Vector2f getVec() {
        return sf::Vector2f(current_scale, current_scale);
    }

    void set(float s) {
        current_scale = s;
    }
}
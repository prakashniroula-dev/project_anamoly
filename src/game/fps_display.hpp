#pragma once
#include <SFML/Graphics.hpp>

class FpsDisplay {
public:
    void update(float dt, sf::RenderWindow& window);

private:
    float m_timer = 0.f;
    int   m_frameCount = 0;
};
#include <game/fps_display.hpp>
#include <string>

void FpsDisplay::update(float dt, sf::RenderWindow& window) {
    m_frameCount++;
    m_timer += dt;
    if (m_timer >= 1.0f) {
        std::string title = "MyApp - FPS: " + std::to_string(m_frameCount);
        window.setTitle(title);
        m_frameCount = 0;
        m_timer = 0.f;
    }
}
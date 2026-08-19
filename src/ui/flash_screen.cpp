#include "flash_screen.hpp"
#include "ui_manager.hpp"
#include <random>
#include <cmath>
#include <algorithm>

FlashScreen::FlashScreen(float duration, sf::Color color)
    : m_duration(duration), m_baseColor(color) {
    m_overlay.setFillColor(sf::Color(color.r, color.g, color.b, 0));
}

void FlashScreen::onEnter() {
    m_elapsed = 0.f;
    m_flickerTimer = 0.f;
    m_fadingOut = false;
    // Start with high opacity
    m_overlay.setFillColor(sf::Color(m_baseColor.r, m_baseColor.g, m_baseColor.b, 200));
}

void FlashScreen::update(float dt) {
    m_elapsed += dt;
    m_flickerTimer += dt;

    // Flicker: random alpha changes while not fading out
    if (!m_fadingOut && m_flickerTimer >= m_flickerInterval) {
        m_flickerTimer = 0.f;
        // Random alpha between 180 and 255 (high opacity)
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<int> dist(180, 255);
        int alpha = dist(gen);
        m_overlay.setFillColor(sf::Color(m_baseColor.r, m_baseColor.g, m_baseColor.b, alpha));
    }

    // After half the duration, start fading out
    if (m_elapsed >= m_duration * 0.5f && !m_fadingOut) {
        m_fadingOut = true;
    }

    if (m_fadingOut) {
        float progress = (m_elapsed - m_duration * 0.5f) / (m_duration * 0.5f);
        int alpha = static_cast<int>(255 * (1.f - progress));
        alpha = std::clamp(alpha, 0, 255);
        m_overlay.setFillColor(sf::Color(m_baseColor.r, m_baseColor.g, m_baseColor.b, alpha));
    }

    // End when fully transparent
    if (m_elapsed >= m_duration) {
        UIManager::get().popScreen();
    }
}

void FlashScreen::draw(sf::RenderWindow& window) {
    // Set overlay to full screen size
    sf::Vector2u size = window.getSize();
    m_overlay.setSize(sf::Vector2f(size));
    m_overlay.setPosition({0.f, 0.f});
    window.draw(m_overlay);
}
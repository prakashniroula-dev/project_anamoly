#pragma once
#include <ui/ui_screen.hpp>
#include <debug/logs.hpp>

class UIManager {
public:
    static UIManager& get() {
        static UIManager instance;
        return instance;
    }

    void pushScreen(std::unique_ptr<UIScreen> screen);
    void popScreen();
    void clearScreens();
    bool handleEvent(const sf::Event& event, sf::RenderWindow& window);
    void update(float dt, bool& gameShouldUpdate);
    void draw(sf::RenderWindow& window) const;
    bool isEmpty() const { return m_screens.empty(); }
    void init() {
        // Load a shared font for UI screens
        if (!m_font.openFromFile("assets/fonts/orbitron.ttf")) {
            Log::error << "Failed to load font for UIManager" << std::endl;
        }
    }
    sf::Font& getFont() { return m_font; } // Provide access to a shared font
    static sf::View getUIView(const sf::RenderWindow& window) {
        static sf::View uiView = window.getDefaultView();
        uiView.setSize(sf::Vector2f(window.getSize()));
        uiView.setCenter(sf::Vector2f(window.getSize()) / 2.f);
        uiView.setViewport(sf::FloatRect(sf::Vector2f(0.f, 0.f), sf::Vector2f(1.f, 1.f))); // to prevent blur
        return uiView;
    }

private:
    UIManager() = default;                          // private constructor
    std::vector<std::unique_ptr<UIScreen>> m_screens;
    sf::Font m_font; // Shared font instance
};
#pragma once
#include "ui_screen.hpp"
#include <SFML/Graphics.hpp>
#include <functional>

namespace Game { class Game; }

class DeathScreen : public UIScreen {
public:
    DeathScreen(Game::Game* game);
    void onEnter() override;
    bool handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void draw(sf::RenderWindow& window) override;
    bool blocksGameUpdate() const override { return true; }
    bool blocksInput() const override { return true; }

private:
    enum Option { Retry, NewGame };
    Game::Game* m_game;
    sf::Font m_font;
    sf::Text m_title;
    sf::Text m_subtitle;
    std::vector<sf::Text> m_options;
    std::vector<sf::FloatRect> m_optionRects;
    sf::RectangleShape m_backdrop;
    int m_selectedIndex = 0;

    void updateLayout(sf::RenderWindow& window);
    void executeSelected();
};
#pragma once
#include <ui/ui_screen.hpp>
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <game/game.hpp>

class PauseMenu : public UIScreen {
public:

void onEnter() override;
void onExit() override;
bool handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
void update(float dt) override;
void draw(sf::RenderWindow& window) override;

PauseMenu(Game::Game* gameInstance);
bool blocksGameUpdate() const override { return true; }   // pause game

private:
    enum Option {
        Resume,
        Save,
        GotoMainMenu,
        Options,
        Quit
    };

    std::vector<std::string> labels;
    int selectedIndex = 0;

    sf::Font font;
    sf::Text titleLine1;   // "Project"
    sf::Text titleLine2;   // "A.N.A.M.O.L.Y"
    std::vector<sf::Text> optionTexts;
    std::vector<sf::FloatRect> optionRects;   // for mouse hit‑testing
    sf::RectangleShape background;

    void updateLayout(sf::RenderWindow& window);
    void executeSelected();
    Game::Game* game = nullptr; // Pointer to the game instance for starting a new game
};
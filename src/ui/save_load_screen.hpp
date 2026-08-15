#pragma once
#include "ui_screen.hpp"
#include <vector>
#include <string>
#include <functional>

class SaveLoadScreen : public UIScreen {
public:
    enum class Mode { Save, Load };

    SaveLoadScreen(Mode mode, std::function<void(const std::string&)> onSelect);
    void onEnter() override;
    bool handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void draw(sf::RenderWindow& window) override;
    bool blocksGameUpdate() const override { return true; }
    bool blocksInput() const override { return true; }

private:
    Mode mode;
    std::function<void(const std::string&)> onSelect;
    std::vector<std::string> slotFiles;
    int selectedIndex = 0;

    sf::Font font;
    sf::Text title;
    std::vector<sf::Text> slotLabels;
    sf::RectangleShape background;

    void updateLayout(sf::RenderWindow& window);
    void scanSlots();
    void executeSelection();
};
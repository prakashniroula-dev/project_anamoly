#pragma once
#include "ui_screen.hpp"
#include <entities/npc.hpp>
#include <SFML/Graphics/Text.hpp>
#include <vector>

class DialogScreen : public UIScreen {
public:
    DialogScreen(NPC* npc, bool allowEscape = true);
    DialogScreen(NPC* npc, const std::string& dialogueId, bool allowEscape = true);
    void onEnter() override;
    void onExit() override;
    bool handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;
    bool blocksGameUpdate() const override { return true; }
    static void show(const DialogLine& line);

private:
    NPC* npc;
    sf::View uiView;   // view used for UI rendering and event mapping
    const DialogLine* currentLine;      // pointer to current node
    std::vector<const DialogLine*> visibleOptions; // filtered by condition
    sf::Text speakerText;
    sf::Text dialogueText;
    std::vector<sf::Text> optionTexts;
    sf::Font font;
    sf::RectangleShape background;
    void moveToLine(int index);   // new
    int highlightedOption = 0; 
    std::vector<sf::FloatRect> optionRects;   // store button rectangles for hit testing
    sf::FloatRect m_boxRect;
    sf::Vector2f  m_speakerPos;
    sf::Vector2f  m_dialoguePos;

    bool m_allowEscape;

    void updateLayout(sf::RenderWindow& window); // recompute optionRects based on current layout

    // Internal
    void advanceToNextLine();              // when no choices, moves to nextIndex or ends
    void selectOption(size_t index);
    void refreshDisplay();                 // updates texts and visible options
    bool isLineValid(const DialogLine* line) const; // checks condition

};
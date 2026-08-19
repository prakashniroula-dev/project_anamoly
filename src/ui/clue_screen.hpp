#pragma once
#include "ui_screen.hpp"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <clue/clue_manager.hpp>

class ClueScreen : public UIScreen {
public:
    ClueScreen(const std::string& initialClueId = "");
    void onEnter() override;
    void onExit() override;
    bool handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;
    bool blocksGameUpdate() const override { return true; }
    bool blocksInput() const override { return true; }

private:
    enum class State { List, Detail };
    State m_state = State::List;
    int m_selectedIndex = 0;
    int m_scrollOffset = 0;
    int m_maxVisibleItems = 0;

    ClueInfo m_currentDetail;

    // Each paragraph is a list of formatted text segments
    struct FormattedParagraph {
        ParagraphType type;
        std::vector<sf::Text> segments;   // each segment has its own style/color
    };
    std::vector<FormattedParagraph> m_paragraphs;

    sf::Font m_font;
    sf::RectangleShape m_backdrop;
    sf::RectangleShape m_parchment;
    sf::Texture m_parchmentTexture;
    sf::Text m_titleText;                 // main title: "Journal" or clue title
    std::vector<sf::Text> m_clueTexts;    // list entries ("1. Title")
    sf::Text m_backHint;

    sf::RectangleShape m_highlightRect;

    // Scroll buttons
    sf::RectangleShape m_upButton;
    sf::RectangleShape m_downButton;
    sf::Text m_upArrow;
    sf::Text m_downArrow;
    bool m_showScrollButtons = false;

    std::string m_initialClueId;

    void updateLayout(sf::RenderWindow& window);
    void refreshList();
    void showDetail(const std::string& clueId);
    void goBack();
    void loadParchmentTexture();
    void buildParagraphs(const ClueInfo& info);   // parses markup
    void clampScrollOffset();
    std::vector<sf::FloatRect> m_itemRects;   // hit‑test rects for visible list items
};
#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

class Palette {
public:
    Palette(const sf::Font& font);
    void setSprites(const std::vector<sf::Sprite>& sprites);
    void setLayout(int columns, int rowsPerPage, float cellSize, float spacing);
    void updateLayout(const sf::Vector2u& windowSize);
    void draw(sf::RenderWindow& window) const;
    bool handleMousePress(const sf::Vector2f& mousePos);
    bool handleMouseScroll(float delta);
    void setSelected(int idx);
    int getSelected() const;
    void setVisible(bool v);
    bool isVisible() const;
    void nextPage();
    void prevPage();

    sf::FloatRect getPrevBtnBounds() const { return prevBtn.getGlobalBounds(); }
    sf::FloatRect getNextBtnBounds() const { return nextBtn.getGlobalBounds(); }

private:
    const sf::Font& font;
    std::vector<sf::Sprite> sprites;
    int columns = 8;
    int rowsPerPage = 3;
    float cellSize = 24.f;
    float spacing = 3.f;
    int page = 0;
    int selected = 0;
    bool visible = true;

    sf::RectangleShape background;
    sf::RectangleShape highlight;
    sf::RectangleShape prevBtn, nextBtn;
    sf::Text prevText, nextText;

    void updateButtonPositions();
    int getStartIdx() const;
    int getEndIdx() const;
};
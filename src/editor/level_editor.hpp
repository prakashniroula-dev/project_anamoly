// src/editor/LevelEditor.hpp
#pragma once
#include <SFML/Graphics.hpp>
#include <entities/terrain.hpp>

class LevelEditor {
public:
    LevelEditor();
    void handleEvent(const sf::Event& event, sf::RenderWindow& window);
    void draw(sf::RenderWindow& window);
    void setActive(bool active);
    bool isActive() const;
    void initPalette();
    

private:
    void updatePaletteLayout(const sf::Vector2u& windowSize);
    private:
    // Base (unscaled) palette parameters
    float basePaletteTileSize = 32.f;
    float baseSpacing = 4.f;
    int paletteColumns = 10;

    bool active = false;
    int selectedTile = 1;   // default tile ID
    sf::RectangleShape cursor;
    sf::Text tileInfo;
    sf::Font font;
    sf::Vector2i hoveredTile;  // grid coords
    bool isPainting = false;
    sf::Vector2i lastPaintedTile;
    bool showGrid;                     // whether to draw grid lines
    bool isErasing; 
    sf::Vector2i lastErasedTile;

    // Palette
    std::vector<sf::Sprite> paletteSprites;   // tile previews using real textures
    sf::RectangleShape paletteBackground;     // semi-transparent background
    sf::RectangleShape selectionHighlight;    // outline around selected tile
    int paletteTileSize = 32;                 // preview size in pixels
    sf::Vector2f paletteOffset; 

    void paintTile(int tx, int ty);
    void updateHighlightPosition();
};
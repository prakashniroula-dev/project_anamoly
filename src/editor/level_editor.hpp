#pragma once
#include <SFML/Graphics.hpp>
#include "tile_editor.hpp"
#include "object_editor.hpp"
#include "solid_editor.hpp"

class LevelEditor {
public:
    LevelEditor();
    void handleEvent(const sf::Event& event, sf::RenderWindow& window);
    void draw(sf::RenderWindow& window);
    void setActive(bool active);
    bool isActive() const;

    void init(); // calls tileEditor.init() and objectEditor.init()

private:
    enum class Mode { Tile, Object, Solid };
    Mode currentMode = Mode::Tile;
    bool active = false;

    sf::Font font;
    TileEditor tileEditor;
    ObjectEditor objectEditor;
    SolidEditor solidEditor;

    void switchMode(Mode mode);
};
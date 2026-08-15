#pragma once
#include <SFML/Graphics.hpp>
#include "tile_editor.hpp"
#include "object_editor.hpp"
#include "solid_editor.hpp"
#include "spawn_editor.hpp"
#include "waypoint_editor.hpp"
#include <sstream>

class LevelEditor {
public:
    LevelEditor();
    void handleEvent(const sf::Event& event, sf::RenderWindow& window);
    void draw(sf::RenderWindow& window);
    void setActive(bool active);
    bool isActive() const;

    void init();

private:
    enum class Mode { Tile, Object, Solid, Spawn, Waypoint };
    Mode currentMode = Mode::Tile;
    bool active = false;

    sf::Font font;
    TileEditor tileEditor;
    ObjectEditor objectEditor;
    SolidEditor solidEditor;
    SpawnEditor spawnEditor;
    WaypointEditor waypointEditor;

    // ---- UI elements for world coordinates ----
    sf::Text m_positionText;
    sf::RectangleShape m_positionBg;

    // ---- Store last mouse world position for clipboard copy ----
    sf::Vector2f m_lastMouseWorld;

    void switchMode(Mode mode);
};
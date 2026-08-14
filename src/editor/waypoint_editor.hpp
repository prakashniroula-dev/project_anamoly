#pragma once
#include <SFML/Graphics.hpp>
#include <entities/terrain.hpp>
#include <entities/npc_manager.hpp>
#include <editor/undo_stack.hpp>
#include <editor/spawn_editor.hpp>

class WaypointEditor {
public:
    WaypointEditor(const sf::Font& font);
    void init();
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void draw(sf::RenderWindow& window);
    void setActive(bool active);
    bool isActive() const;
    void undo() { undoStack.undo(); }
    void redo() { undoStack.redo(); }

private:
    bool active = false;
    const sf::Font& font;
    sf::Vector2f mouseWorldPos;

    // Selected spawn state
    bool m_hasSelection = false;
    std::pair<float, float> m_selectedKey;
    SpawnProps* m_selectedProps = nullptr;

    // UI elements
    sf::RectangleShape selectionHighlight;
    sf::CircleShape waypointDot;
    sf::RectangleShape infoBackground;
    sf::Text infoText;

    using WaypointUndoStack = UndoStack<SpawnChange, applySpawnChange>;
    WaypointUndoStack undoStack;

    void startRecording() { undoStack.beginGroup(); }
    void stopRecording() { undoStack.commitGroup(); }

    void setSpawnWithUndo(const std::pair<float,float>& key, const SpawnProps& newProps);

    void selectSpawnAt(const sf::Vector2f& worldPos);
    void addWaypoint(const sf::Vector2f& worldPos);
    void removeWaypointAt(const sf::Vector2f& worldPos);
    void updateInfoText();
    void drawWaypoints(sf::RenderWindow& window, const SpawnProps& props, const sf::Vector2f& worldPos);
};
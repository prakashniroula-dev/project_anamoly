#pragma once
#include <SFML/Graphics.hpp>
#include "undo_stack.hpp"
#include "palette.hpp"
#include <entities/objects.hpp>

struct ObjectChange {
    float x, y;
    ObjectProps oldProps;
    ObjectProps newProps;
    bool isPaint;
};

void applyObjectChange(ObjectChange& change, bool forward);

class ObjectEditor {
public:
    explicit ObjectEditor(const sf::Font& font);
    void init();

    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void draw(sf::RenderWindow& window);
    void setActive(bool active);
    bool isActive() const;
    void setPaletteVisible(bool visible);

    void undo();
    void redo();
    void togglePaletteVisibility();

    // Exposed so LevelEditor can force an update before event handling
    void updatePaletteLayout(const sf::Vector2u& windowSize);

private:
    bool active = false;
    bool showPalette = true;

    Palette palette;
    int selectedObject = 0;
    float currentObjectScale = 1.0f;

    sf::Sprite objectCursor;
    sf::Texture dummyTexture;
    sf::Vector2f mouseWorldPos;

    using ObjectUndoStack = UndoStack<ObjectChange, applyObjectChange>;
    ObjectUndoStack undoStack;

    void updateCursor();
    void paintObject(float x, float y);
    void eraseObject(float x, float y);
    void setObjectWithUndo(float x, float y, const ObjectProps& newProps, bool isPaint);
    void startRecording() { undoStack.beginGroup(); }
    void stopRecording() { undoStack.commitGroup(); }

    friend void applyObjectChange(ObjectChange& change, bool forward);
};
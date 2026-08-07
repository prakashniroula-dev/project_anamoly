#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "undo_stack.hpp"

struct SolidChange {
    int tx, ty;
    int oldType;
    int newType;
};

void applySolidChange(SolidChange& change, bool forward);

class SolidEditor {
public:
    explicit SolidEditor(const sf::Font& font);
    void init();

    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void draw(sf::RenderWindow& window);
    void setActive(bool active);
    bool isActive() const;

    void undo();
    void redo();

private:
    bool active = false;
    sf::Vector2i hoveredTile;
    sf::RectangleShape solidCursor;

    // Drag state
    bool isPainting = false;
    sf::Vector2i lastPaintedTile;
    bool isErasing = false;
    sf::Vector2i lastErasedTile;
    std::vector<bool> processedInDrag;  // avoid duplicate undo entries

    using SolidUndoStack = UndoStack<SolidChange, applySolidChange>;
    SolidUndoStack undoStack;

    void setSolidWithUndo(int tx, int ty, int newType);
    void startRecording() { undoStack.beginGroup(); }
    void stopRecording() { undoStack.commitGroup(); }

    friend void applySolidChange(SolidChange& change, bool forward);
};
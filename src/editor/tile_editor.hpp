#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "undo_stack.hpp"
#include "palette.hpp"

struct TileChange {
    int tx, ty;
    std::vector<int> oldTiles;
    std::vector<int> newTiles;
    bool isPaint;
};

void applyTileChange(TileChange& change, bool forward);

class TileEditor {
public:
    explicit TileEditor(const sf::Font& font);
    void init();

    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void draw(sf::RenderWindow& window);
    void setActive(bool active);
    bool isActive() const;
    void setPaletteVisible(bool visible);

    void undo();
    void redo();
    void toggleStackMode();
    bool isStackMode() const;
    void togglePaletteVisibility();

    // Exposed so LevelEditor can force an update before event handling
    void updatePaletteLayout(const sf::Vector2u& windowSize);

private:
    bool active = false;
    bool stackMode = false;
    bool showPalette = true;

    Palette palette;
    int selectedTile = 1;

    sf::RectangleShape tileCursor;
    sf::Vector2i hoveredTile;
    sf::Vector2f mouseWorldPos;

    bool isPainting = false;
    sf::Vector2i lastPaintedTile;
    bool isErasing = false;
    sf::Vector2i lastErasedTile;
    std::vector<bool> processedInDrag;
    std::vector<std::vector<int>> previousTiles;

    using TileUndoStack = UndoStack<TileChange, applyTileChange>;
    TileUndoStack undoStack;

    sf::RectangleShape replaceBtn, stackBtn;
    sf::Text replaceText, stackText;

    void paintTile(int tx, int ty);
    void handleRightClickTile(int tx, int ty, bool shiftHeld);
    void setTileWithUndo(int tx, int ty, int newTile, bool isPaint);
    void startRecording() { undoStack.beginGroup(); }
    void stopRecording() { undoStack.commitGroup(); }

    friend void applyTileChange(TileChange& change, bool forward);
};
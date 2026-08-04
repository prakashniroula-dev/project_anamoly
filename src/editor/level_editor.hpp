// src/editor/LevelEditor.hpp
#pragma once
#include <SFML/Graphics.hpp>
#include <entities/terrain.hpp>
#include <vector>
#include <algorithm>

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
    void updateHighlightPosition();
    void paintTile(int tx, int ty);
    void handleRightClickAction(int tx, int ty, bool shiftHeld);

    // Undo/redo helpers
    void startRecording();
    void stopRecordingAndPush();
    void setTileDirect(int tx, int ty, int newTile);
    void setTileWithUndo(int tx, int ty, int newTile, bool isPaint);
    void undo();
    void redo();

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
    bool showPallete;                     // whether to show pallete
    bool isErasing;
    sf::Vector2i lastErasedTile;

    // Palette
    std::vector<sf::Sprite> paletteSprites;   // tile previews using real textures
    sf::RectangleShape paletteBackground;     // semi-transparent background
    sf::RectangleShape selectionHighlight;    // outline around selected tile
    int paletteTileSize = 32;                 // preview size in pixels
    sf::Vector2f paletteOffset;

    // Right‑click one‑level undo (per tile)
    std::vector<int> previousTiles;
    std::vector<bool> processedInDrag;        // marks tiles already handled in current right‑click drag

    // Multi‑level undo/redo
    struct TileChange {
        int tx, ty;
        int oldTile;
        int newTile;
        bool isPaint;   // <-- add this line
    };
    struct UndoGroup {
        std::vector<TileChange> changes;
    };
    std::vector<UndoGroup> undoStack;
    std::vector<UndoGroup> redoStack;
    UndoGroup currentGroup;
    bool recording = false;
    static constexpr size_t MAX_UNDO = 6;     // 5–6 levels
};
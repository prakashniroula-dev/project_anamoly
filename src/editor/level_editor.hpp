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
    void initPalette();          // tile palette
    void initObjectPalette();    // object palette

private:
    enum class EditorMode { Tile, Object };
    EditorMode currentMode = EditorMode::Tile;
    // Pagination
    int tilePage = 0;
    int objectPage = 0;
    int tileRowsPerPage = 4;
    int objectRowsPerPage = 3;

    bool stackMode = false;   // false = Replace, true = Stack
    // Mode toggle buttons (UI)
    sf::RectangleShape replaceBtn, stackBtn;
    sf::Text replaceText, stackText;


    // Navigation buttons
    sf::RectangleShape prevTileBtn, nextTileBtn;
    sf::RectangleShape prevObjectBtn, nextObjectBtn;
    sf::Text prevTileText, nextTileText, prevObjectText, nextObjectText;

    void ensureTilePageForIndex(int idx);
    void ensureObjectPageForIndex(int idx);

    // --- Tile editing ---
    void updatePaletteLayout(const sf::Vector2u& windowSize);
    // void updateHighlightPosition();
    void paintTile(int tx, int ty);
    void handleRightClickTile(int tx, int ty, bool shiftHeld);
    void setTileDirect(int tx, int ty, const std::vector<int>& tiles);
    void setTileWithUndo(int tx, int ty, int newTile, bool isPaint);
    void undoTile();
    void redoTile();
    void startRecordingTiles();
    void stopRecordingAndPushTiles();

    // --- Object editing ---
    void updateObjectPaletteLayout(const sf::Vector2u& windowSize);
    // void updateObjectHighlightPosition();
    void paintObject(float x, float y);
    void eraseObject(float x, float y);
    void setObjectDirect(float x, float y, const ObjectProps& props);
    void setObjectWithUndo(float x, float y, const ObjectProps& newProps, bool isPaint);
    void undoObject();
    void redoObject();
    void startRecordingObjects();
    void stopRecordingAndPushObjects();

    // --- General UI ---
    // void drawPalette(sf::RenderWindow& window);
    void updateObjectCursor();

    // Base (unscaled) palette parameters
    float basePaletteTileSize = 32.f;
    float baseSpacing = 4.f;
    int paletteColumns = 10;

    // Object palette parameters (smaller)
    float baseObjectPaletteTileSize = 24.f;
    float baseObjectSpacing = 3.f;
    int objectPaletteColumns = 8;

    bool active = false;

    // Tile palette
    int selectedTile = 1;
    std::vector<sf::Sprite> paletteSprites;
    sf::RectangleShape paletteBackground;
    sf::RectangleShape selectionHighlight;
    bool showPallete = true;

    // Object palette
    int selectedObject = 0;
    float currentObjectScale = 1.0f;              // scale for new objects
    std::vector<sf::Sprite> objectPaletteSprites;
    sf::RectangleShape objectPaletteBackground;
    sf::RectangleShape objectSelectionHighlight;
    bool showObjectPallete = false;               // initially hidden

    // Cursor and hover info
    sf::RectangleShape tileCursor;                // for tile mode
    sf::Texture _tempTex;
    sf::Sprite objectCursor;                      // for object mode (follows mouse)
    sf::Vector2f mouseWorldPos;                   // latest world‑space mouse position
    sf::Vector2i hoveredTile;                     // grid coords (tile mode)

    // Tile undo/redo
    struct TileChange {
        int tx, ty;
        std::vector<int> oldTiles;
        std::vector<int> newTiles;
        bool isPaint;   // or a flag to distinguish operation
    };
    struct TileUndoGroup {
        std::vector<TileChange> changes;
    };
    std::vector<TileUndoGroup> undoStackTiles, redoStackTiles;
    TileUndoGroup currentGroupTiles;
    bool recordingTiles = false;
    static constexpr size_t MAX_UNDO = 6;

    // Object undo/redo
    struct ObjectChange {
        float x, y;
        ObjectProps oldProps;
        ObjectProps newProps;
        bool isPaint;
    };
    struct ObjectUndoGroup {
        std::vector<ObjectChange> changes;
    };
    std::vector<ObjectUndoGroup> undoStackObjects, redoStackObjects;
    ObjectUndoGroup currentGroupObjects;
    bool recordingObjects = false;

    // Per‑tile single‑step undo (right‑click revert)
    std::vector<std::vector<int>> previousTiles; // per-cell stack backup
    std::vector<bool> processedInDrag;

    // UI text
    sf::Text tileInfo;
    sf::Font font;

    bool isPainting = false;
    sf::Vector2i lastPaintedTile;
    bool isErasing = false;
    sf::Vector2i lastErasedTile;
};
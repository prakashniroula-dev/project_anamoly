#include <core/constants.hpp>
#include <graphics/tiles.hpp>
#include <editor/level_editor.hpp>
#include <core/scale.hpp>
#include <iostream>
#include <algorithm>
#include <SFML/Graphics.hpp>
#include <debug/logs.hpp>
#include <cmath>
#include <entities/objects.hpp>

// ----------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------
LevelEditor::LevelEditor()
    : font(),
      tileInfo(font),
      showPallete(true),
      showObjectPallete(false),
      isErasing(false),
      lastErasedTile({-1, -1}),
      objectCursor(_tempTex),
      tilePage(0),
      objectPage(0),
      prevTileText(font),
      nextTileText(font),
        prevObjectText(font),
        nextObjectText(font),
        replaceText(font),
        stackText(font)
{
    tileCursor.setSize(sf::Vector2f(Constants::TILE_SIZE, Constants::TILE_SIZE));
    tileCursor.setFillColor(sf::Color(255, 255, 255, 100));
    tileCursor.setOutlineColor(sf::Color::Red);
    tileCursor.setOutlineThickness(2.f);

    font.openFromFile("assets/fonts/orbitron.ttf");
    tileInfo.setFont(font);
    tileInfo.setFillColor(sf::Color::White);

    int totalTiles = Constants::WORLD_WIDTH_TILES * Constants::WORLD_HEIGHT_TILES;
    previousTiles.resize(totalTiles);
    for (auto &vec : previousTiles) vec.clear();
    processedInDrag.resize(totalTiles, false);

    // Init navigation buttons
    prevTileBtn.setFillColor(sf::Color(60, 60, 60));
    prevTileBtn.setOutlineColor(sf::Color::White);
    prevTileBtn.setOutlineThickness(1.f);
    nextTileBtn.setFillColor(sf::Color(60, 60, 60));
    nextTileBtn.setOutlineColor(sf::Color::White);
    nextTileBtn.setOutlineThickness(1.f);
    prevObjectBtn = prevTileBtn;
    nextObjectBtn = nextTileBtn;

    prevTileText.setFont(font);
    prevTileText.setString("<");
    prevTileText.setFillColor(sf::Color::White);
    nextTileText.setFont(font);
    nextTileText.setString(">");
    nextTileText.setFillColor(sf::Color::White);
    prevObjectText = prevTileText;
    nextObjectText = nextTileText;

     // Mode buttons
    replaceBtn.setFillColor(sf::Color(60, 60, 60));
    replaceBtn.setOutlineColor(sf::Color::White);
    replaceBtn.setOutlineThickness(1.f);
    stackBtn = replaceBtn;

    replaceText.setFont(font);
    replaceText.setString("Replace");
    replaceText.setFillColor(sf::Color::White);
    stackText.setFont(font);
    stackText.setString("Stack");
    stackText.setFillColor(sf::Color::White);
}

// ----------------------------------------------------------------------
// Palette initialisation (tiles)
// ----------------------------------------------------------------------
void LevelEditor::initPalette()
{
    const int numTileTypes = Tiles::getCount();
    float originalWidth = Constants::TILE_SIZE;
    float originalHeight = Constants::TILE_SIZE;
    float scaleX = basePaletteTileSize / originalWidth;
    float scaleY = basePaletteTileSize / originalHeight;

    for (int i = 0; i < numTileTypes; ++i)
    {
        sf::Sprite sprite = Tiles::getTileSprite(i);
        sprite.setScale(sf::Vector2f(scaleX, scaleY));
        paletteSprites.push_back(sprite);
    }

    paletteBackground.setFillColor(sf::Color(40, 40, 40, 220));
    paletteBackground.setOutlineColor(sf::Color::White);
    paletteBackground.setOutlineThickness(1.f);

    selectionHighlight.setFillColor(sf::Color::Transparent);
    selectionHighlight.setOutlineColor(sf::Color::Yellow);
    selectionHighlight.setOutlineThickness(2.f);
}

// ----------------------------------------------------------------------
// Object palette initialisation (sprites only; layout is done each frame)
// ----------------------------------------------------------------------
void LevelEditor::initObjectPalette()
{
    const int numObjects = Objects::getCount();
    if (numObjects == 0)
        return;

    objectPaletteSprites.clear();
    for (int i = 0; i < numObjects; ++i)
    {
        sf::Sprite sprite = Objects::getObjectSprite(i);
        sprite.setOrigin(sf::Vector2f(0.f, 0.f)); // top‑left for palette cells
        objectPaletteSprites.push_back(sprite);
    }

    objectPaletteBackground.setFillColor(sf::Color(40, 40, 40, 220));
    objectPaletteBackground.setOutlineColor(sf::Color::White);
    objectPaletteBackground.setOutlineThickness(1.f);

    objectSelectionHighlight.setFillColor(sf::Color::Transparent);
    objectSelectionHighlight.setOutlineColor(sf::Color::Yellow);
    objectSelectionHighlight.setOutlineThickness(2.f);

    // Init cursor
    updateObjectCursor();
}

// ----------------------------------------------------------------------
// Layout updates (with pagination)
// ----------------------------------------------------------------------
void LevelEditor::updatePaletteLayout(const sf::Vector2u &windowSize)
{
    if (paletteSprites.empty())
        return;

    sf::Vector2f scale = Scale::getVec();
    float tileSize = basePaletteTileSize * std::min(scale.x, scale.y);
    float spacing = baseSpacing * std::min(scale.x, scale.y);

    float winWidth = static_cast<float>(windowSize.x);
    float winHeight = static_cast<float>(windowSize.y);

    const int totalTiles = static_cast<int>(paletteSprites.size());
    const int perPage = paletteColumns * tileRowsPerPage;
    const int startIdx = tilePage * perPage;
    const int endIdx = std::min(startIdx + perPage, totalTiles);
    const int numDisplayed = endIdx - startIdx;

    if (numDisplayed <= 0)
    {
        // If no tiles on this page, go to previous page
        if (tilePage > 0)
        {
            tilePage--;
            updatePaletteLayout(windowSize);
        }
        return;
    }

    // Calculate rows needed for this page
    const int rows = (numDisplayed + paletteColumns - 1) / paletteColumns;

    float totalWidth = paletteColumns * (tileSize + spacing) - spacing;
    float totalHeight = rows * (tileSize + spacing) - spacing;

    float startX = (winWidth - totalWidth) / 2.f;
    float startY = 20.f * std::min(scale.x, scale.y);

    // Position only the visible sprites
    for (int i = startIdx; i < endIdx; ++i)
    {
        int localIdx = i - startIdx;
        int row = localIdx / paletteColumns;
        int col = localIdx % paletteColumns;
        float x = startX + col * (tileSize + spacing);
        float y = startY + row * (tileSize + spacing);
        paletteSprites[i].setPosition(sf::Vector2f(x, y));
        float spriteScale = tileSize / Constants::TILE_SIZE;
        paletteSprites[i].setScale(sf::Vector2f(spriteScale, spriteScale));
    }

    // Background
    float bgWidth = totalWidth + 20.f * std::min(scale.x, scale.y);
    float bgHeight = totalHeight + 20.f * std::min(scale.x, scale.y);
    paletteBackground.setSize(sf::Vector2f(bgWidth, bgHeight));
    paletteBackground.setPosition(sf::Vector2f(startX - 10.f * std::min(scale.x, scale.y),
                                               startY - 10.f * std::min(scale.x, scale.y)));

    // Navigation buttons: placed below the background
    float btnSize = 30.f * std::min(scale.x, scale.y);
    float btnSpacing = 10.f * std::min(scale.x, scale.y);
    float totalBtnWidth = btnSize * 2 + btnSpacing;
    float btnY = paletteBackground.getPosition().y + paletteBackground.getSize().y + 10.f * std::min(scale.x, scale.y);

    prevTileBtn.setSize(sf::Vector2f(btnSize, btnSize));
    nextTileBtn.setSize(sf::Vector2f(btnSize, btnSize));
    prevTileBtn.setPosition(sf::Vector2f(startX + (totalWidth - totalBtnWidth) / 2.f, btnY));
    nextTileBtn.setPosition(sf::Vector2f(prevTileBtn.getPosition().x + btnSize + btnSpacing, btnY));

    // Button text
    float charSize = btnSize * 0.6f;
    prevTileText.setCharacterSize(static_cast<unsigned int>(charSize));
    nextTileText.setCharacterSize(static_cast<unsigned int>(charSize));
    sf::FloatRect prevBounds = prevTileText.getLocalBounds();
    sf::FloatRect nextBounds = nextTileText.getLocalBounds();
    prevTileText.setPosition(sf::Vector2f(prevTileBtn.getPosition().x + (btnSize - prevBounds.size.x) / 2.f,
                                          prevTileBtn.getPosition().y + (btnSize - prevBounds.size.y) / 2.f - 2.f));
    nextTileText.setPosition(sf::Vector2f(nextTileBtn.getPosition().x + (btnSize - nextBounds.size.x) / 2.f,
                                          nextTileBtn.getPosition().y + (btnSize - nextBounds.size.y) / 2.f - 2.f));


    // After nav buttons
    float modeBtnSize = 40.f * std::min(scale.x, scale.y);
    float modeSpacing = 10.f * std::min(scale.x, scale.y);
    float modeY = prevTileBtn.getPosition().y; // same row as nav buttons
    float modeX = nextTileBtn.getPosition().x + nextTileBtn.getSize().x + modeSpacing;

    replaceBtn.setSize(sf::Vector2f(modeBtnSize, modeBtnSize));
    stackBtn.setSize(sf::Vector2f(modeBtnSize, modeBtnSize));
    replaceBtn.setPosition(sf::Vector2f(modeX, modeY));
    stackBtn.setPosition(sf::Vector2f(modeX + modeBtnSize + modeSpacing, modeY));

    // Text
    replaceText.setCharacterSize(modeBtnSize * 0.5f);
    stackText.setCharacterSize(modeBtnSize * 0.5f);
    // Center text inside buttons
    // (similar to previous text positioning)

    // Highlight – only if selected tile is on this page
    if (selectedTile >= startIdx && selectedTile < endIdx)
    {
        const sf::Sprite &spr = paletteSprites[selectedTile];
        selectionHighlight.setSize(sf::Vector2f(tileSize, tileSize));
        selectionHighlight.setPosition(spr.getPosition());
        selectionHighlight.setFillColor(sf::Color::Transparent);
        selectionHighlight.setOutlineColor(sf::Color::Yellow);
        selectionHighlight.setOutlineThickness(2.f);
    }
    else
    {
        // Hide highlight by making it transparent and outline thin (or move offscreen)
        selectionHighlight.setFillColor(sf::Color::Transparent);
        selectionHighlight.setOutlineColor(sf::Color::Transparent);
        selectionHighlight.setOutlineThickness(0.f);
    }

    int charSizeInfo = static_cast<int>(16 * std::min(scale.x, scale.y));
    tileInfo.setCharacterSize(charSizeInfo);
}

void LevelEditor::updateObjectPaletteLayout(const sf::Vector2u &windowSize)
{
    if (objectPaletteSprites.empty())
        return;

    sf::Vector2f scale = Scale::getVec();
    float cellSize = baseObjectPaletteTileSize * std::min(scale.x, scale.y);
    float spacing = baseObjectSpacing * std::min(scale.x, scale.y);

    float winWidth = static_cast<float>(windowSize.x);
    float winHeight = static_cast<float>(windowSize.y);

    const int totalObjects = static_cast<int>(objectPaletteSprites.size());
    const int perPage = objectPaletteColumns * objectRowsPerPage;
    const int startIdx = objectPage * perPage;
    const int endIdx = std::min(startIdx + perPage, totalObjects);
    const int numDisplayed = endIdx - startIdx;

    if (numDisplayed <= 0)
    {
        if (objectPage > 0)
        {
            objectPage--;
            updateObjectPaletteLayout(windowSize);
        }
        return;
    }

    const int rows = (numDisplayed + objectPaletteColumns - 1) / objectPaletteColumns;

    float totalWidth = objectPaletteColumns * (cellSize + spacing) - spacing;
    float totalHeight = rows * (cellSize + spacing) - spacing;

    float startX = (winWidth - totalWidth) / 2.f;
    float startY = 20.f * std::min(scale.x, scale.y);

    for (int i = startIdx; i < endIdx; ++i)
    {
        int localIdx = i - startIdx;
        int row = localIdx / objectPaletteColumns;
        int col = localIdx % objectPaletteColumns;
        float x = startX + col * (cellSize + spacing);
        float y = startY + row * (cellSize + spacing);
        objectPaletteSprites[i].setPosition(sf::Vector2f(x, y));

        sf::Sprite &spr = objectPaletteSprites[i];
        sf::FloatRect bounds = spr.getLocalBounds();
        float maxDim = std::max(bounds.size.x, bounds.size.y);
        float spriteScale = (maxDim > 0) ? cellSize / maxDim : 1.f;
        spr.setScale(sf::Vector2f(spriteScale, spriteScale));
    }

    float bgWidth = totalWidth + 20.f * std::min(scale.x, scale.y);
    float bgHeight = totalHeight + 20.f * std::min(scale.x, scale.y);
    objectPaletteBackground.setSize(sf::Vector2f(bgWidth, bgHeight));
    objectPaletteBackground.setPosition(sf::Vector2f(startX - 10.f * std::min(scale.x, scale.y),
                                                     startY - 10.f * std::min(scale.x, scale.y)));

    // Navigation buttons
    float btnSize = 30.f * std::min(scale.x, scale.y);
    float btnSpacing = 10.f * std::min(scale.x, scale.y);
    float totalBtnWidth = btnSize * 2 + btnSpacing;
    float btnY = objectPaletteBackground.getPosition().y + objectPaletteBackground.getSize().y + 10.f * std::min(scale.x, scale.y);

    prevObjectBtn.setSize(sf::Vector2f(btnSize, btnSize));
    nextObjectBtn.setSize(sf::Vector2f(btnSize, btnSize));
    prevObjectBtn.setPosition(sf::Vector2f(startX + (totalWidth - totalBtnWidth) / 2.f, btnY));
    nextObjectBtn.setPosition(sf::Vector2f(prevObjectBtn.getPosition().x + btnSize + btnSpacing, btnY));

    float charSize = btnSize * 0.6f;
    prevObjectText.setCharacterSize(static_cast<unsigned int>(charSize));
    nextObjectText.setCharacterSize(static_cast<unsigned int>(charSize));
    sf::FloatRect prevBounds = prevObjectText.getLocalBounds();
    sf::FloatRect nextBounds = nextObjectText.getLocalBounds();
    prevObjectText.setPosition(sf::Vector2f(prevObjectBtn.getPosition().x + (btnSize - prevBounds.size.x) / 2.f,
                                            prevObjectBtn.getPosition().y + (btnSize - prevBounds.size.y) / 2.f - 2.f));
    nextObjectText.setPosition(sf::Vector2f(nextObjectBtn.getPosition().x + (btnSize - nextBounds.size.x) / 2.f,
                                            nextObjectBtn.getPosition().y + (btnSize - nextBounds.size.y) / 2.f - 2.f));

    // Highlight – only if selected object is on this page
    if (selectedObject >= startIdx && selectedObject < endIdx)
    {
        const sf::Sprite &spr = objectPaletteSprites[selectedObject];
        objectSelectionHighlight.setSize(sf::Vector2f(cellSize, cellSize));
        objectSelectionHighlight.setPosition(spr.getPosition());
        objectSelectionHighlight.setFillColor(sf::Color::Transparent);
        objectSelectionHighlight.setOutlineColor(sf::Color::Yellow);
        objectSelectionHighlight.setOutlineThickness(2.f);
    }
    else
    {
        objectSelectionHighlight.setFillColor(sf::Color::Transparent);
        objectSelectionHighlight.setOutlineColor(sf::Color::Transparent);
        objectSelectionHighlight.setOutlineThickness(0.f);
    }

    int charSizeInfo = static_cast<int>(16 * std::min(scale.x, scale.y));
    tileInfo.setCharacterSize(charSizeInfo);
}

// ----------------------------------------------------------------------
// Helper: ensure selected index is on its page
// ----------------------------------------------------------------------
void LevelEditor::ensureTilePageForIndex(int idx)
{
    if (paletteSprites.empty()) return;
    const int perPage = paletteColumns * tileRowsPerPage;
    int newPage = idx / perPage;
    if (newPage != tilePage)
    {
        tilePage = newPage;
        // Re-layout will happen in draw via updatePaletteLayout
    }
}

void LevelEditor::ensureObjectPageForIndex(int idx)
{
    if (objectPaletteSprites.empty()) return;
    const int perPage = objectPaletteColumns * objectRowsPerPage;
    int newPage = idx / perPage;
    if (newPage != objectPage)
    {
        objectPage = newPage;
    }
}

// ----------------------------------------------------------------------
// Tile manipulation (with separate undo stacks)
// ----------------------------------------------------------------------
void LevelEditor::paintTile(int tx, int ty)
{
    setTileWithUndo(tx, ty, selectedTile, true);
}

void LevelEditor::setTileDirect(int tx, int ty, const std::vector<int>& tiles) {
    Terrain::setTileVector(tx, ty, tiles);
}

void LevelEditor::setTileWithUndo(int tx, int ty, int newTile, bool isPaint) {
    // Get current vector
    std::vector<int> oldTiles = Terrain::getTile(tx, ty); // returns vector
    std::vector<int> newTiles;

    if (stackMode) {
        newTiles = oldTiles;
        if (newTile == -1) {
            if (!newTiles.empty()) newTiles.pop_back();
        } else {
            newTiles.push_back(newTile);
        }
    } else {
        // Replace mode
        if (newTile == -1) {
            newTiles.clear();
        } else {
            newTiles = { newTile };
        }
    }

    if (oldTiles == newTiles) return;

    // ---- Save to undo stack ----
    if (recordingTiles) {
        currentGroupTiles.changes.push_back({tx, ty, oldTiles, newTiles, isPaint});
    }

    // ---- Store previous vector for right-click restore ----
    if (isPaint) {
        int idx = ty * Constants::WORLD_WIDTH_TILES + tx;
        previousTiles[idx] = oldTiles;   // Save the old stack
    }

    // Apply to terrain
    Terrain::setTileVector(tx, ty, newTiles);
}

void LevelEditor::handleRightClickTile(int tx, int ty, bool shiftHeld) {
    int idx = ty * Constants::WORLD_WIDTH_TILES + tx;

    if (shiftHeld) {
        // Shift+Right-click: erase all tiles (like before)
        std::vector<int> empty;
        setTileWithUndo(tx, ty, -1, false); // force replace with empty
        previousTiles[idx].clear();         // clear backup
        return;
    }

    // Normal right-click: restore previous vector if available
    if (!previousTiles[idx].empty()) {
        // Get current vector
        std::vector<int> current = Terrain::getTile(tx, ty);
        // Record this as an undoable action
        if (recordingTiles) {
            currentGroupTiles.changes.push_back({tx, ty, current, previousTiles[idx], false});
        }
        Terrain::setTileVector(tx, ty, previousTiles[idx]);
        previousTiles[idx].clear();
    }
}

void LevelEditor::startRecordingTiles()
{
    currentGroupTiles.changes.clear();
    recordingTiles = true;
}

void LevelEditor::stopRecordingAndPushTiles()
{
    if (!recordingTiles)
        return;
    recordingTiles = false;
    if (currentGroupTiles.changes.empty())
        return;

    undoStackTiles.push_back(std::move(currentGroupTiles));
    if (undoStackTiles.size() > MAX_UNDO)
        undoStackTiles.erase(undoStackTiles.begin());
    redoStackTiles.clear();
}

void LevelEditor::undoTile() {
    if (undoStackTiles.empty()) return;
    TileUndoGroup group = std::move(undoStackTiles.back());
    undoStackTiles.pop_back();

    for (auto it = group.changes.rbegin(); it != group.changes.rend(); ++it) {
        const auto &change = *it;
        Terrain::setTileVector(change.tx, change.ty, change.oldTiles);
    }
    redoStackTiles.push_back(std::move(group));
}

void LevelEditor::redoTile() {
    if (redoStackTiles.empty()) return;
    TileUndoGroup group = std::move(redoStackTiles.back());
    redoStackTiles.pop_back();

    for (auto &change : group.changes) {
        Terrain::setTileVector(change.tx, change.ty, change.newTiles);
    }
    undoStackTiles.push_back(std::move(group));
}


// ----------------------------------------------------------------------
// Object manipulation (with separate undo stacks)
// ----------------------------------------------------------------------
void LevelEditor::paintObject(float x, float y)
{
    float s = Scale::get();
    ObjectProps newProps{currentObjectScale, selectedObject};
    setObjectWithUndo(x / s, y / s, newProps, true);
}

void LevelEditor::eraseObject(float x, float y)
{
    auto it = Terrain::getObjectMap().find({x, y});
    if (it == Terrain::getObjectMap().end())
        return;
    ObjectProps emptyProps{0.f, -1};
    setObjectWithUndo(x, y, emptyProps, true);
}

void LevelEditor::setObjectDirect(float x, float y, const ObjectProps &props)
{
    if (props.index < 0)
        Terrain::eraseObject(x, y);
    else
        Terrain::setObject(x, y, props);
}

void LevelEditor::setObjectWithUndo(float x, float y, const ObjectProps &newProps, bool isPaint)
{
    auto &objMap = Terrain::getObjectMap();
    auto it = objMap.find({x, y});
    ObjectProps oldProps = (it != objMap.end()) ? it->second : ObjectProps{0.f, -1};

    if (oldProps.index == newProps.index && oldProps.scale == newProps.scale)
        return;

    if (recordingObjects)
        currentGroupObjects.changes.push_back({x, y, oldProps, newProps, isPaint});

    setObjectDirect(x, y, newProps);
}

void LevelEditor::startRecordingObjects()
{
    currentGroupObjects.changes.clear();
    recordingObjects = true;
}

void LevelEditor::stopRecordingAndPushObjects()
{
    if (!recordingObjects)
        return;
    recordingObjects = false;
    if (currentGroupObjects.changes.empty())
        return;

    undoStackObjects.push_back(std::move(currentGroupObjects));
    if (undoStackObjects.size() > MAX_UNDO)
        undoStackObjects.erase(undoStackObjects.begin());
    redoStackObjects.clear();
}

void LevelEditor::undoObject()
{
    if (undoStackObjects.empty())
        return;
    ObjectUndoGroup group = std::move(undoStackObjects.back());
    undoStackObjects.pop_back();

    for (auto it = group.changes.rbegin(); it != group.changes.rend(); ++it)
    {
        const auto &change = *it;
        setObjectDirect(change.x, change.y, change.oldProps);
    }
    redoStackObjects.push_back(std::move(group));
}

void LevelEditor::redoObject()
{
    if (redoStackObjects.empty())
        return;
    ObjectUndoGroup group = std::move(redoStackObjects.back());
    redoStackObjects.pop_back();

    for (auto &change : group.changes)
        setObjectDirect(change.x, change.y, change.newProps);

    undoStackObjects.push_back(std::move(group));
}

void LevelEditor::updateObjectCursor()
{
    if (selectedObject >= 0 && selectedObject < Objects::getCount())
    {
        objectCursor = Objects::getObjectSprite(selectedObject);
        sf::FloatRect bounds = objectCursor.getLocalBounds();
        objectCursor.setOrigin(sf::Vector2f(bounds.size.x / 2.f, bounds.size.y / 2.f));
        objectCursor.setScale(sf::Vector2f(1.f, 1.f));
    }
}

// ----------------------------------------------------------------------
// Event handling
// ----------------------------------------------------------------------
void LevelEditor::handleEvent(const sf::Event &event, sf::RenderWindow &window)
{
    if (!active)
        return;

    if (currentMode == EditorMode::Tile && showPallete)
        updatePaletteLayout(window.getSize());
    else if (currentMode == EditorMode::Object && showObjectPallete)
        updateObjectPaletteLayout(window.getSize());

    sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
    sf::Vector2f mouseDefault = window.mapPixelToCoords(mousePixel, window.getDefaultView());
    sf::Vector2f mouseWorld = window.mapPixelToCoords(mousePixel);
    mouseWorldPos = mouseWorld;

    float s = Scale::get();
    int tx = static_cast<int>(std::floor(mouseWorld.x / (Constants::TILE_SIZE * s)));
    int ty = static_cast<int>(std::floor((Constants::WORLD_HEIGHT_PIXELS * s - mouseWorld.y) / (Constants::TILE_SIZE * s)));
    tx = std::clamp(tx, 0, Constants::WORLD_WIDTH_TILES - 1);
    ty = std::clamp(ty, 0, Constants::WORLD_HEIGHT_TILES - 1);
    hoveredTile = {tx, ty};

    bool shiftHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LShift) ||
                     sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RShift);

    // --- Keyboard shortcuts ---
    if (const auto *keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        if (keyPressed->scancode == sf::Keyboard::Scancode::F1)
        {
            if (recordingTiles)
                stopRecordingAndPushTiles();
            if (recordingObjects)
                stopRecordingAndPushObjects();
            currentMode = EditorMode::Tile;
            showPallete = true;
            showObjectPallete = false;
        }
        else if (keyPressed->scancode == sf::Keyboard::Scancode::F2)
        {
            if (recordingTiles)
                stopRecordingAndPushTiles();
            if (recordingObjects)
                stopRecordingAndPushObjects();
            currentMode = EditorMode::Object;
            showObjectPallete = true;
            showPallete = false;
            updateObjectCursor();
        }
        else if (keyPressed->scancode == sf::Keyboard::Scancode::E)
        {
            if (currentMode == EditorMode::Tile)
                showPallete = !showPallete;
        }
        else if (keyPressed->scancode == sf::Keyboard::Scancode::O)
        {
            if (currentMode == EditorMode::Object)
                showObjectPallete = !showObjectPallete;
        }
        else if (keyPressed->scancode == sf::Keyboard::Scancode::U)
        {
            if (currentMode == EditorMode::Tile)
            {
                if (recordingTiles)
                    stopRecordingAndPushTiles();
                undoTile();
            }
            else
            {
                if (recordingObjects)
                    stopRecordingAndPushObjects();
                undoObject();
            }
        }
        else if (keyPressed->scancode == sf::Keyboard::Scancode::R)
        {
            if (currentMode == EditorMode::Tile)
            {
                if (recordingTiles)
                    stopRecordingAndPushTiles();
                redoTile();
            }
            else
            {
                if (recordingObjects)
                    stopRecordingAndPushObjects();
                redoObject();
            }
        } else if (keyPressed->scancode == sf::Keyboard::Scancode::M) {
            stackMode = !stackMode;
        }
    }

    // --- Mouse presses ---
    if (const auto *btnPressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (btnPressed->button == sf::Mouse::Button::Left)
        {
            if (currentMode == EditorMode::Tile)
            {
                bool clickedPalette = false;
                if (showPallete)
                {
                    // Check navigation buttons first
                    sf::FloatRect prevBounds = prevTileBtn.getGlobalBounds();
                    sf::FloatRect nextBounds = nextTileBtn.getGlobalBounds();
                    if (prevBounds.contains(static_cast<sf::Vector2f>(mouseDefault)))
                    {
                        if (tilePage > 0)
                        {
                            tilePage--;
                            // Ensure selected tile is on new page if possible
                            int perPage = paletteColumns * tileRowsPerPage;
                            if (selectedTile >= tilePage * perPage && selectedTile < (tilePage + 1) * perPage)
                            {
                                // keep selection
                            }
                            else
                            {
                                // Select first tile of new page
                                int newStart = tilePage * perPage;
                                if (newStart < (int)paletteSprites.size())
                                    selectedTile = newStart;
                                else
                                    selectedTile = 0;
                            }
                        }
                        clickedPalette = true;
                    }
                    else if (nextBounds.contains(static_cast<sf::Vector2f>(mouseDefault)))
                    {
                        int totalTiles = paletteSprites.size();
                        int perPage = paletteColumns * tileRowsPerPage;
                        if ((tilePage + 1) * perPage < totalTiles)
                        {
                            tilePage++;
                            int newStart = tilePage * perPage;
                            if (newStart < totalTiles)
                                selectedTile = newStart;
                        }
                        clickedPalette = true;
                    }
                    // After handling nav buttons, before palette sprite checks
                    else if (replaceBtn.getGlobalBounds().contains(mouseDefault)) {
                        stackMode = false;
                        clickedPalette = true; // to prevent painting
                    }
                    else if (stackBtn.getGlobalBounds().contains(mouseDefault)) {
                        stackMode = true;
                        clickedPalette = true;
                    }
                    else
                    {
                        const int perPage = paletteColumns * tileRowsPerPage;
                        const int startIdx = tilePage * perPage;
                        const int endIdx = std::min(startIdx + perPage, (int)paletteSprites.size());

                        for (int i = startIdx; i < endIdx; ++i)
                        {
                            if (paletteSprites[i].getGlobalBounds().contains(static_cast<sf::Vector2f>(mouseDefault)))
                            {
                                selectedTile = i;
                                clickedPalette = true;
                                break;
                            }
                        }
                    }
                }
                if (clickedPalette)
                    return;

                startRecordingTiles();
                isPainting = true;
                lastPaintedTile = {-1, -1};

                int tileToPaint = shiftHeld ? -1 : selectedTile;
                setTileWithUndo(tx, ty, tileToPaint, true);
            }
            else // Object mode
            {
                bool clickedObjectPalette = false;
                if (showObjectPallete)
                {
                    // Check object nav buttons
                    sf::FloatRect prevBounds = prevObjectBtn.getGlobalBounds();
                    sf::FloatRect nextBounds = nextObjectBtn.getGlobalBounds();
                    if (prevBounds.contains(static_cast<sf::Vector2f>(mouseDefault)))
                    {
                        if (objectPage > 0)
                        {
                            objectPage--;
                            int perPage = objectPaletteColumns * objectRowsPerPage;
                            if (selectedObject >= objectPage * perPage && selectedObject < (objectPage + 1) * perPage)
                            {
                                // keep
                            }
                            else
                            {
                                int newStart = objectPage * perPage;
                                if (newStart < (int)objectPaletteSprites.size())
                                    selectedObject = newStart;
                                else
                                    selectedObject = 0;
                            }
                        }
                        clickedObjectPalette = true;
                    }
                    else if (nextBounds.contains(static_cast<sf::Vector2f>(mouseDefault)))
                    {
                        int totalObjects = objectPaletteSprites.size();
                        int perPage = objectPaletteColumns * objectRowsPerPage;
                        if ((objectPage + 1) * perPage < totalObjects)
                        {
                            objectPage++;
                            int newStart = objectPage * perPage;
                            if (newStart < totalObjects)
                                selectedObject = newStart;
                        }
                        clickedObjectPalette = true;
                    }
                    else
                    {
                        const int perPage = objectPaletteColumns * objectRowsPerPage;
                        const int startIdx = objectPage * perPage;
                        const int endIdx = std::min(startIdx + perPage, (int)objectPaletteSprites.size());

                        for (int i = startIdx; i < endIdx; ++i)
                        {
                            if (objectPaletteSprites[i].getGlobalBounds().contains(static_cast<sf::Vector2f>(mouseDefault)))
                            {
                                selectedObject = i;
                                updateObjectCursor(); // if you want immediate cursor update
                                clickedObjectPalette = true;
                                break;
                            }
                        }
                    }
                }
                if (clickedObjectPalette)
                    return;

                // Place object centered at mouse
                startRecordingObjects();
                paintObject(mouseWorld.x, mouseWorld.y);
                stopRecordingAndPushObjects();
            }
        }
        else if (btnPressed->button == sf::Mouse::Button::Right)
        {
            if (currentMode == EditorMode::Tile)
            {
                startRecordingTiles();
                isErasing = true;
                lastErasedTile = {-1, -1};
                std::fill(processedInDrag.begin(), processedInDrag.end(), false);

                int index = ty * Constants::WORLD_WIDTH_TILES + tx;
                handleRightClickTile(tx, ty, false);
                processedInDrag[index] = true;
            }
            else // Object mode erase
            {
                const auto &order = Terrain::getObjectOrder();
                for (auto it = order.rbegin(); it != order.rend(); ++it)
                {
                    const auto &key = *it;
                    auto mapIt = Terrain::getObjectMap().find(key);
                    if (mapIt == Terrain::getObjectMap().end())
                        continue;
                    const auto &props = mapIt->second;
                    sf::Sprite spr = Objects::getObjectSprite(props.index);
                    sf::FloatRect bounds = spr.getLocalBounds();
                    spr.setOrigin(sf::Vector2f(bounds.size.x / 2.f, bounds.size.y / 2.f));
                    sf::Vector2f worldPos(key.first * Scale::get(), key.second * Scale::get());
                    spr.setPosition(worldPos);
                    spr.setScale(Scale::getVec() * props.scale);
                    if (spr.getGlobalBounds().contains(mouseWorld))
                    {
                        startRecordingObjects();
                        eraseObject(key.first, key.second);
                        stopRecordingAndPushObjects();
                        break;
                    }
                }
            }
        }
    }

    // --- Mouse release ---
    if (const auto *btnReleased = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (btnReleased->button == sf::Mouse::Button::Left)
        {
            if (currentMode == EditorMode::Tile)
            {
                isPainting = false;
                stopRecordingAndPushTiles();
            }
        }
        if (btnReleased->button == sf::Mouse::Button::Right)
        {
            if (currentMode == EditorMode::Tile)
            {
                isErasing = false;
                stopRecordingAndPushTiles();
            }
        }
    }

    // --- Mouse move (drag for tiles) ---
    if (const auto *mouseMoved = event.getIf<sf::Event::MouseMoved>())
    {
        if (currentMode == EditorMode::Tile)
        {
            if (isPainting)
            {
                if (tx != lastPaintedTile.x || ty != lastPaintedTile.y)
                {
                    bool shiftNow = sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LShift) ||
                                    sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RShift);
                    int tileToPaint = shiftNow ? -1 : selectedTile;
                    setTileWithUndo(tx, ty, tileToPaint, true);
                    lastPaintedTile = {tx, ty};
                }
            }
            else if (isErasing)
            {
                if (tx != lastErasedTile.x || ty != lastErasedTile.y)
                {
                    int index = ty * Constants::WORLD_WIDTH_TILES + tx;
                    if (!processedInDrag[index])
                    {
                        handleRightClickTile(tx, ty, false);
                        processedInDrag[index] = true;
                    }
                    lastErasedTile = {tx, ty};
                }
            }
        }
    }

    // --- Mouse wheel ---
    if (const auto *scroll = event.getIf<sf::Event::MouseWheelScrolled>())
    {
        if (currentMode == EditorMode::Tile)
        {
            if (!paletteSprites.empty())
            {
                selectedTile += static_cast<int>(scroll->delta);
                selectedTile = std::clamp(selectedTile, 0, (int)paletteSprites.size() - 1);
                ensureTilePageForIndex(selectedTile);
                // updateHighlightPosition();
            }
        }
        else // Object mode
        {
            if (shiftHeld)
            {
                currentObjectScale += scroll->delta * 0.1f;
                currentObjectScale = std::clamp(currentObjectScale, 0.1f, 5.0f);
            }
            else if (!objectPaletteSprites.empty())
            {
                selectedObject += static_cast<int>(scroll->delta);
                selectedObject = std::clamp(selectedObject, 0, (int)objectPaletteSprites.size() - 1);
                ensureObjectPageForIndex(selectedObject);
                // updateObjectHighlightPosition();
                updateObjectCursor();
            }
        }
    }
}

// ----------------------------------------------------------------------
// Drawing
// ----------------------------------------------------------------------
void LevelEditor::draw(sf::RenderWindow &window)
{
    if (!active)
        return;

    float s = Scale::get();
    sf::View originalView = window.getView();

    // ---- World drawing ----
    if (currentMode == EditorMode::Tile)
    {
        if (!showPallete)
        {
            for (int x = 0; x <= Constants::WORLD_WIDTH_TILES; ++x)
            {
                sf::Vertex line[] = {
                    {{x * Constants::TILE_SIZE * s, 0.f}, sf::Color(200, 200, 200, 100)},
                    {{x * Constants::TILE_SIZE * s, Constants::WORLD_HEIGHT_PIXELS * s}, sf::Color(200, 200, 200, 100)}};
                window.draw(line, 2, sf::PrimitiveType::Lines);
            }
            for (int y = 0; y <= Constants::WORLD_HEIGHT_TILES; ++y)
            {
                sf::Vertex line[] = {
                    {{0.f, y * Constants::TILE_SIZE * s}, sf::Color(200, 200, 200, 100)},
                    {{Constants::WORLD_WIDTH_PIXELS * s, y * Constants::TILE_SIZE * s}, sf::Color(200, 200, 200, 100)}};
                window.draw(line, 2, sf::PrimitiveType::Lines);
            }
        }

         sf::Vector2f cursorPos(
            hoveredTile.x * Constants::TILE_SIZE * s,
            Constants::WORLD_HEIGHT_PIXELS * s - (hoveredTile.y + 1) * Constants::TILE_SIZE * s
        );

        bool shiftHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LShift) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RShift);

        if (selectedTile >= 0 && !shiftHeld)  // show ghost tile
        {
            sf::Sprite ghost = Tiles::getTileSprite(selectedTile);
            ghost.setPosition(cursorPos);
            ghost.setScale(Scale::getVec());
            ghost.setColor(sf::Color(255, 255, 255, 150)); // ~60% opacity
            window.draw(ghost);
        }
        else  // erasing mode – show red rectangle
        {
            tileCursor.setPosition(cursorPos);
            tileCursor.setScale(Scale::getVec());
            tileCursor.setFillColor(sf::Color(255, 0, 0, 100));
            tileCursor.setOutlineColor(sf::Color::Red);
            tileCursor.setOutlineThickness(2.f);
            window.draw(tileCursor);
        }
    }
    else // Object mode
    {
        if (selectedObject >= 0 && selectedObject < Objects::getCount())
        {
            objectCursor.setPosition(mouseWorldPos);
            sf::Vector2f worldScale = Scale::getVec();
            objectCursor.setScale(worldScale * currentObjectScale);
            window.draw(objectCursor);
        }
    }

    // ---- UI (default view) ----
    window.setView(window.getDefaultView());

    if (currentMode == EditorMode::Tile)
    {
        updatePaletteLayout(window.getSize());
        if (showPallete)
        {
            window.draw(paletteBackground);
            const int perPage = paletteColumns * tileRowsPerPage;
            const int startIdx = tilePage * perPage;
            const int endIdx = std::min(startIdx + perPage, (int)paletteSprites.size());
            for (int i = startIdx; i < endIdx; ++i)
                window.draw(paletteSprites[i]);
            window.draw(selectionHighlight);
            // Draw navigation buttons
            window.draw(prevTileBtn);
            window.draw(nextTileBtn);
            window.draw(prevTileText);
            window.draw(nextTileText);

            // In the tile palette drawing block, after nav buttons:
            replaceBtn.setFillColor(stackMode ? sf::Color(60,60,60) : sf::Color(100,100,200));
            stackBtn.setFillColor(stackMode ? sf::Color(100,100,200) : sf::Color(60,60,60));
            window.draw(replaceBtn);
            window.draw(stackBtn);
            window.draw(replaceText);
            window.draw(stackText);
        }
    }
    else
    {
        updateObjectPaletteLayout(window.getSize());
        if (showObjectPallete)
        {
            window.draw(objectPaletteBackground);
            const int perPage = objectPaletteColumns * objectRowsPerPage;
            const int startIdx = objectPage * perPage;
            const int endIdx = std::min(startIdx + perPage, (int)objectPaletteSprites.size());
            for (int i = startIdx; i < endIdx; ++i)
                window.draw(objectPaletteSprites[i]);
            window.draw(objectSelectionHighlight);
            window.draw(prevObjectBtn);
            window.draw(nextObjectBtn);
            window.draw(prevObjectText);
            window.draw(nextObjectText);
        }
    }

    window.setView(originalView);
}

// ----------------------------------------------------------------------
// Activation
// ----------------------------------------------------------------------
void LevelEditor::setActive(bool a) { active = a; }
bool LevelEditor::isActive() const { return active; }
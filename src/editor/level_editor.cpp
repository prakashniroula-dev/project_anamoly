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
      objectCursor(_tempTex)
{
    tileCursor.setSize(sf::Vector2f(Constants::TILE_SIZE, Constants::TILE_SIZE));
    tileCursor.setFillColor(sf::Color(255, 255, 255, 100));
    tileCursor.setOutlineColor(sf::Color::Red);
    tileCursor.setOutlineThickness(2.f);

    font.openFromFile("assets/fonts/orbitron.ttf");
    tileInfo.setFont(font);
    tileInfo.setFillColor(sf::Color::White);

    int totalTiles = Constants::WORLD_WIDTH_TILES * Constants::WORLD_HEIGHT_TILES;
    previousTiles.resize(totalTiles, -1);
    processedInDrag.resize(totalTiles, false);
}

// ----------------------------------------------------------------------
// Palette initialisation (tiles)
// ----------------------------------------------------------------------
void LevelEditor::initPalette()
{
    const int numTileTypes = 64 + 1;
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
// Layout updates
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

    const int numTiles = static_cast<int>(paletteSprites.size());
    const int columns = paletteColumns;
    const int rows = (numTiles + columns - 1) / columns;

    float totalWidth = columns * (tileSize + spacing) - spacing;
    float totalHeight = rows * (tileSize + spacing) - spacing;

    float startX = (winWidth - totalWidth) / 2.f;
    float startY = 20.f * std::min(scale.x, scale.y);

    for (int i = 0; i < numTiles; ++i)
    {
        int row = i / columns;
        int col = i % columns;
        float x = startX + col * (tileSize + spacing);
        float y = startY + row * (tileSize + spacing);
        paletteSprites[i].setPosition(sf::Vector2f(x, y));
        float spriteScale = tileSize / Constants::TILE_SIZE;
        paletteSprites[i].setScale(sf::Vector2f(spriteScale, spriteScale));
    }

    float bgWidth = totalWidth + 20.f * std::min(scale.x, scale.y);
    float bgHeight = totalHeight + 20.f * std::min(scale.x, scale.y);
    paletteBackground.setSize(sf::Vector2f(bgWidth, bgHeight));
    paletteBackground.setPosition(sf::Vector2f(startX - 10.f * std::min(scale.x, scale.y),
                                               startY - 10.f * std::min(scale.x, scale.y)));

    selectionHighlight.setSize(sf::Vector2f(tileSize, tileSize));
    updateHighlightPosition();

    int charSize = static_cast<int>(16 * std::min(scale.x, scale.y));
    tileInfo.setCharacterSize(charSize);
}

void LevelEditor::updateHighlightPosition()
{
    if (selectedTile >= 0 && selectedTile < (int)paletteSprites.size())
    {
        const sf::Sprite &spr = paletteSprites[selectedTile];
        selectionHighlight.setPosition(spr.getPosition());
    }
}

// Object palette layout – smaller cells, sprites scaled to fit
void LevelEditor::updateObjectPaletteLayout(const sf::Vector2u &windowSize)
{
    if (objectPaletteSprites.empty())
        return;

    sf::Vector2f scale = Scale::getVec();
    float cellSize = baseObjectPaletteTileSize * std::min(scale.x, scale.y);
    float spacing = baseObjectSpacing * std::min(scale.x, scale.y);

    float winWidth = static_cast<float>(windowSize.x);
    float winHeight = static_cast<float>(windowSize.y);

    const int numObjects = static_cast<int>(objectPaletteSprites.size());
    const int columns = objectPaletteColumns;
    const int rows = (numObjects + columns - 1) / columns;

    float totalWidth = columns * (cellSize + spacing) - spacing;
    float totalHeight = rows * (cellSize + spacing) - spacing;

    float startX = (winWidth - totalWidth) / 2.f;
    float startY = 20.f * std::min(scale.x, scale.y);

    for (int i = 0; i < numObjects; ++i)
    {
        int row = i / columns;
        int col = i % columns;
        float x = startX + col * (cellSize + spacing);
        float y = startY + row * (cellSize + spacing);
        objectPaletteSprites[i].setPosition(sf::Vector2f(x, y));

        // Scale sprite to fit inside cell while preserving aspect
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

    objectSelectionHighlight.setSize(sf::Vector2f(cellSize, cellSize));
    updateObjectHighlightPosition();

    int charSize = static_cast<int>(16 * std::min(scale.x, scale.y));
    tileInfo.setCharacterSize(charSize);
}

void LevelEditor::updateObjectHighlightPosition()
{
    if (selectedObject >= 0 && selectedObject < (int)objectPaletteSprites.size())
    {
        const sf::Sprite &spr = objectPaletteSprites[selectedObject];
        // Highlight covers the cell (position is top‑left of cell)
        objectSelectionHighlight.setPosition(spr.getPosition());
    }
}

// ----------------------------------------------------------------------
// Tile manipulation (with separate undo stacks)
// ----------------------------------------------------------------------
void LevelEditor::paintTile(int tx, int ty)
{
    setTileWithUndo(tx, ty, selectedTile, true);
}

void LevelEditor::setTileDirect(int tx, int ty, int newTile)
{
    Terrain::setTile(tx, ty, newTile);
}

void LevelEditor::setTileWithUndo(int tx, int ty, int newTile, bool isPaint)
{
    int oldTile = Terrain::getTile(tx, ty);
    if (oldTile == newTile)
        return;

    if (recordingTiles)
        currentGroupTiles.changes.push_back({tx, ty, oldTile, newTile, isPaint});

    Terrain::setTile(tx, ty, newTile);

    int index = ty * Constants::WORLD_WIDTH_TILES + tx;
    if (isPaint)
        previousTiles[index] = oldTile;
    else
        previousTiles[index] = -1;
}

void LevelEditor::handleRightClickTile(int tx, int ty, bool shiftHeld)
{
    int index = ty * Constants::WORLD_WIDTH_TILES + tx;
    int currentTile = Terrain::getTile(tx, ty);

    int newTileId;
    if (shiftHeld)
    {
        newTileId = -1;
        previousTiles[index] = -1;
    }
    else
    {
        if (currentTile != -1 && previousTiles[index] != -1)
        {
            newTileId = previousTiles[index];
            previousTiles[index] = -1;
        }
        else
            return;
    }
    setTileWithUndo(tx, ty, newTileId, false);
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

void LevelEditor::undoTile()
{
    if (undoStackTiles.empty())
        return;
    TileUndoGroup group = std::move(undoStackTiles.back());
    undoStackTiles.pop_back();

    for (auto it = group.changes.rbegin(); it != group.changes.rend(); ++it)
    {
        const auto &change = *it;
        setTileDirect(change.tx, change.ty, change.oldTile);

        int index = change.ty * Constants::WORLD_WIDTH_TILES + change.tx;
        if (change.isPaint)
            previousTiles[index] = -1;
        else
            previousTiles[index] = change.newTile;
    }
    redoStackTiles.push_back(std::move(group));
}

void LevelEditor::redoTile()
{
    if (redoStackTiles.empty())
        return;
    TileUndoGroup group = std::move(redoStackTiles.back());
    redoStackTiles.pop_back();

    for (auto &change : group.changes)
    {
        setTileDirect(change.tx, change.ty, change.newTile);

        int index = change.ty * Constants::WORLD_WIDTH_TILES + change.tx;
        if (change.isPaint)
            previousTiles[index] = change.oldTile;
        else
            previousTiles[index] = -1;
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
    // Store normalized coordinates
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

    sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
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
                    for (size_t i = 0; i < paletteSprites.size(); ++i)
                    {
                        if (paletteSprites[i].getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePixel)))
                        {
                            selectedTile = static_cast<int>(i);
                            updateHighlightPosition();
                            clickedPalette = true;
                            break;
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
                    for (size_t i = 0; i < objectPaletteSprites.size(); ++i)
                    {
                        if (objectPaletteSprites[i].getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePixel)))
                        {
                            selectedObject = static_cast<int>(i);
                            updateObjectHighlightPosition();
                            updateObjectCursor();
                            clickedObjectPalette = true;
                            break;
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
                // Iterate reverse order (newest first) to pick topmost object
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
        // Object mode cursor updates automatically via mouseWorldPos in draw
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
                updateHighlightPosition();
            }
        }
        else // Object mode
        {
            if (shiftHeld)
            {
                // Change placement scale
                currentObjectScale += scroll->delta * 0.1f;
                currentObjectScale = std::clamp(currentObjectScale, 0.1f, 5.0f);
            }
            else if (!objectPaletteSprites.empty())
            {
                selectedObject += static_cast<int>(scroll->delta);
                selectedObject = std::clamp(selectedObject, 0, (int)objectPaletteSprites.size() - 1);
                updateObjectHighlightPosition();
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
            // Draw grid lines
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

        // Tile cursor
        sf::Vector2f cursorPos(hoveredTile.x * Constants::TILE_SIZE * s,
                               Constants::WORLD_HEIGHT_PIXELS * s - (hoveredTile.y + 1) * Constants::TILE_SIZE * s);
        tileCursor.setPosition(cursorPos);
        tileCursor.setScale(Scale::getVec());
        window.draw(tileCursor);
    }
    else // Object mode
    {
        if (selectedObject >= 0 && selectedObject < Objects::getCount())
        {
            objectCursor.setPosition(mouseWorldPos);
            // Apply world scale and the current object scale
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
            for (const auto &sprite : paletteSprites)
                window.draw(sprite);
            window.draw(selectionHighlight);
        }
    }
    else
    {
        updateObjectPaletteLayout(window.getSize());
        if (showObjectPallete)
        {
            window.draw(objectPaletteBackground);
            for (const auto &sprite : objectPaletteSprites)
                window.draw(sprite);
            window.draw(objectSelectionHighlight);
        }
    }

    // ---- Info text ----
    float minScale = std::min(Scale::getVec().x, Scale::getVec().y);
    tileInfo.setPosition(sf::Vector2f(10.f * minScale, 10.f * minScale));
    std::string modeStr = (currentMode == EditorMode::Tile) ? "TILE" : "OBJECT";
    std::string selectedStr;
    std::string hoverStr;
    if (currentMode == EditorMode::Tile)
    {
        selectedStr = "Tile ID: " + std::to_string(selectedTile);
        hoverStr = "Hovered Tile: (" + std::to_string(hoveredTile.x) + ", " + std::to_string(hoveredTile.y) + ")";
    }
    else
    {
        selectedStr = "Object ID: " + std::to_string(selectedObject) + "  Scale: " + std::to_string(currentObjectScale);
        hoverStr = "Mouse Pos: (" + std::to_string(mouseWorldPos.x) + ", " + std::to_string(mouseWorldPos.y) + ")";
    }
    tileInfo.setString(
        "[F1: tiles] [F2: objects]  [E: toggle tile palette] [O: toggle object palette]\n"
        "[U: undo] [R: redo]\n"
        "Mode: " +
        modeStr +
        "\n" + selectedStr +
        "\n" + hoverStr);
    window.draw(tileInfo);

    window.setView(originalView);
}

// ----------------------------------------------------------------------
// Activation
// ----------------------------------------------------------------------
void LevelEditor::setActive(bool a) { active = a; }
bool LevelEditor::isActive() const { return active; }
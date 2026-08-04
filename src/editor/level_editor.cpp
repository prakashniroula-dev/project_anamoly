// src/editor/LevelEditor.cpp
#include <core/constants.hpp>
#include <graphics/tiles.hpp>
#include <editor/level_editor.hpp>
#include <core/scale.hpp>
#include <iostream>
#include <algorithm>
#include <SFML/Graphics.hpp>
#include <debug/logs.hpp>
#include <cmath>

// ----------------------------------------------------------------------
// IMPORTANT: The UndoChange struct in LevelEditor.hpp must include:
//     bool isPaint;   // true if this was a paint/erase, false if it was a revert
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------
LevelEditor::LevelEditor()
    : font(),
      tileInfo(font),
      showPallete(true),
      isErasing(false),
      lastErasedTile({-1, -1})
{
    cursor.setSize(sf::Vector2f(Constants::TILE_SIZE, Constants::TILE_SIZE));
    cursor.setFillColor(sf::Color(255, 255, 255, 100));
    cursor.setOutlineColor(sf::Color::Red);
    cursor.setOutlineThickness(2.f);

    font.openFromFile("assets/fonts/orbitron.ttf");
    tileInfo.setFont(font);
    tileInfo.setFillColor(sf::Color::White);

    // Allocate per‑tile history arrays
    int totalTiles = Constants::WORLD_WIDTH_TILES * Constants::WORLD_HEIGHT_TILES;
    previousTiles.resize(totalTiles, -1);
    processedInDrag.resize(totalTiles, false);
}

// ----------------------------------------------------------------------
// Palette initialisation
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
// Layout update
// ----------------------------------------------------------------------
void LevelEditor::updatePaletteLayout(const sf::Vector2u& windowSize)
{
    if (paletteSprites.empty()) return;

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

// ----------------------------------------------------------------------
// Tile manipulation
// ----------------------------------------------------------------------
void LevelEditor::paintTile(int tx, int ty)
{
    // This is called from left‑click painting; we use setTileWithUndo now.
    setTileWithUndo(tx, ty, selectedTile, true);
}

void LevelEditor::setTileDirect(int tx, int ty, int newTile)
{
    Terrain::setTile(tx, ty, newTile);
}

void LevelEditor::setTileWithUndo(int tx, int ty, int newTile, bool isPaint)
{
    int oldTile = Terrain::getTile(tx, ty);
    if (oldTile == newTile) return;

    // Record for multi‑level undo
    if (recording) {
        currentGroup.changes.push_back({tx, ty, oldTile, newTile, isPaint});
    }

    Terrain::setTile(tx, ty, newTile);

    // Update per‑tile one‑level undo history
    int index = ty * Constants::WORLD_WIDTH_TILES + tx;
    if (isPaint) {
        previousTiles[index] = oldTile;   // store old tile for right‑click undo
    } else {
        previousTiles[index] = -1;        // right‑click action consumed the history
    }
}

// ----------------------------------------------------------------------
// Right‑click action (one‑level undo / erase)
// ----------------------------------------------------------------------
void LevelEditor::handleRightClickAction(int tx, int ty, bool shiftHeld)
{
    int index = ty * Constants::WORLD_WIDTH_TILES + tx;
    int currentTile = Terrain::getTile(tx, ty);

    int newTileId;
    if (shiftHeld) {
        // (Optional) Shift+RightClick still erases – you may remove this branch
        newTileId = -1;
        previousTiles[index] = -1;
    } else {
        // Normal right‑click: only revert if history exists
        if (currentTile != -1 && previousTiles[index] != -1) {
            newTileId = previousTiles[index];
            previousTiles[index] = -1;   // history consumed
        } else {
            return;   // no history → do nothing
        }
    }

    // Apply the change (isPaint = false because this is a revert)
    setTileWithUndo(tx, ty, newTileId, false);
}

// ----------------------------------------------------------------------
// Undo / Redo (multi‑level)
// ----------------------------------------------------------------------
void LevelEditor::startRecording()
{
    currentGroup.changes.clear();
    recording = true;
}

void LevelEditor::stopRecordingAndPush()
{
    if (!recording) return;
    recording = false;
    if (currentGroup.changes.empty()) return;

    undoStack.push_back(std::move(currentGroup));
    if (undoStack.size() > MAX_UNDO)
        undoStack.erase(undoStack.begin());
    redoStack.clear();   // new action invalidates redo
}

void LevelEditor::undo()
{
    if (undoStack.empty()) return;
    UndoGroup group = std::move(undoStack.back());
    undoStack.pop_back();

    // Apply in reverse order (restore old tiles)
    for (auto it = group.changes.rbegin(); it != group.changes.rend(); ++it) {
        const auto &change = *it;
        setTileDirect(change.tx, change.ty, change.oldTile);

        // Update per‑tile history to stay consistent
        int index = change.ty * Constants::WORLD_WIDTH_TILES + change.tx;
        if (change.isPaint) {
            // Undoing a paint/erase → clear the per‑tile history
            previousTiles[index] = -1;
        } else {
            // Undoing a revert → we restore the painted tile,
            // so the previous tile (for right‑click) should be the one before that paint
            previousTiles[index] = change.newTile;
        }
    }
    redoStack.push_back(std::move(group));
}

void LevelEditor::redo()
{
    if (redoStack.empty()) return;
    UndoGroup group = std::move(redoStack.back());
    redoStack.pop_back();

    // Apply in forward order (restore new tiles)
    for (auto &change : group.changes) {
        setTileDirect(change.tx, change.ty, change.newTile);

        // Update per‑tile history to stay consistent
        int index = change.ty * Constants::WORLD_WIDTH_TILES + change.tx;
        if (change.isPaint) {
            // Redoing a paint/erase → set previous tile to the tile before it
            previousTiles[index] = change.oldTile;
        } else {
            // Redoing a revert → the history is consumed again
            previousTiles[index] = -1;
        }
    }
    undoStack.push_back(std::move(group));
}

// ----------------------------------------------------------------------
// Event handling
// ----------------------------------------------------------------------
void LevelEditor::handleEvent(const sf::Event &event, sf::RenderWindow &window)
{
    if (!active) return;

    sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
    sf::Vector2f mouseWorld = window.mapPixelToCoords(mousePixel);
    float s = Scale::get();

    int tx = static_cast<int>(std::floor(mouseWorld.x / (Constants::TILE_SIZE * s)));
    int ty = static_cast<int>(std::floor((Constants::WORLD_HEIGHT_PIXELS * s - mouseWorld.y) / (Constants::TILE_SIZE * s)));
    tx = std::clamp(tx, 0, Constants::WORLD_WIDTH_TILES - 1);
    ty = std::clamp(ty, 0, Constants::WORLD_HEIGHT_TILES - 1);
    hoveredTile = {tx, ty};

    bool shiftHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LShift) ||
                     sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RShift);

    // Keyboard shortcuts
    if (const auto *keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->scancode == sf::Keyboard::Scancode::E)
            showPallete = !showPallete;
        if (keyPressed->scancode == sf::Keyboard::Scancode::U)
            undo();
        if (keyPressed->scancode == sf::Keyboard::Scancode::R)
            redo();
    }

    // Mouse press
    if (const auto *btnPressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (btnPressed->button == sf::Mouse::Button::Left) {
            bool clickedPalette = false;
            if (showPallete) {
                for (size_t i = 0; i < paletteSprites.size(); ++i) {
                    if (paletteSprites[i].getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePixel))) {
                        selectedTile = static_cast<int>(i);
                        updateHighlightPosition();
                        clickedPalette = true;
                        break;
                    }
                }
            }
            if (clickedPalette) return;

            startRecording();
            isPainting = true;
            lastPaintedTile = {-1, -1};

            // Shift + Left = erase, else paint selected tile
            int tileToPaint = shiftHeld ? -1 : selectedTile;
            setTileWithUndo(tx, ty, tileToPaint, true);
        }
        else if (btnPressed->button == sf::Mouse::Button::Right) {
            startRecording();
            isErasing = true;
            lastErasedTile = {-1, -1};

            // Reset processed flags for this drag
            std::fill(processedInDrag.begin(), processedInDrag.end(), false);

            // Handle initial tile (revert only – we ignore shift for right‑click)
            int index = ty * Constants::WORLD_WIDTH_TILES + tx;
            handleRightClickAction(tx, ty, false);   // always revert, never erase
            processedInDrag[index] = true;
        }
    }

    // Mouse release
    if (const auto *btnReleased = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (btnReleased->button == sf::Mouse::Button::Left) {
            isPainting = false;
            stopRecordingAndPush();
        }
        if (btnReleased->button == sf::Mouse::Button::Right) {
            isErasing = false;
            stopRecordingAndPush();
        }
    }

    // Mouse move (drag)
    if (const auto *mouseMoved = event.getIf<sf::Event::MouseMoved>()) {
        if (isPainting) {
            if (tx != lastPaintedTile.x || ty != lastPaintedTile.y) {
                // Re‑evaluate shift state each step
                bool shiftNow = sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LShift) ||
                                sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RShift);
                int tileToPaint = shiftNow ? -1 : selectedTile;
                setTileWithUndo(tx, ty, tileToPaint, true);
                lastPaintedTile = {tx, ty};
            }
        }
        else if (isErasing) {
            if (tx != lastErasedTile.x || ty != lastErasedTile.y) {
                int index = ty * Constants::WORLD_WIDTH_TILES + tx;
                if (!processedInDrag[index]) {
                    handleRightClickAction(tx, ty, false);   // revert only
                    processedInDrag[index] = true;
                }
                lastErasedTile = {tx, ty};
            }
        }
    }

    // Mouse wheel – change selected tile
    if (const auto *scroll = event.getIf<sf::Event::MouseWheelScrolled>()) {
        selectedTile += static_cast<int>(scroll->delta);
        selectedTile = std::clamp(selectedTile, 0, (int)paletteSprites.size() - 1);
        updateHighlightPosition();
    }
}

// ----------------------------------------------------------------------
// Drawing
// ----------------------------------------------------------------------
void LevelEditor::draw(sf::RenderWindow &window)
{
    if (!active) return;

    float s = Scale::get();
    sf::View originalView = window.getView();

    // ---- World grid and cursor ----
    if (!showPallete) {
        for (int x = 0; x <= Constants::WORLD_WIDTH_TILES; ++x) {
            sf::Vertex line[] = {
                {{x * Constants::TILE_SIZE * s, 0.f}, sf::Color(200, 200, 200, 100)},
                {{x * Constants::TILE_SIZE * s, Constants::WORLD_HEIGHT_PIXELS * s}, sf::Color(200, 200, 200, 100)}
            };
            window.draw(line, 2, sf::PrimitiveType::Lines);
        }
        for (int y = 0; y <= Constants::WORLD_HEIGHT_TILES; ++y) {
            sf::Vertex line[] = {
                {{0.f, y * Constants::TILE_SIZE * s}, sf::Color(200, 200, 200, 100)},
                {{Constants::WORLD_WIDTH_PIXELS * s, y * Constants::TILE_SIZE * s}, sf::Color(200, 200, 200, 100)}
            };
            window.draw(line, 2, sf::PrimitiveType::Lines);
        }
    }

    sf::Vector2f cursorPos(hoveredTile.x * Constants::TILE_SIZE * s,
                           Constants::WORLD_HEIGHT_PIXELS * s - (hoveredTile.y + 1) * Constants::TILE_SIZE * s);
    cursor.setPosition(cursorPos);
    cursor.setScale(Scale::getVec());
    window.draw(cursor);

    // ---- UI (default view) ----
    window.setView(window.getDefaultView());
    updatePaletteLayout(window.getSize());

    if (showPallete) {
        window.draw(paletteBackground);
        for (const auto &sprite : paletteSprites)
            window.draw(sprite);
        window.draw(selectionHighlight);
    }

    float minScale = std::min(Scale::getVec().x, Scale::getVec().y);
    tileInfo.setPosition(sf::Vector2f(10.f * minScale, 10.f * minScale));
    tileInfo.setString(
        "[f1: save], [esc: exit], [e: toggle grid], [u: undo], [r: redo]\n"
        "Selected Tile ID: " + std::to_string(selectedTile) +
        "\nHovered Tile: (" + std::to_string(hoveredTile.x) + ", " + std::to_string(hoveredTile.y) + ")"
    );
    window.draw(tileInfo);

    window.setView(originalView);
}

// ----------------------------------------------------------------------
// Activation
// ----------------------------------------------------------------------
void LevelEditor::setActive(bool a) { active = a; }
bool LevelEditor::isActive() const { return active; }
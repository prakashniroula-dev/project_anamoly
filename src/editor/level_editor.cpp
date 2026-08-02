// src/editor/LevelEditor.cpp
#include <core/constants.hpp>
#include <graphics/Tiles.hpp>
#include <editor/level_editor.hpp>
#include <core/scale.hpp>
#include <iostream>
#include <algorithm>
#include <SFML/Graphics.hpp>

void LevelEditor::paintTile(int tx, int ty)
{
    Terrain::setTile(tx, ty, selectedTile);
}

void LevelEditor::updateHighlightPosition()
{
    if (selectedTile >= 0 && selectedTile < (int)paletteSprites.size())
    {
        const sf::Sprite &spr = paletteSprites[selectedTile];
        selectionHighlight.setPosition(spr.getPosition());
    }
}

LevelEditor::LevelEditor()
    : font(),
      tileInfo(font),
      showGrid(true),
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
    // Character size and position will be set in draw()
}

void LevelEditor::initPalette()
{
    const int numTileTypes = 64 + 1;
    // We'll store base values and recompute layout later
    // So just create sprites with the correct texture rects and scales
    float originalWidth = Constants::TILE_SIZE;
    float originalHeight = Constants::TILE_SIZE;
    float scaleX = basePaletteTileSize / originalWidth;
    float scaleY = basePaletteTileSize / originalHeight;

    for (int i = 0; i < numTileTypes; ++i)
    {
        sf::Sprite sprite = Tiles::getTileSprite(i);
        // Scale sprite to base tile size (will be re‑scaled later in layout)
        sprite.setScale(sf::Vector2f(scaleX, scaleY));
        paletteSprites.push_back(sprite);
    }

    // Background and highlight will be sized in updatePaletteLayout()
    // Just create them with default size for now
    paletteBackground.setFillColor(sf::Color(40, 40, 40, 220));
    paletteBackground.setOutlineColor(sf::Color::White);
    paletteBackground.setOutlineThickness(1.f);

    selectionHighlight.setFillColor(sf::Color::Transparent);
    selectionHighlight.setOutlineColor(sf::Color::Yellow);
    selectionHighlight.setOutlineThickness(2.f);
}

void LevelEditor::updatePaletteLayout(const sf::Vector2u& windowSize)
{
    if (paletteSprites.empty()) return;

    // Get current scale vector (uniform or non‑uniform)
    sf::Vector2f scale = Scale::getVec();

    // Compute scaled values
    // For simplicity, we assume uniform scaling for tile aspect, but we can use both.
    // We'll set width and height to the same scaled size to keep square tiles.
    float tileWidth = basePaletteTileSize * scale.x;
    float tileHeight = basePaletteTileSize * scale.y;
    // But we want square tiles, so we can average or use both. Let's use tileSize = basePaletteTileSize * scale.x (if uniform).
    // If scale is non‑uniform, we might want to keep square tiles: use the minimum or just scale width and height.
    // We'll use tileSize = basePaletteTileSize * std::min(scale.x, scale.y) to keep square.
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
    float startY = 20.f * std::min(scale.x, scale.y); // scale top margin as well

    // Reposition sprites
    for (int i = 0; i < numTiles; ++i)
    {
        int row = i / columns;
        int col = i % columns;
        float x = startX + col * (tileSize + spacing);
        float y = startY + row * (tileSize + spacing);
        paletteSprites[i].setPosition(sf::Vector2f(x, y));
        // Scale the sprite to the current tile size (if not already)
        // We set scale in initPalette based on base size, so we need to adjust scale
        // to match current tileSize. We can set scale each frame:
        float spriteScaleX = tileSize / Constants::TILE_SIZE;
        float spriteScaleY = tileSize / Constants::TILE_SIZE;
        paletteSprites[i].setScale(sf::Vector2f(spriteScaleX, spriteScaleY));
    }

    // Reposition background
    float bgWidth = totalWidth + 20.f * std::min(scale.x, scale.y);
    float bgHeight = totalHeight + 20.f * std::min(scale.x, scale.y);
    paletteBackground.setSize(sf::Vector2f(bgWidth, bgHeight));
    paletteBackground.setPosition(sf::Vector2f(startX - 10.f * std::min(scale.x, scale.y),
                                               startY - 10.f * std::min(scale.x, scale.y)));

    // Update highlight size and position
    selectionHighlight.setSize(sf::Vector2f(tileSize, tileSize));
    updateHighlightPosition();

    // Scale info text character size
    int charSize = static_cast<int>(20 * std::min(scale.x, scale.y));
    tileInfo.setCharacterSize(charSize);
}

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

    // Keyboard toggle
    if (const auto *keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        if (keyPressed->scancode == sf::Keyboard::Scancode::E)
            showGrid = !showGrid;
    }

    if (const auto *btnPressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (btnPressed->button == sf::Mouse::Button::Left)
        {
            bool clickedPalette = false;
            if (showGrid)
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
            if (clickedPalette) return;

            isPainting = true;
            lastPaintedTile = {-1, -1};
            paintTile(tx, ty);
        }
        else if (btnPressed->button == sf::Mouse::Button::Right)
        {
            isErasing = true;
            lastErasedTile = {tx, ty};
            Terrain::eraseTile(tx, ty);
        }
    }

    if (const auto *btnReleased = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (btnReleased->button == sf::Mouse::Button::Left) isPainting = false;
        if (btnReleased->button == sf::Mouse::Button::Right) isErasing = false;
    }

    if (const auto *mouseMoved = event.getIf<sf::Event::MouseMoved>())
    {
        if (isPainting)
        {
            if (tx != lastPaintedTile.x || ty != lastPaintedTile.y)
            {
                paintTile(tx, ty);
                lastPaintedTile = {tx, ty};
            }
        }
        else if (isErasing)
        {
            if (tx != lastErasedTile.x || ty != lastErasedTile.y)
            {
                Terrain::eraseTile(tx, ty);
                lastErasedTile = {tx, ty};
            }
        }
    }

    if (const auto *scroll = event.getIf<sf::Event::MouseWheelScrolled>())
    {
        selectedTile += static_cast<int>(scroll->delta);
        selectedTile = std::clamp(selectedTile, 0, (int)paletteSprites.size() - 1);
        updateHighlightPosition();
    }
}

void LevelEditor::draw(sf::RenderWindow &window)
{
    if (!active) return;

    float s = Scale::get();

    // Save current view
    sf::View originalView = window.getView();

    // ---- World elements (grid, cursor) ----
    if (!showGrid)
    {
        for (int x = 0; x <= Constants::WORLD_WIDTH_TILES; ++x)
        {
            sf::Vertex line[] = {
                {{x * Constants::TILE_SIZE * s, 0.f}, sf::Color(200, 200, 200, 100)},
                {{x * Constants::TILE_SIZE * s, Constants::WORLD_HEIGHT_PIXELS * s}, sf::Color(200, 200, 200, 100)}
            };
            window.draw(line, 2, sf::PrimitiveType::Lines);
        }
        for (int y = 0; y <= Constants::WORLD_HEIGHT_TILES; ++y)
        {
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

    // ---- Switch to default view for UI ----
    window.setView(window.getDefaultView());

    // ---- Update UI layout with current scale and window size ----
    updatePaletteLayout(window.getSize());

    // ---- Draw UI (palette, highlight, info) ----
    if (showGrid)
    {
        window.draw(paletteBackground);
        for (const auto &sprite : paletteSprites)
            window.draw(sprite);
        window.draw(selectionHighlight);
    }

    tileInfo.setPosition(sf::Vector2f(10.f * std::min(Scale::getVec().x, Scale::getVec().y), 10.f * std::min(Scale::getVec().x, Scale::getVec().y)));
    tileInfo.setString("Tile: " + std::to_string(selectedTile) + "  (scroll)  [E: toggle grid/palette]");
    window.draw(tileInfo);

    // ---- Restore original view ----
    window.setView(originalView);
}

void LevelEditor::setActive(bool a) { active = a; }
bool LevelEditor::isActive() const { return active; }
#include "tile_editor.hpp"
#include <core/constants.hpp>
#include <graphics/tiles.hpp>
#include <core/scale.hpp>
#include <entities/terrain.hpp>
#include <algorithm>
#include <cmath>

void applyTileChange(TileChange& change, bool forward) {
    const auto& tiles = forward ? change.newTiles : change.oldTiles;
    Terrain::setTileVector(change.tx, change.ty, tiles);
}

TileEditor::TileEditor(const sf::Font& font)
    : palette(font),
      tileCursor(sf::Vector2f(Constants::TILE_SIZE, Constants::TILE_SIZE)),
      replaceText(font), stackText(font)
{
    tileCursor.setFillColor(sf::Color(255, 0, 0, 100));
    tileCursor.setOutlineColor(sf::Color::Red);
    tileCursor.setOutlineThickness(2.f);

    replaceBtn.setFillColor(sf::Color(60, 60, 60));
    replaceBtn.setOutlineColor(sf::Color::White);
    replaceBtn.setOutlineThickness(1.f);
    stackBtn = replaceBtn;

    replaceText.setString("Replace");
    replaceText.setFillColor(sf::Color::White);
    stackText.setString("Stack");
    stackText.setFillColor(sf::Color::White);
}

void TileEditor::init() {
    const int numTileTypes = Tiles::getCount();
    std::vector<sf::Sprite> sprites;
    sprites.reserve(numTileTypes);
    for (int i = 0; i < numTileTypes; ++i) {
        sprites.push_back(Tiles::getTileSprite(i));
    }
    palette.setSprites(sprites);
    palette.setLayout(10, 4, 32.f, 4.f);
    palette.setSelected(selectedTile);

    int totalTiles = Constants::WORLD_WIDTH_TILES * Constants::WORLD_HEIGHT_TILES;
    previousTiles.resize(totalTiles);
    processedInDrag.resize(totalTiles, false);
}

void TileEditor::setActive(bool a) { active = a; }
bool TileEditor::isActive() const { return active; }
void TileEditor::setPaletteVisible(bool visible) { showPalette = visible; }

void TileEditor::toggleStackMode() { stackMode = !stackMode; }
bool TileEditor::isStackMode() const { return stackMode; }
void TileEditor::togglePaletteVisibility() { showPalette = !showPalette; }

void TileEditor::updatePaletteLayout(const sf::Vector2u& windowSize) {
    palette.updateLayout(windowSize);
    selectedTile = palette.getSelected();
}

void TileEditor::handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    if (!active) return;

    // ★ Update palette layout NOW so sprite positions are current for hit-testing ★
    if (showPalette) {
        updatePaletteLayout(window.getSize());
    }

    sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
    sf::Vector2f mouseWorld = window.mapPixelToCoords(mousePixel);
    mouseWorldPos = mouseWorld;

    float s = Scale::get();
    int tx = static_cast<int>(std::floor(mouseWorld.x / (Constants::TILE_SIZE * s)));
    int ty = static_cast<int>(std::floor((Constants::WORLD_HEIGHT_PIXELS * s - mouseWorld.y) / (Constants::TILE_SIZE * s)));
    tx = std::clamp(tx, 0, Constants::WORLD_WIDTH_TILES - 1);
    ty = std::clamp(ty, 0, Constants::WORLD_HEIGHT_TILES - 1);
    hoveredTile = {tx, ty};

    // Palette interaction
    if (showPalette) {
        if (const auto* btn = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (btn->button == sf::Mouse::Button::Left) {
                sf::Vector2f mouseDefault = window.mapPixelToCoords(mousePixel, window.getDefaultView());
                // Mode buttons
                if (replaceBtn.getGlobalBounds().contains(mouseDefault)) {
                    stackMode = false;
                    return;
                }
                if (stackBtn.getGlobalBounds().contains(mouseDefault)) {
                    stackMode = true;
                    return;
                }
                if (palette.handleMousePress(mouseDefault)) {
                    selectedTile = palette.getSelected();
                    return;
                }
            }
        }
        if (const auto* scroll = event.getIf<sf::Event::MouseWheelScrolled>()) {
            if (palette.handleMouseScroll(scroll->delta)) {
                selectedTile = palette.getSelected();
                // no return – allow painting after scroll
            }
        }
    }

    bool shiftHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LShift) ||
                     sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RShift);

    if (const auto* btn = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (btn->button == sf::Mouse::Button::Left) {
            startRecording();
            isPainting = true;
            lastPaintedTile = {-1, -1};
            int tileToPaint = shiftHeld ? -1 : selectedTile;
            setTileWithUndo(tx, ty, tileToPaint, true);
        } else if (btn->button == sf::Mouse::Button::Right) {
            startRecording();
            isErasing = true;
            lastErasedTile = {-1, -1};
            std::fill(processedInDrag.begin(), processedInDrag.end(), false);
            handleRightClickTile(tx, ty, false);
        }
    }

    if (const auto* rel = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (rel->button == sf::Mouse::Button::Left && isPainting) {
            isPainting = false;
            stopRecording();
        }
        if (rel->button == sf::Mouse::Button::Right && isErasing) {
            isErasing = false;
            stopRecording();
        }
    }

    if (const auto* move = event.getIf<sf::Event::MouseMoved>()) {
        if (isPainting) {
            if (tx != lastPaintedTile.x || ty != lastPaintedTile.y) {
                bool shiftNow = sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LShift) ||
                                sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RShift);
                int tileToPaint = shiftNow ? -1 : selectedTile;
                setTileWithUndo(tx, ty, tileToPaint, true);
                lastPaintedTile = {tx, ty};
            }
        } else if (isErasing) {
            if (tx != lastErasedTile.x || ty != lastErasedTile.y) {
                int idx = ty * Constants::WORLD_WIDTH_TILES + tx;
                if (!processedInDrag[idx]) {
                    handleRightClickTile(tx, ty, false);
                    processedInDrag[idx] = true;
                }
                lastErasedTile = {tx, ty};
            }
        }
    }

    // Fallback scroll for tile selection when palette not handling it
    if (const auto* scroll = event.getIf<sf::Event::MouseWheelScrolled>()) {
        if (!showPalette || !palette.isVisible()) {
            int total = Tiles::getCount();
            selectedTile = std::clamp(selectedTile + static_cast<int>(scroll->delta), 0, total - 1);
            palette.setSelected(selectedTile);
        }
    }
}

void TileEditor::draw(sf::RenderWindow& window) {
    if (!active) return;

    float s = Scale::get();

    // Grid
    if (!showPalette) {
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

    // Ghost / cursor
    sf::Vector2f cursorPos(
        hoveredTile.x * Constants::TILE_SIZE * s,
        Constants::WORLD_HEIGHT_PIXELS * s - (hoveredTile.y + 1) * Constants::TILE_SIZE * s
    );
    bool shiftHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LShift) ||
                     sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RShift);

    if (selectedTile >= 0 && !shiftHeld) {
        sf::Sprite ghost = Tiles::getTileSprite(selectedTile);
        ghost.setPosition(cursorPos);
        ghost.setScale(Scale::getVec());
        ghost.setColor(sf::Color(255, 255, 255, 150));
        window.draw(ghost);
    } else {
        tileCursor.setPosition(cursorPos);
        tileCursor.setScale(Scale::getVec());
        window.draw(tileCursor);
    }

    // UI
    sf::View defaultView = window.getDefaultView();
    window.setView(defaultView);

    if (showPalette) {
        // Layout already updated in handleEvent, but update again for drawing
        updatePaletteLayout(window.getSize());
        palette.draw(window);

        // Mode buttons: place to the right of palette's next button
        sf::FloatRect nextBounds = palette.getNextBtnBounds();
        sf::Vector2f scale = Scale::getVec();
        float modeBtnSize = 40.f * std::min(scale.x, scale.y);
        float modeSpacing = 10.f * std::min(scale.x, scale.y);
        float modeX = nextBounds.position.x + nextBounds.size.x + modeSpacing;
        float modeY = nextBounds.position.y;

        replaceBtn.setSize({modeBtnSize, modeBtnSize});
        stackBtn.setSize({modeBtnSize, modeBtnSize});
        replaceBtn.setPosition({modeX, modeY});
        stackBtn.setPosition({modeX + modeBtnSize + modeSpacing, modeY});

        replaceBtn.setFillColor(stackMode ? sf::Color(60,60,60) : sf::Color(100,100,200));
        stackBtn.setFillColor(stackMode ? sf::Color(100,100,200) : sf::Color(60,60,60));

        replaceText.setCharacterSize(static_cast<unsigned>(modeBtnSize * 0.5f));
        stackText.setCharacterSize(static_cast<unsigned>(modeBtnSize * 0.5f));
        auto centerText = [&](sf::Text& text, const sf::RectangleShape& btn) {
            sf::FloatRect b = text.getLocalBounds();
            text.setPosition({
                btn.getPosition().x + (btn.getSize().x - b.size.x) / 2.f,
                btn.getPosition().y + (btn.getSize().y - b.size.y) / 2.f - 2.f
            });
        };
        centerText(replaceText, replaceBtn);
        centerText(stackText, stackBtn);

        window.draw(replaceBtn);
        window.draw(stackBtn);
        window.draw(replaceText);
        window.draw(stackText);
    }

    window.setView(defaultView);
}

void TileEditor::setTileWithUndo(int tx, int ty, int newTile, bool isPaint) {
    std::vector<int> oldTiles = Terrain::getTile(tx, ty);
    std::vector<int> newTiles;
    if (stackMode) {
        newTiles = oldTiles;
        if (newTile == -1) {
            if (!newTiles.empty()) newTiles.pop_back();
        } else {
            newTiles.push_back(newTile);
        }
    } else {
        if (newTile == -1) newTiles.clear();
        else newTiles = {newTile};
    }
    if (oldTiles == newTiles) return;

    TileChange change{tx, ty, oldTiles, newTiles, isPaint};
    undoStack.addChange(change);

    if (isPaint) {
        int idx = ty * Constants::WORLD_WIDTH_TILES + tx;
        previousTiles[idx] = oldTiles;
    }
    Terrain::setTileVector(tx, ty, newTiles);
}

void TileEditor::handleRightClickTile(int tx, int ty, bool shiftHeld) {
    int idx = ty * Constants::WORLD_WIDTH_TILES + tx;
    if (shiftHeld) {
        std::vector<int> empty;
        setTileWithUndo(tx, ty, -1, false);
        previousTiles[idx].clear();
        return;
    }
    if (!previousTiles[idx].empty()) {
        std::vector<int> current = Terrain::getTile(tx, ty);
        TileChange change{tx, ty, current, previousTiles[idx], false};
        undoStack.addChange(change);
        Terrain::setTileVector(tx, ty, previousTiles[idx]);
        previousTiles[idx].clear();
    }
}

void TileEditor::undo() { undoStack.undo(); }
void TileEditor::redo() { undoStack.redo(); }
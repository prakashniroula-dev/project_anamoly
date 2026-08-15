#include "solid_editor.hpp"
#include <map/terrain.hpp>
#include <core/constants.hpp>
#include <core/scale.hpp>
#include <graphics/tiles.hpp>
#include <algorithm>
#include <cmath>

void applySolidChange(SolidChange& change, bool forward) {
    const int& type = forward ? change.newType : change.oldType;
    Terrain::setSolidTile(change.tx, change.ty, type);
}

SolidEditor::SolidEditor(const sf::Font& font)
    : solidCursor(sf::Vector2f(Constants::TILE_SIZE, Constants::TILE_SIZE))
{
    solidCursor.setFillColor(sf::Color(0, 255, 0, 80));
    solidCursor.setOutlineColor(sf::Color::Green);
    solidCursor.setOutlineThickness(2.f);
}

void SolidEditor::init() {
    int totalTiles = Constants::WORLD_WIDTH_TILES * Constants::WORLD_HEIGHT_TILES;
    processedInDrag.resize(totalTiles, false);
}

void SolidEditor::setActive(bool a) { active = a; }
bool SolidEditor::isActive() const { return active; }

void SolidEditor::handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    if (!active) return;

    sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
    sf::Vector2f mouseWorld = window.mapPixelToCoords(mousePixel);

    float s = Scale::get();
    int tx = static_cast<int>(std::floor(mouseWorld.x / (Constants::TILE_SIZE * s)));
    int ty = static_cast<int>(std::floor((Constants::WORLD_HEIGHT_PIXELS * s - mouseWorld.y) / (Constants::TILE_SIZE * s)));
    tx = std::clamp(tx, 0, Constants::WORLD_WIDTH_TILES - 1);
    ty = std::clamp(ty, 0, Constants::WORLD_HEIGHT_TILES - 1);
    hoveredTile = {tx, ty};

    // ---------- Mouse Press ----------
    if (const auto* btn = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (btn->button == sf::Mouse::Button::Left) {
            startRecording();
            isPainting = true;
            lastPaintedTile = {-1, -1};
            setSolidWithUndo(tx, ty, 1); // paint solid
        } else if (btn->button == sf::Mouse::Button::Right) {
            startRecording();
            isErasing = true;
            lastErasedTile = {-1, -1};
            std::fill(processedInDrag.begin(), processedInDrag.end(), false);
            setSolidWithUndo(tx, ty, 0); // erase
        }
    }

    // ---------- Mouse Release ----------
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

    // ---------- Mouse Move (drag) ----------
    if (const auto* move = event.getIf<sf::Event::MouseMoved>()) {
        if (isPainting) {
            if (tx != lastPaintedTile.x || ty != lastPaintedTile.y) {
                setSolidWithUndo(tx, ty, 1);
                lastPaintedTile = {tx, ty};
            }
        } else if (isErasing) {
            if (tx != lastErasedTile.x || ty != lastErasedTile.y) {
                int idx = ty * Constants::WORLD_WIDTH_TILES + tx;
                if (!processedInDrag[idx]) {
                    setSolidWithUndo(tx, ty, 0);
                    processedInDrag[idx] = true;
                }
                lastErasedTile = {tx, ty};
            }
        }
    }
}

void SolidEditor::draw(sf::RenderWindow& window) {
    if (!active) return;

    float s = Scale::get();

    // Draw existing solid tiles as an overlay
    const auto& solidMap = Terrain::getSolidMap();
    for (const auto& [pos, type] : solidMap) {
        if (type <= 0) continue;
        sf::RectangleShape rect(sf::Vector2f(Constants::TILE_SIZE * s, Constants::TILE_SIZE * s));
        rect.setPosition(Tiles::getTilePosition(pos.first, pos.second));
        rect.setFillColor(sf::Color(0, 255, 0, 50));
        rect.setOutlineColor(sf::Color::Green);
        rect.setOutlineThickness(1.f);
        window.draw(rect);
    }

    // Cursor
    sf::Vector2f cursorPos = Tiles::getTilePosition(hoveredTile.x, hoveredTile.y);
    solidCursor.setPosition(cursorPos);
    solidCursor.setScale(Scale::getVec());
    window.draw(solidCursor);
}

void SolidEditor::setSolidWithUndo(int tx, int ty, int newType) {
    int oldType = Terrain::getSolidTile(tx, ty);
    if (oldType == newType) return;
    SolidChange change{tx, ty, oldType, newType};
    undoStack.addChange(change);
    Terrain::setSolidTile(tx, ty, newType);
}

void SolidEditor::undo() { undoStack.undo(); }
void SolidEditor::redo() { undoStack.redo(); }
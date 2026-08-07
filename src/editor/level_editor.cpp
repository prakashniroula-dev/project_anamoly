#include "level_editor.hpp"
#include <core/constants.hpp>
#include <iostream>

LevelEditor::LevelEditor()
    : tileEditor(font), objectEditor(font)
{
    if (!font.openFromFile("assets/fonts/orbitron.ttf")) {
        // fallback
    }
    tileEditor.setActive(true);
    objectEditor.setActive(false);
    tileEditor.setPaletteVisible(true);
    objectEditor.setPaletteVisible(false);
}

void LevelEditor::init() {
    tileEditor.init();
    objectEditor.init();
}

void LevelEditor::setActive(bool a) {
    active = a;
    if (active) {
        tileEditor.setActive(currentMode == Mode::Tile);
        objectEditor.setActive(currentMode == Mode::Object);
    } else {
        tileEditor.setActive(false);
        objectEditor.setActive(false);
    }
}

bool LevelEditor::isActive() const { return active; }

void LevelEditor::switchMode(Mode mode) {
    if (mode == currentMode) return;
    currentMode = mode;
    tileEditor.setActive(mode == Mode::Tile);
    objectEditor.setActive(mode == Mode::Object);
    tileEditor.setPaletteVisible(mode == Mode::Tile);
    objectEditor.setPaletteVisible(mode == Mode::Object);
}

void LevelEditor::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (!active) return;

    // ★ Force palette layout update before any event handling ★
    // This ensures mouse click hit‑testing works correctly.
    if (currentMode == Mode::Tile) {
        tileEditor.updatePaletteLayout(window.getSize());
    } else {
        objectEditor.updatePaletteLayout(window.getSize());
    }

    // Global shortcuts
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        switch (key->scancode) {
            case sf::Keyboard::Scancode::F1:
                switchMode(Mode::Tile);
                break;
            case sf::Keyboard::Scancode::F2:
                switchMode(Mode::Object);
                break;
            case sf::Keyboard::Scancode::E:
                if (currentMode == Mode::Tile) tileEditor.togglePaletteVisibility();
                break;
            case sf::Keyboard::Scancode::O:
                if (currentMode == Mode::Object) objectEditor.togglePaletteVisibility();
                break;
            case sf::Keyboard::Scancode::U:
                if (currentMode == Mode::Tile) tileEditor.undo();
                else objectEditor.undo();
                break;
            case sf::Keyboard::Scancode::R:
                if (currentMode == Mode::Tile) tileEditor.redo();
                else objectEditor.redo();
                break;
            case sf::Keyboard::Scancode::M:
                if (currentMode == Mode::Tile) tileEditor.toggleStackMode();
                break;
            default: break;
        }
    }

    // Forward event to the active editor
    if (currentMode == Mode::Tile)
        tileEditor.handleEvent(event, window);
    else
        objectEditor.handleEvent(event, window);
}

void LevelEditor::draw(sf::RenderWindow& window) {
    if (!active) return;
    if (currentMode == Mode::Tile)
        tileEditor.draw(window);
    else
        objectEditor.draw(window);
}
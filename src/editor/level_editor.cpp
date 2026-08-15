#include "level_editor.hpp"
#include <core/constants.hpp>
#include <iostream>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/Clipboard.hpp>
#include <ui/ui_manager.hpp>

LevelEditor::LevelEditor()
    : tileEditor(font), objectEditor(font), solidEditor(font), spawnEditor(font), waypointEditor(font), m_positionText(font)
{
    if (!font.openFromFile("assets/fonts/orbitron.ttf"))
    {
        // fallback
    }
    tileEditor.setActive(true);
    objectEditor.setActive(false);
    tileEditor.setPaletteVisible(true);
    objectEditor.setPaletteVisible(false);
    spawnEditor.setActive(false);
    spawnEditor.setPaletteVisible(false);
    waypointEditor.setActive(false);

    // ---- Init position display ----
    m_positionText.setFont(font);
    m_positionText.setCharacterSize(16);
    m_positionText.setFillColor(sf::Color::White);
    m_positionText.setString("X: 0.0  Y: 0.0");

    m_positionBg.setFillColor(sf::Color(0, 0, 0, 180));
    m_positionBg.setOutlineColor(sf::Color::White);
    m_positionBg.setOutlineThickness(1.f);
    m_lastMouseWorld = sf::Vector2f(0.f, 0.f);
}

void LevelEditor::init()
{
    tileEditor.init();
    objectEditor.init();
    solidEditor.init();
    spawnEditor.init();
    waypointEditor.init();
}

void LevelEditor::setActive(bool a)
{
    active = a;
    if (active)
    {
        tileEditor.setActive(currentMode == Mode::Tile);
        objectEditor.setActive(currentMode == Mode::Object);
        solidEditor.setActive(currentMode == Mode::Solid);
        spawnEditor.setActive(currentMode == Mode::Spawn);
        waypointEditor.setActive(currentMode == Mode::Waypoint);
    }
    else
    {
        tileEditor.setActive(false);
        objectEditor.setActive(false);
        solidEditor.setActive(false);
        spawnEditor.setActive(false);
        waypointEditor.setActive(false);
    }
}

bool LevelEditor::isActive() const { return active; }

void LevelEditor::switchMode(Mode mode)
{
    if (mode == currentMode)
        return;
    currentMode = mode;
    tileEditor.setActive(mode == Mode::Tile);
    objectEditor.setActive(mode == Mode::Object);
    solidEditor.setActive(mode == Mode::Solid);
    spawnEditor.setActive(mode == Mode::Spawn);
    tileEditor.setPaletteVisible(mode == Mode::Tile);
    objectEditor.setPaletteVisible(mode == Mode::Object);
    spawnEditor.setPaletteVisible(mode == Mode::Spawn);
    waypointEditor.setActive(mode == Mode::Waypoint);
}

void LevelEditor::handleEvent(const sf::Event &event, sf::RenderWindow &window)
{
    if (!active)
        return;

    // ★ Force palette layout update before any event handling ★
    // This ensures mouse click hit‑testing works correctly.
    if (currentMode == Mode::Tile)
    {
        tileEditor.updatePaletteLayout(window);
    }
    else if (currentMode == Mode::Object)
    {
        objectEditor.updatePaletteLayout(window);
    } else if (currentMode == Mode::Spawn) {
        spawnEditor.updatePaletteLayout(window);
    }

    // Global shortcuts
    if (const auto *key = event.getIf<sf::Event::KeyPressed>())
    {
        bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LControl) ||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RControl);
        bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LShift) ||
                     sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RShift);
        switch (key->scancode)
        {
        case sf::Keyboard::Scancode::F1:
        case sf::Keyboard::Scancode::Num1:
            switchMode(Mode::Tile);
            break;
        case sf::Keyboard::Scancode::F2:
        case sf::Keyboard::Scancode::Num2:
            switchMode(Mode::Object);
            break;
        case sf::Keyboard::Scancode::F3:
        case sf::Keyboard::Scancode::Num3:
            switchMode(Mode::Solid);
            break;
        case sf::Keyboard::Scancode::F4:
        case sf::Keyboard::Scancode::Num4:
            switchMode(Mode::Spawn);
            break;
        case sf::Keyboard::Scancode::F5:
        case sf::Keyboard::Scancode::Num5:
            switchMode(Mode::Waypoint);
            break;
        case sf::Keyboard::Scancode::E:
            if (currentMode == Mode::Tile)
                tileEditor.togglePaletteVisibility();
            break;
        case sf::Keyboard::Scancode::O:
            if (currentMode == Mode::Object)
                objectEditor.togglePaletteVisibility();
            break;
        case sf::Keyboard::Scancode::M:
            if (currentMode == Mode::Tile)
                tileEditor.toggleStackMode();
            break;
        case sf::Keyboard::Scancode::C:
            {
                // Copy coordinates to clipboard
                std::ostringstream oss;
                oss.precision(1);
                oss << std::fixed << m_lastMouseWorld.x << ", " << m_lastMouseWorld.y;
                sf::Clipboard::setString(oss.str());
                // Optionally log or show feedback (we can update the position text temporarily)
                std::string msg = "Copied: " + oss.str();
                m_positionText.setString(msg);
                // We'll let the normal draw update it later, so it's fine.
                break;
            }
        default:
            break;
        }

        if (ctrl && key->scancode == sf::Keyboard::Scancode::Z)
        {
            if (currentMode == Mode::Tile)
                tileEditor.undo();
            else if (currentMode == Mode::Object)
                objectEditor.undo();
            else if (currentMode == Mode::Solid)
                solidEditor.undo();
            else if (currentMode == Mode::Spawn)
                spawnEditor.undo();
            else if (currentMode == Mode::Waypoint)
                waypointEditor.undo();
        }
        // Redo: Ctrl+Y
        if (ctrl && key->scancode == sf::Keyboard::Scancode::Y)
        {
            if (currentMode == Mode::Tile)
                tileEditor.redo();
            else if (currentMode == Mode::Object)
                objectEditor.redo();
            else if (currentMode == Mode::Solid)
                solidEditor.redo();
            else if (currentMode == Mode::Spawn)
                spawnEditor.redo();
            else if (currentMode == Mode::Waypoint)
                waypointEditor.redo();
        }
    }

    // Forward event to the active editor
    if (currentMode == Mode::Tile)
        tileEditor.handleEvent(event, window);
    else if (currentMode == Mode::Solid)
        solidEditor.handleEvent(event, window);
    else if (currentMode == Mode::Spawn)
        spawnEditor.handleEvent(event, window);
    else if (currentMode == Mode::Waypoint)
        waypointEditor.handleEvent(event, window);
    else
        objectEditor.handleEvent(event, window);
}

void LevelEditor::draw(sf::RenderWindow &window)
{
    if (!active) return;

    // 1. Save the current (world) view before any editor changes it
    sf::View worldView = window.getView();

    // 2. Draw the active editor (may change view to default)
    if (currentMode == Mode::Tile)
        tileEditor.draw(window);
    else if (currentMode == Mode::Solid)
        solidEditor.draw(window);
    else if (currentMode == Mode::Spawn)
        spawnEditor.draw(window);
    else if (currentMode == Mode::Waypoint)
        waypointEditor.draw(window);
    else
        objectEditor.draw(window);

    
    window.setView(worldView); // before computing world coordinates
    // ---- Draw world coordinates (bottom right) ----
    sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
    // Use the saved worldView to get world coordinates
    sf::Vector2f mouseWorld = window.mapPixelToCoords(mousePixel, worldView);
    m_lastMouseWorld = mouseWorld / Scale::get();
    
    // Update text
    std::ostringstream oss;
    oss.precision(1);
    oss << std::fixed << "X: " << m_lastMouseWorld.x << "  Y: " << m_lastMouseWorld.y;
    m_positionText.setString(oss.str());

    // Compute size and position (bottom right, 10px padding)
    sf::FloatRect textBounds = m_positionText.getLocalBounds();
    float padding = 10.f;
    float bgWidth = textBounds.size.x + padding * 2.f;
    float bgHeight = textBounds.size.y + padding * 2.f;
    float bgX = window.getSize().x - bgWidth - padding;
    float bgY = window.getSize().y - bgHeight - padding;

    m_positionBg.setSize({bgWidth, bgHeight});
    m_positionBg.setPosition({bgX, bgY});

    // Center text inside the background
    m_positionText.setPosition({
        bgX + padding,
        bgY + padding
    });

    // before drawing
    window.setView(UIManager::get().getUIView(window)); // Restore the UI view

    // Draw
    window.draw(m_positionBg);
    window.draw(m_positionText);
}
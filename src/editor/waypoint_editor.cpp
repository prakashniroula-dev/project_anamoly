#include "waypoint_editor.hpp"
#include <core/scale.hpp>
#include <map/terrain.hpp>
#include <debug/logs.hpp>
#include <algorithm>
#include <cmath>

WaypointEditor::WaypointEditor(const sf::Font& font)
    : font(font)
    , selectionHighlight(sf::Vector2f(64.f, 64.f))
    , waypointDot(6.f)
    , infoText(font)
{
    selectionHighlight.setFillColor(sf::Color::Transparent);
    selectionHighlight.setOutlineColor(sf::Color::Yellow);
    selectionHighlight.setOutlineThickness(3.f);

    waypointDot.setFillColor(sf::Color::Cyan);
    waypointDot.setOutlineColor(sf::Color::White);
    waypointDot.setOutlineThickness(1.f);

    infoBackground.setFillColor(sf::Color(0, 0, 0, 200));
    infoBackground.setOutlineColor(sf::Color::White);
    infoBackground.setOutlineThickness(1.f);

    infoText.setCharacterSize(14);
    infoText.setFillColor(sf::Color::White);
    infoText.setOutlineColor(sf::Color::Black);
    infoText.setOutlineThickness(1.f);
}

void WaypointEditor::init() {
    // Nothing needed
}

void WaypointEditor::setActive(bool a) {
    active = a;
    if (!active) {
        m_hasSelection = false;
        m_selectedProps = nullptr;
    }
}

bool WaypointEditor::isActive() const { return active; }

void WaypointEditor::handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    if (!active) return;

    sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
    mouseWorldPos = window.mapPixelToCoords(mousePixel);

    bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LControl) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RControl);

    if (const auto* btn = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (btn->button == sf::Mouse::Button::Left) {
            if (ctrl) {
                // Ctrl+click: select spawn at mouse position
                selectSpawnAt(mouseWorldPos);
            } else if (m_hasSelection && m_selectedProps) {
                // Add waypoint at mouse position (world coords, unscaled)
                sf::Vector2f unscaled = mouseWorldPos / Scale::get();
                addWaypoint(unscaled);
            }
        } else if (btn->button == sf::Mouse::Button::Right) {
            if (m_hasSelection && m_selectedProps) {
                // Remove waypoint near mouse position
                sf::Vector2f unscaled = mouseWorldPos / Scale::get();
                removeWaypointAt(unscaled);
            }
        }
    }

    // Press Enter to save waypoints (they are already saved in the map, but we can flush to file)
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Enter && m_hasSelection && m_selectedProps) {
            Log::info << "Waypoints updated for spawn at (" << m_selectedKey.first << ", " << m_selectedKey.second << ")" << std::endl;
        }
        // Press Escape to deselect
        if (key->code == sf::Keyboard::Key::Escape) {
            m_hasSelection = false;
            m_selectedProps = nullptr;
        }
    }
}

void WaypointEditor::draw(sf::RenderWindow& window) {
    if (!active) return;

    const auto& spawnMap = Terrain::getSpawnMap();

    // First, draw all spawns and their waypoints
    for (const auto& [pos, props] : spawnMap) {
        // Draw spawn sprite (top-left origin, like character system)
        sf::Sprite spr = Characters::getCharacterSprite(props.characterKey);
        spr.setOrigin({0.f, 0.f});
        sf::Vector2f worldPos(pos.first * Scale::get(), pos.second * Scale::get());
        spr.setPosition(worldPos);
        spr.setScale(Scale::getVec() * props.scale);
        window.draw(spr);

        // Draw waypoints for this spawn
        drawWaypoints(window, props, worldPos);

        // Highlight selected spawn
        if (m_hasSelection && m_selectedKey == pos) {
            selectionHighlight.setSize({64.f * Scale::get(), 64.f * Scale::get()});
            selectionHighlight.setPosition(worldPos);
            window.draw(selectionHighlight);
        }
    }

    // Draw info panel if a spawn is selected
    if (m_hasSelection && m_selectedProps) {
        updateInfoText();

        // Position the panel in the default view (top-left corner)
        sf::View defaultView = window.getDefaultView();
        window.setView(defaultView);

        sf::FloatRect textBounds = infoText.getLocalBounds();
        float padding = 10.f;
        float panelWidth = textBounds.size.x + padding * 2;
        float panelHeight = textBounds.size.y + padding * 2;
        infoBackground.setSize({panelWidth, panelHeight});
        infoBackground.setPosition({10.f, 10.f});

        infoText.setPosition({10.f + padding, 10.f + padding});

        window.draw(infoBackground);
        window.draw(infoText);

        window.setView(defaultView); // restore world view
    }
}

void WaypointEditor::setSpawnWithUndo(const std::pair<float,float>& key, const SpawnProps& newProps) {
    SpawnProps oldProps = Terrain::getSpawn(key.first, key.second);
    if (oldProps.waypoints == newProps.waypoints &&
        oldProps.characterKey == newProps.characterKey &&
        oldProps.npcTypeId == newProps.npcTypeId &&
      oldProps.scale == newProps.scale &&
        oldProps.rotation == newProps.rotation &&
        oldProps.flipX == newProps.flipX &&
        oldProps.flipY == newProps.flipY) {
        return; // no change
    }
    undoStack.addChange({key.first, key.second, oldProps, newProps});
    Terrain::setSpawn(key.first, key.second, newProps);
}

void WaypointEditor::drawWaypoints(sf::RenderWindow& window, const SpawnProps& props, const sf::Vector2f& worldPos) {
    if (props.waypoints.empty()) return;

    // Draw lines between waypoints
    for (size_t i = 1; i < props.waypoints.size(); ++i) {
        sf::Vector2f from = props.waypoints[i-1] * Scale::get();
        sf::Vector2f to = props.waypoints[i] * Scale::get();
        sf::Vertex line[] = {
            {from, sf::Color::Yellow},
            {to, sf::Color::Yellow}
        };
        window.draw(line, 2, sf::PrimitiveType::Lines);
    }

    // Draw dots for each waypoint
    for (const auto& wp : props.waypoints) {
        sf::Vector2f dotPos = wp * Scale::get();
        waypointDot.setPosition(dotPos - sf::Vector2f(waypointDot.getRadius(), waypointDot.getRadius()));
        window.draw(waypointDot);
    }
}

void WaypointEditor::selectSpawnAt(const sf::Vector2f& worldPos) {
    const auto& spawnMap = Terrain::getSpawnMap();
    for (const auto& [pos, props] : spawnMap) {
        sf::Sprite spr = Characters::getCharacterSprite(props.characterKey);
        spr.setOrigin({0.f, 0.f});
        spr.setPosition({pos.first * Scale::get(), pos.second * Scale::get()});
        spr.setScale(Scale::getVec() * props.scale);
        if (spr.getGlobalBounds().contains(worldPos)) {
            m_selectedKey = pos;
            m_selectedProps = const_cast<SpawnProps*>(&props);
            m_hasSelection = true;
            updateInfoText();
            return;
        }
    }
    // Deselect if nothing found
    m_hasSelection = false;
    m_selectedProps = nullptr;
}

void WaypointEditor::addWaypoint(const sf::Vector2f& worldPos) {
    if (!m_hasSelection || !m_selectedProps) return;
    startRecording();
    SpawnProps newProps = *m_selectedProps;
    newProps.waypoints.push_back(worldPos);
    setSpawnWithUndo(m_selectedKey, newProps);
    stopRecording();
    // Update pointer to the new props (Terrain stores a copy, so we need to re-fetch)
    auto it = Terrain::getSpawnMap().find(m_selectedKey);
    if (it != Terrain::getSpawnMap().end())
        m_selectedProps = const_cast<SpawnProps*>(&it->second);
    updateInfoText();
}

void WaypointEditor::removeWaypointAt(const sf::Vector2f& worldPos) {
    if (!m_hasSelection || !m_selectedProps) return;
    const float threshold = 5.f; // in unscaled units
    auto& wps = m_selectedProps->waypoints;
    for (size_t i = 0; i < wps.size(); ++i) {
        if (std::hypot(wps[i].x - worldPos.x, wps[i].y - worldPos.y) < threshold) {
            startRecording();
            SpawnProps newProps = *m_selectedProps;
            newProps.waypoints.erase(newProps.waypoints.begin() + i);
            setSpawnWithUndo(m_selectedKey, newProps);
            stopRecording();
            // Re-fetch pointer
            auto it = Terrain::getSpawnMap().find(m_selectedKey);
            if (it != Terrain::getSpawnMap().end())
                m_selectedProps = const_cast<SpawnProps*>(&it->second);
            updateInfoText();
            break;
        }
    }
}

void WaypointEditor::updateInfoText() {
    if (!m_hasSelection || !m_selectedProps) {
        infoText.setString("");
        return;
    }
    std::string text = "Selected NPC:\n";
    text += "  npcTypeId: " + m_selectedProps->npcTypeId + "\n";
    text += "  uniqueID: " + m_selectedProps->uniqueID + "\n";
    text += "  scriptName: " + m_selectedProps->scriptName + "\n";
    text += "  Waypoints: " + std::to_string(m_selectedProps->waypoints.size()) + "\n";
    for (size_t i = 0; i < m_selectedProps->waypoints.size(); ++i) {
        const auto& wp = m_selectedProps->waypoints[i];
        text += "    " + std::to_string(i+1) + ": (" + std::to_string(wp.x) + ", " + std::to_string(wp.y) + ")\n";
    }
    infoText.setString(text);
}
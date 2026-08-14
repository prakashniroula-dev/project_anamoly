#include "spawn_editor.hpp"
#include <core/scale.hpp>
#include <core/constants.hpp>
#include <debug/logs.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>

// -----------------------------------------------------------------------------
void applySpawnChange(SpawnChange& change, bool forward) {
    const auto& props = forward ? change.newProps : change.oldProps;
    if (props.characterKey.empty()) {
        Terrain::eraseSpawn(change.x, change.y);
    } else {
        Terrain::setSpawn(change.x, change.y, props);
    }
}

// -----------------------------------------------------------------------------
SpawnEditor::SpawnEditor(const sf::Font& font)
    : palette(font)
    , cursorSprite(dummyTexture)
    , debugText(font)
    , font(font)
    , m_propTitle(font)
    , m_propNpcType(font)
    , m_propUniqueID(font)
    , m_propScript(font)
    , m_propHelp(font)
{
    debugText.setCharacterSize(14);
    debugText.setFillColor(sf::Color::White);
    debugText.setOutlineColor(sf::Color::Black);
    debugText.setOutlineThickness(1.f);

    // Property panel styling
    m_propPanel.setFillColor(sf::Color(30, 30, 30, 230));
    m_propPanel.setOutlineColor(sf::Color::White);
    m_propPanel.setOutlineThickness(1.f);

    m_propTitle.setString("Edit Spawn Properties");
    m_propTitle.setCharacterSize(18);
    m_propTitle.setFillColor(sf::Color::Yellow);
    m_propTitle.setStyle(sf::Text::Bold);

    m_propNpcType.setCharacterSize(16);
    m_propNpcType.setFillColor(sf::Color::White);
    m_propUniqueID.setCharacterSize(16);
    m_propUniqueID.setFillColor(sf::Color::White);
    m_propScript.setCharacterSize(16);
    m_propScript.setFillColor(sf::Color::White);

    m_propHelp.setCharacterSize(12);
    m_propHelp.setFillColor(sf::Color(200, 200, 200));
    m_propHelp.setString("Tab to switch fields, Enter to save, Escape to cancel");
}

// -----------------------------------------------------------------------------
void SpawnEditor::init() {
    undoStack.reset();

    // Build palette entries
    spawnKeys.clear();
    spawnKeys.push_back("player"); // special entry for player spawn

    std::vector<std::string> typeIds = NPCManager::get().getTypeIds();
    for (const auto& id : typeIds) {
        spawnKeys.push_back(id);
    }

    // Build palette sprites (each sprite is centered)
    std::vector<sf::Sprite> sprites;
    for (const auto& key : spawnKeys) {
        sf::Sprite spr(Characters::getCharacterSprite(Characters::Player));
        if (key != "player") {
            const NPCType* type = NPCManager::get().getType(key);
            if (type) {
                spr = Characters::getCharacterSprite(type->characterKey);
            } else {
                spr = Characters::getCharacterSprite(Characters::Fighter_Boss);
            }
        }
        sf::FloatRect bounds = spr.getLocalBounds();
        spr.setOrigin({bounds.size.x / 2.f, bounds.size.y / 2.f});
        sprites.push_back(spr);
    }
    palette.setSprites(sprites);
    palette.setLayout(8, 2, 48.f, 6.f);
    palette.setSelected(selectedSpawnIndex);
    updateCursor();
    updateDebugText();
}

// -----------------------------------------------------------------------------
void SpawnEditor::updateCursor() {
    if (selectedSpawnIndex >= 0 && selectedSpawnIndex < (int)spawnKeys.size()) {
        const std::string& key = spawnKeys[selectedSpawnIndex];
        sf::Sprite spr(Characters::getCharacterSprite(Characters::Player));
        if (key != "player") {
            const NPCType* type = NPCManager::get().getType(key);
            if (type) {
                spr = Characters::getCharacterSprite(type->characterKey);
            } else {
                spr = Characters::getCharacterSprite(Characters::Fighter_Boss);
            }
        }
        cursorSprite = spr;
        cursorSprite.setOrigin({0.f, 0.f});   // top-left origin
    }
}

// -----------------------------------------------------------------------------
void SpawnEditor::updateDebugText() {
    std::string text = "Selected: ";
    if (selectedSpawnIndex >= 0 && selectedSpawnIndex < (int)spawnKeys.size()) {
        text += spawnKeys[selectedSpawnIndex];
    } else {
        text += "none";
    }
    text += "\nTotal spawns: " + std::to_string(Terrain::getSpawnMap().size());
    debugText.setString(text);
}

// -----------------------------------------------------------------------------
void SpawnEditor::updatePaletteLayout(const sf::RenderWindow& window) {
    if (showPalette) {
        sf::Vector2f size = window.getDefaultView().getSize();
        palette.updateLayout({(unsigned)size.x, (unsigned)size.y});
    }
}

// -----------------------------------------------------------------------------
void SpawnEditor::handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    if (!active) return;

    if (showPalette) {
        updatePaletteLayout(window);
    }

    sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
    mouseWorldPos = window.mapPixelToCoords(mousePixel);

    // Palette interaction (left click to select)
    if (showPalette) {
        sf::Vector2f mouseDefault = window.mapPixelToCoords(mousePixel, window.getDefaultView());
        if (const auto* btn = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (btn->button == sf::Mouse::Button::Left) {
                if (palette.handleMousePress(mouseDefault)) {
                    selectedSpawnIndex = palette.getSelected();
                    updateCursor();
                    updateDebugText();
                    return;
                }
            }
        }
        if (const auto* scroll = event.getIf<sf::Event::MouseWheelScrolled>()) {
            if (palette.handleMouseScroll(scroll->delta)) {
                selectedSpawnIndex = palette.getSelected();
                updateCursor();
                updateDebugText();
                // don't return; we also allow scroll to change type
            }
        }
    }

    bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LControl) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RControl);

    // Arrow keys adjust cursor offset (in tile units)
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        switch (key->scancode) {
            case sf::Keyboard::Scancode::Up:    cursorOffset.y -= 1.f; break;
            case sf::Keyboard::Scancode::Down:  cursorOffset.y += 1.f; break;
            case sf::Keyboard::Scancode::Left:  cursorOffset.x -= 1.f; break;
            case sf::Keyboard::Scancode::Right: cursorOffset.x += 1.f; break;
            default: break;
        }
    }

    // Mouse clicks
    if (const auto* btn = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (ctrl && btn->button == sf::Mouse::Button::Left) {
            // Pick spawn from world – select for editing
            const auto& spawnMap = Terrain::getSpawnMap();
            for (auto it = spawnMap.rbegin(); it != spawnMap.rend(); ++it) {
                const auto& pos = it->first;
                const auto& props = it->second;
                sf::Sprite spr = Characters::getCharacterSprite(props.characterKey);
                spr.setOrigin({0.f, 0.f});
                spr.setPosition({pos.first * Scale::get(), pos.second * Scale::get()});
                spr.setScale(Scale::getVec() * props.scale);
                if (spr.getGlobalBounds().contains(mouseWorldPos)) {
                    selectSpawnForEditing(pos);
                    break;
                }
            }
        } else if (btn->button == sf::Mouse::Button::Left) {
            // Place spawn at mouse position + offset (converted to tile units)
            if (selectedSpawnIndex >= 0 && selectedSpawnIndex < (int)spawnKeys.size()) {
                startRecording();
                SpawnProps props;
                const std::string& key = spawnKeys[selectedSpawnIndex];
                if (key == "player") {
                    props.characterKey = Characters::Player;
                    props.npcTypeId = "player";
                } else {
                    const NPCType* type = NPCManager::get().getType(key);
                    if (type) {
                        props.characterKey = type->characterKey;
                        props.npcTypeId = key;
                    } else {
                        props.characterKey = key;
                        props.npcTypeId = key;
                    }
                }
                props.scale = 1.f;
                props.rotation = 0.f;
                props.flipX = false;
                props.flipY = false;
                props.uniqueID = "";
                props.scriptName = "";
                // Convert mouse world position to tile units, then add offset
                float tileX = mouseWorldPos.x / Scale::get() + cursorOffset.x;
                float tileY = mouseWorldPos.y / Scale::get() + cursorOffset.y;
                setSpawnWithUndo(tileX, tileY, props);
                stopRecording();
                updateDebugText();
            }
        } else if (btn->button == sf::Mouse::Button::Right) {
            // Erase spawn under cursor
            const auto& spawnMap = Terrain::getSpawnMap();
            for (auto it = spawnMap.rbegin(); it != spawnMap.rend(); ++it) {
                const auto& pos = it->first;
                const auto& props = it->second;
                sf::Sprite spr = Characters::getCharacterSprite(props.characterKey);
                spr.setOrigin({0.f, 0.f});
                spr.setPosition({pos.first * Scale::get(), pos.second * Scale::get()});
                spr.setScale(Scale::getVec() * props.scale);
                if (spr.getGlobalBounds().contains(mouseWorldPos)) {
                    startRecording();
                    eraseSpawn(pos.first, pos.second);
                    stopRecording();
                    updateDebugText();
                    break;
                }
            }
        }
    }

    // Scroll to change spawn type (if palette didn't consume it)
    if (const auto* scroll = event.getIf<sf::Event::MouseWheelScrolled>()) {
        if (!showPalette || !palette.isVisible() || !palette.handleMouseScroll(scroll->delta)) {
            int total = (int)spawnKeys.size();
            selectedSpawnIndex = std::clamp(selectedSpawnIndex + (int)scroll->delta, 0, total - 1);
            palette.setSelected(selectedSpawnIndex);
            updateCursor();
            updateDebugText();
        }
    }

    // ---- Property editing keyboard input ----
    if (m_editingProps) {
        if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
            // Tab to switch focus
            if (key->code == sf::Keyboard::Key::Tab) {
                switch (m_focusedField) {
                    case InputField::None:
                    case InputField::NpcType: m_focusedField = InputField::UniqueID; break;
                    case InputField::UniqueID: m_focusedField = InputField::Script; break;
                    case InputField::Script: m_focusedField = InputField::NpcType; break;
                }
                m_inputBuffer.clear();
                updatePropertyTexts();
                return;
            }
            // Escape to cancel editing
            if (key->code == sf::Keyboard::Key::Escape) {
                m_editingProps = false;
                m_focusedField = InputField::None;
                m_selectedSpawnProps = nullptr;
                return;
            }
            // Enter to save and exit
            if (key->code == sf::Keyboard::Key::Enter) {
                savePropertyChanges();
                m_editingProps = false;
                m_focusedField = InputField::None;
                return;
            }
            // Backspace
            if (key->code == sf::Keyboard::Key::Backspace) {
                if (!m_inputBuffer.empty()) {
                    m_inputBuffer.pop_back();
                    updatePropertyTexts();
                }
                return;
            }
        }

        // Text input (for actual characters)
        if (const auto* text = event.getIf<sf::Event::TextEntered>()) {
            if (text->unicode < 128 && text->unicode != 13 && text->unicode != 27) {
                char c = static_cast<char>(text->unicode);
                if (c >= 32 && c <= 126) { // printable
                    m_inputBuffer.push_back(c);
                    updatePropertyTexts();
                }
            }
        }
    }
}

// -----------------------------------------------------------------------------
void SpawnEditor::selectSpawnForEditing(const std::pair<float,float>& key) {
    const auto& spawnMap = Terrain::getSpawnMap();
    auto it = spawnMap.find(key);
    if (it == spawnMap.end()) {
        m_editingProps = false;
        m_selectedSpawnProps = nullptr;
        return;
    }
    m_selectedSpawnKey = key;
    m_selectedSpawnProps = const_cast<SpawnProps*>(&it->second);
    m_editingProps = true;
    m_focusedField = InputField::NpcType;
    m_inputBuffer.clear();
    updatePropertyTexts();
}

// -----------------------------------------------------------------------------
void SpawnEditor::updatePropertyTexts() {
    if (!m_selectedSpawnProps) return;

    std::string npcType = m_selectedSpawnProps->npcTypeId;
    std::string uniqueID = m_selectedSpawnProps->uniqueID;
    std::string script = m_selectedSpawnProps->scriptName;

    // If a field is focused, replace its text with the input buffer (if not empty)
    if (m_focusedField == InputField::NpcType && !m_inputBuffer.empty()) {
        npcType = m_inputBuffer;
    } else if (m_focusedField == InputField::UniqueID && !m_inputBuffer.empty()) {
        uniqueID = m_inputBuffer;
    } else if (m_focusedField == InputField::Script && !m_inputBuffer.empty()) {
        script = m_inputBuffer;
    }

    // Build label strings with cursor indicator
    std::string npcLabel = "npcTypeId: " + npcType + (m_focusedField == InputField::NpcType ? "|" : "");
    std::string uniqueLabel = "uniqueID: " + uniqueID + (m_focusedField == InputField::UniqueID ? "|" : "");
    std::string scriptLabel = "scriptName: " + script + (m_focusedField == InputField::Script ? "|" : "");

    m_propNpcType.setString(npcLabel);
    m_propUniqueID.setString(uniqueLabel);
    m_propScript.setString(scriptLabel);
}

// -----------------------------------------------------------------------------
void SpawnEditor::savePropertyChanges() {
    if (!m_selectedSpawnProps) return;

    // Apply the edited values (if input buffer has content, use it; otherwise keep existing)
    if (m_focusedField == InputField::NpcType && !m_inputBuffer.empty()) {
        m_selectedSpawnProps->npcTypeId = m_inputBuffer;
    } else if (m_focusedField == InputField::UniqueID && !m_inputBuffer.empty()) {
        m_selectedSpawnProps->uniqueID = m_inputBuffer;
    } else if (m_focusedField == InputField::Script && !m_inputBuffer.empty()) {
        m_selectedSpawnProps->scriptName = m_inputBuffer;
    }
    m_inputBuffer.clear();
    updatePropertyTexts();
}

// -----------------------------------------------------------------------------
void SpawnEditor::draw(sf::RenderWindow& window) {
    if (!active) return;

    // Draw existing spawns in the world view
    const auto& spawnMap = Terrain::getSpawnMap();
    for (const auto& [pos, props] : spawnMap) {
        sf::Sprite spr = Characters::getCharacterSprite(props.characterKey);
        spr.setOrigin({0.f, 0.f});
        sf::Vector2f worldPos(pos.first * Scale::get(), pos.second * Scale::get());
        spr.setPosition(worldPos);
        spr.setScale(Scale::getVec() * props.scale);
        window.draw(spr);

        // Draw label above sprite (npcTypeId or "player")
        sf::Text label(font);
        label.setCharacterSize(12);
        label.setFillColor(sf::Color::White);
        label.setOutlineColor(sf::Color::Black);
        label.setOutlineThickness(1.f);
        std::string labelText = (props.npcTypeId == "player") ? "player" : props.npcTypeId;
        if (labelText.empty()) labelText = props.characterKey;
        label.setString(labelText);
        sf::FloatRect labelBounds = label.getLocalBounds();
        float labelX = worldPos.x + (spr.getLocalBounds().size.x * Scale::get()) / 2.f - labelBounds.size.x / 2.f;
        float labelY = worldPos.y - labelBounds.size.y + 10.f;
        label.setPosition({labelX, labelY});
        window.draw(label);
    }

    // Draw cursor overlay (top-left origin)
    if (selectedSpawnIndex >= 0 && selectedSpawnIndex < (int)spawnKeys.size()) {
        cursorSprite.setPosition(mouseWorldPos + cursorOffset * Scale::get());
        window.draw(cursorSprite);
    }

    // Palette and debug text in default view
    sf::View defaultView = window.getDefaultView();
    window.setView(defaultView);
    if (showPalette) {
        updatePaletteLayout(window);
        palette.draw(window);
        sf::Vector2f viewSize = window.getDefaultView().getSize();
        debugText.setPosition({viewSize.x - debugText.getLocalBounds().size.x - 20.f, 20.f});
        window.draw(debugText);
    }

    // ---- Property panel ----
    if (m_editingProps && m_selectedSpawnProps) {
        updatePropertyTexts();

        // Compute panel size and position
        float padding = 15.f;
        float lineHeight = 25.f;
        float width = 300.f;
        float height = padding * 2 + 25.f + 3 * lineHeight + 10.f; // title + 3 fields + help
        float x = 20.f;
        float y = 80.f; // below debug text

        m_propPanel.setSize({width, height});
        m_propPanel.setPosition({x, y});

        // Position title
        m_propTitle.setPosition({x + padding, y + padding});
        // Position fields
        float fieldY = y + padding + 25.f + 5.f;
        m_propNpcType.setPosition({x + padding, fieldY});
        m_propUniqueID.setPosition({x + padding, fieldY + lineHeight});
        m_propScript.setPosition({x + padding, fieldY + 2 * lineHeight});

        // Help text at bottom
        m_propHelp.setPosition({x + padding, y + height - padding - 20.f});

        // Draw
        window.draw(m_propPanel);
        window.draw(m_propTitle);
        window.draw(m_propNpcType);
        window.draw(m_propUniqueID);
        window.draw(m_propScript);
        window.draw(m_propHelp);
    }

    window.setView(defaultView); // restore world view
}

// -----------------------------------------------------------------------------
void SpawnEditor::placeSpawn(float x, float y, const SpawnProps& props) {
    Terrain::setSpawn(x, y, props);
}

void SpawnEditor::eraseSpawn(float x, float y) {
    Terrain::eraseSpawn(x, y);
}

// -----------------------------------------------------------------------------
void SpawnEditor::setSpawnWithUndo(float x, float y, const SpawnProps& newProps) {
    auto oldProps = Terrain::getSpawn(x, y);
    if (oldProps.characterKey == newProps.characterKey &&
        oldProps.npcTypeId == newProps.npcTypeId &&
        oldProps.scale == newProps.scale &&
        oldProps.rotation == newProps.rotation &&
        oldProps.flipX == newProps.flipX &&
        oldProps.flipY == newProps.flipY) {
        return;
    }
    SpawnChange change{x, y, oldProps, newProps};
    undoStack.addChange(change);
    Terrain::setSpawn(x, y, newProps);
}

// -----------------------------------------------------------------------------
void SpawnEditor::undo() { undoStack.undo(); updateDebugText(); }
void SpawnEditor::redo() { undoStack.redo(); updateDebugText(); }
void SpawnEditor::setActive(bool a) { active = a; }
bool SpawnEditor::isActive() const { return active; }
void SpawnEditor::setPaletteVisible(bool visible) { showPalette = visible; }
void SpawnEditor::togglePaletteVisibility() { showPalette = !showPalette; }
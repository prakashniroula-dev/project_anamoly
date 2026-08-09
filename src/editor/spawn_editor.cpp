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
{
    debugText.setCharacterSize(14);
    debugText.setFillColor(sf::Color::White);
    debugText.setOutlineColor(sf::Color::Black);
    debugText.setOutlineThickness(1.f);
}

// -----------------------------------------------------------------------------
void SpawnEditor::init() {
    // Start with a clean undo state (spawns are assumed already loaded)
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
        // Use top‑left origin (default is (0,0)) – no need to set explicitly
        cursorSprite.setOrigin({0.f, 0.f});   // <-- changed from center
        // The sprite already has Scale::getVec() from getCharacterSprite, so we don't set it again.
        cursorSprite.setRotation(sf::Angle(sf::degrees(0.f)));
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
            // Pick spawn from world – find the topmost spawn at cursor
            const auto& spawnMap = Terrain::getSpawnMap();
            for (auto it = spawnMap.rbegin(); it != spawnMap.rend(); ++it) {
                const auto& pos = it->first;
                const auto& props = it->second;
                sf::Sprite spr = Characters::getCharacterSprite(props.characterKey);
                sf::FloatRect bounds = spr.getLocalBounds();
                spr.setOrigin({0.f, 0.f});   // <-- changed from center
                spr.setPosition({pos.first * Scale::get(), pos.second * Scale::get()});
                spr.setScale(Scale::getVec() * props.scale);
                if (spr.getGlobalBounds().contains(mouseWorldPos)) {
                    std::string key;
                    if (props.characterKey == Characters::Player) {
                        key = "player";
                    } else {
                        key = props.npcTypeId.empty() ? props.characterKey : props.npcTypeId;
                    }
                    auto itKey = std::find(spawnKeys.begin(), spawnKeys.end(), key);
                    if (itKey != spawnKeys.end()) {
                        selectedSpawnIndex = (int)std::distance(spawnKeys.begin(), itKey);
                        palette.setSelected(selectedSpawnIndex);
                        updateCursor();
                        updateDebugText();
                    }
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
                sf::FloatRect bounds = spr.getLocalBounds();
                spr.setOrigin({bounds.size.x / 2.f, bounds.size.y / 2.f});
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
}

// -----------------------------------------------------------------------------
void SpawnEditor::draw(sf::RenderWindow& window) {
    if (!active) return;

    // 1) Draw all existing spawns with top‑left origin
    const auto& spawnMap = Terrain::getSpawnMap();
    sf::Text label(font);
    label.setCharacterSize(12);
    label.setFillColor(sf::Color::White);
    label.setOutlineColor(sf::Color::Black);
    label.setOutlineThickness(1.f);
    float overlapPixels = 32.f; // pixels to overlap the sprite when drawing label above it

    for (const auto& [pos, props] : spawnMap) {
        sf::Sprite spr = Characters::getCharacterSprite(props.characterKey);
        spr.setOrigin({0.f, 0.f});
        sf::Vector2f worldPos(pos.first * Scale::get(), pos.second * Scale::get());
        spr.setPosition(worldPos);
        window.draw(spr);

        // Draw label above the sprite
        std::string labelText = props.npcTypeId.empty() ? props.characterKey : props.npcTypeId;
        if (labelText == Characters::Player) labelText = "player";
        label.setString(labelText);
        // Center the label horizontally above the sprite
        sf::FloatRect labelBounds = label.getLocalBounds();
        float labelX = worldPos.x + (spr.getLocalBounds().size.x * Scale::get()) / 2.f - labelBounds.size.x / 2.f;
        float labelY = worldPos.y - labelBounds.size.y + overlapPixels;
        label.setPosition({labelX, labelY});
        window.draw(label);
    }

    // 2) Draw cursor overlay (top‑left origin)
    if (selectedSpawnIndex >= 0 && selectedSpawnIndex < (int)spawnKeys.size()) {
        cursorSprite.setPosition(mouseWorldPos + cursorOffset * Scale::get());
        window.draw(cursorSprite);
    }

    // 3) Palette and debug text in default view (unchanged) ...
    sf::View defaultView = window.getDefaultView();
    window.setView(defaultView);
    if (showPalette) {
        updatePaletteLayout(window);
        palette.draw(window);
        sf::Vector2f viewSize = window.getDefaultView().getSize();
        debugText.setPosition({viewSize.x - debugText.getLocalBounds().size.x - 20.f, 20.f});
        window.draw(debugText);
    }
    window.setView(defaultView);
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
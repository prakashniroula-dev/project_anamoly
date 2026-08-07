#include "object_editor.hpp"
#include <entities/terrain.hpp>
#include <entities/objects.hpp>
#include <core/scale.hpp>
#include <core/constants.hpp>
#include <algorithm>

void applyObjectChange(ObjectChange& change, bool forward) {
    const auto& props = forward ? change.newProps : change.oldProps;
    if (props.index < 0)
        Terrain::eraseObject(change.x, change.y);
    else
        Terrain::setObject(change.x, change.y, props);
}

ObjectEditor::ObjectEditor(const sf::Font& font)
    : palette(font), objectCursor(dummyTexture) {}

void ObjectEditor::updatePaletteLayout(const sf::Vector2u& windowSize) {
    if (showPalette) {
        palette.updateLayout(windowSize);
    }
}

void ObjectEditor::init() {
    const int numObjects = Objects::getCount();
    std::vector<sf::Sprite> sprites;
    sprites.reserve(numObjects);
    for (int i = 0; i < numObjects; ++i) {
        sprites.push_back(Objects::getObjectSprite(i));
        sprites.back().setOrigin({0.f, 0.f});
    }
    palette.setSprites(sprites);
    palette.setLayout(8, 3, 24.f, 3.f);
    palette.setSelected(selectedObject);
    updateCursor();
}

void ObjectEditor::setActive(bool a) { active = a; }
bool ObjectEditor::isActive() const { return active; }
void ObjectEditor::setPaletteVisible(bool visible) { showPalette = visible; }
void ObjectEditor::togglePaletteVisibility() { showPalette = !showPalette; }

void ObjectEditor::updateCursor() {
    if (selectedObject >= 0 && selectedObject < Objects::getCount()) {
        objectCursor = Objects::getObjectSprite(selectedObject);
        sf::FloatRect bounds = objectCursor.getLocalBounds();
        objectCursor.setOrigin(sf::Vector2f(bounds.size.x / 2.f, bounds.size.y / 2.f));
        objectCursor.setScale({1.f, 1.f});
    }
}

void ObjectEditor::handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    if (!active) return;

    // ★ Update palette layout NOW so sprite positions are current for hit-testing ★
    if (showPalette) {
        palette.updateLayout(window.getSize());
    }

    sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
    mouseWorldPos = window.mapPixelToCoords(mousePixel);

    // Palette interaction
    if (showPalette) {
        sf::Vector2f mouseDefault = window.mapPixelToCoords(mousePixel, window.getDefaultView());
        if (const auto* btn = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (btn->button == sf::Mouse::Button::Left) {
                if (palette.handleMousePress(mouseDefault)) {
                    selectedObject = palette.getSelected();
                    updateCursor();
                    return;
                }
            }
        }
    }

    bool shiftHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LShift) ||
                     sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::RShift);

    if (const auto* btn = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (btn->button == sf::Mouse::Button::Left) {
            startRecording();
            paintObject(mouseWorldPos.x, mouseWorldPos.y);
            stopRecording();
        } else if (btn->button == sf::Mouse::Button::Right) {
            const auto& order = Terrain::getObjectOrder();
            for (auto it = order.rbegin(); it != order.rend(); ++it) {
                const auto& key = *it;
                auto mapIt = Terrain::getObjectMap().find(key);
                if (mapIt == Terrain::getObjectMap().end()) continue;
                const auto& props = mapIt->second;
                sf::Sprite spr = Objects::getObjectSprite(props.index);
                sf::FloatRect bounds = spr.getLocalBounds();
                spr.setOrigin(sf::Vector2f(bounds.size.x / 2.f, bounds.size.y / 2.f));
                sf::Vector2f worldPos(key.first * Scale::get(), key.second * Scale::get());
                spr.setPosition(worldPos);
                spr.setScale(Scale::getVec() * props.scale);
                if (spr.getGlobalBounds().contains(mouseWorldPos)) {
                    startRecording();
                    eraseObject(key.first, key.second);
                    stopRecording();
                    break;
                }
            }
        }
    }

    if (const auto* scroll = event.getIf<sf::Event::MouseWheelScrolled>()) {
        if (shiftHeld) {
            currentObjectScale = std::clamp(currentObjectScale + scroll->delta * 0.1f, 0.1f, 5.0f);
        } else {
            
            int total = Objects::getCount();
            selectedObject = std::clamp(selectedObject + static_cast<int>(scroll->delta), 0, total - 1);
            palette.setSelected(selectedObject);
            updateCursor();
        }
    }
}

void ObjectEditor::draw(sf::RenderWindow& window) {
    if (!active) return;

    // Draw cursor
    if (selectedObject >= 0 && selectedObject < Objects::getCount()) {
        objectCursor.setPosition(mouseWorldPos);
        sf::Vector2f worldScale = Scale::getVec();
        objectCursor.setScale(worldScale * currentObjectScale);
        window.draw(objectCursor);
    }

    // UI
    sf::View defaultView = window.getDefaultView();
    window.setView(defaultView);
    if (showPalette) {
        palette.updateLayout(window.getSize());
        palette.draw(window);
    }
    window.setView(defaultView);
}

void ObjectEditor::paintObject(float x, float y) {
    float s = Scale::get();
    ObjectProps newProps{currentObjectScale, selectedObject};
    setObjectWithUndo(x / s, y / s, newProps, true);
}

void ObjectEditor::eraseObject(float x, float y) {
    ObjectProps empty{0.f, -1};
    setObjectWithUndo(x, y, empty, true);
}

void ObjectEditor::setObjectWithUndo(float x, float y, const ObjectProps& newProps, bool isPaint) {
    auto& objMap = Terrain::getObjectMap();
    auto it = objMap.find({x, y});
    ObjectProps oldProps = (it != objMap.end()) ? it->second : ObjectProps{0.f, -1};
    if (oldProps.index == newProps.index && oldProps.scale == newProps.scale) return;

    ObjectChange change{x, y, oldProps, newProps, isPaint};
    undoStack.addChange(change);
    if (newProps.index < 0)
        Terrain::eraseObject(x, y);
    else
        Terrain::setObject(x, y, newProps);
}

void ObjectEditor::undo() { undoStack.undo(); }
void ObjectEditor::redo() { undoStack.redo(); }
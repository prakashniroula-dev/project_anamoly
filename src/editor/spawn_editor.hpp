#pragma once
#include <SFML/Graphics.hpp>
#include "undo_stack.hpp"
#include "palette.hpp"
#include <entities/npc_manager.hpp>
#include <map/terrain.hpp>
#include <string>
#include <vector>

struct SpawnChange {
    float x, y;
    SpawnProps oldProps;
    SpawnProps newProps;
};

void applySpawnChange(SpawnChange& change, bool forward);

class SpawnEditor {
public:
    explicit SpawnEditor(const sf::Font& font);
    void init();  // now loads spawns from file and resets undo

    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void draw(sf::RenderWindow& window);
    void setActive(bool active);
    bool isActive() const;
    void setPaletteVisible(bool visible);
    void togglePaletteVisibility();

    void undo();
    void redo();

    // Exposed so LevelEditor can force layout update before event handling
    void updatePaletteLayout(const sf::RenderWindow& window);

private:
    bool active = false;
    bool showPalette = true;
    sf::Vector2f cursorOffset = {0.f, 0.f};
    sf::Vector2f mouseWorldPos;

    Palette palette;
    int selectedSpawnIndex = 0;
    std::vector<std::string> spawnKeys;   // "player" + all NPC type IDs

    sf::Sprite cursorSprite;
    sf::Texture dummyTexture;

    // Debug text showing selected NPC ID and total spawn count
    sf::Text debugText;

    const sf::Font& font;

    using SpawnUndoStack = UndoStack<SpawnChange, applySpawnChange>;
    SpawnUndoStack undoStack;

    void updateCursor();
    void updateDebugText();

    

    void placeSpawn(float x, float y, const SpawnProps& props);
    void eraseSpawn(float x, float y);
    void setSpawnWithUndo(float x, float y, const SpawnProps& newProps);

    void startRecording() { undoStack.beginGroup(); }
    void stopRecording() { undoStack.commitGroup(); }

    friend void applySpawnChange(SpawnChange& change, bool forward);

    // Property editing state
    bool m_editingProps = false;
    std::pair<float, float> m_selectedSpawnKey; // selected spawn for editing
    SpawnProps* m_selectedSpawnProps = nullptr;

    // UI elements for property editing
    sf::RectangleShape m_propPanel;
    sf::Text m_propTitle;
    sf::Text m_propNpcType;
    sf::Text m_propUniqueID;
    sf::Text m_propScript;
    sf::Text m_propHelp;

    // Input focus states
    enum class InputField { None, NpcType, UniqueID, Script };
    InputField m_focusedField = InputField::None;
    std::string m_inputBuffer;

    void selectSpawnForEditing(const std::pair<float, float>& key);
    void updatePropertyTexts();
    void savePropertyChanges();

    // Helper to get character sprite for a given spawn
    sf::Sprite getSpawnSprite(const std::pair<float,float>& pos, const SpawnProps& props);
};
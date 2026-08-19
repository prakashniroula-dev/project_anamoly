#pragma once
#include <string>
#include <optional>
#include <map/types.hpp>
#include <SFML/System/Vector2.hpp>
#include <map/map_data.hpp>

namespace Game { class Game; }

class MapManager {
public:
    static MapManager& get();
    void setBaseDir(const std::string& dir) { baseDir = dir; }
    void loadMap(const std::string& mapName);
    void saveCurrentMap() const;
    void switchToMap(const std::string& mapName, std::optional<sf::Vector2f> spawnPos = std::nullopt);
    
    // Accessors
    const MapData& getData() const { return m_data; }
    MapData& getData() { return m_data; }   // use with care (mainly for editor)

    const std::vector<Transition>& getTransitions() const { return m_data.transitions; }
    std::optional<Transition> getTransitionAt(const sf::Vector2f& position, float threshold = 50.f) const;
    void checkCutsceneTriggers(const sf::Vector2f& playerPos);

    std::string getCurrentMap() const { return currentMap; }
    void setGame(Game::Game* game) { m_game = game; }

    // Spawn all NPCs from current map's spawn data
    void spawnNPCs();

    void clearCurrentMapData();
    Game::Game* getGame() const { return m_game; }

private:
    MapManager() = default;
    
    MapData m_data;
    std::string currentMap = "default";
    std::string baseDir = "assets/maps/";
    Game::Game* m_game = nullptr;
};
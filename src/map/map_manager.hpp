#pragma once
#include <string>
#include <filesystem>
#include <SFML/System/Vector2.hpp>
#include <vector>
#include <optional>
// #include <game/game.hpp>

namespace Game {
    class Game;
};
class Character;

// map/map_manager.hpp
struct Transition {
    sf::Vector2f triggerPos;
    std::string targetMap;
    std::optional<sf::Vector2f> spawnPosition;
    std::string label;   // mandatory label, e.g. "Door", "Elevator"
};

class MapManager {
public:
    static MapManager& get();

    void setBaseDir(const std::string& dir) { baseDir = dir; }
    std::filesystem::path getPath(const std::string& filename) const;

    void loadMap(const std::string& mapName);
    void saveCurrentMap() const;
    void switchToMap(const std::string& mapName, std::optional<sf::Vector2f> spawnPos = std::nullopt);

    // Returns the transition info if player is near a transition, otherwise std::nullopt
    std::optional<Transition> getTransitionAt(const sf::Vector2f& position, float threshold = 50.f) const;

    std::string getCurrentMap() const { return currentMap; }
    const std::vector<Transition>& getTransitions() const { return transitions; }

    void setPlayer(Character* player) { m_player = player; }
    void spawnNPCs();
    void setGame(Game::Game* game) { m_game = game; }
    void clearCurrentMapData();

private:
    MapManager() = default;
    Game::Game* m_game = nullptr;
    std::string currentMap = "default";
    std::string baseDir = "assets/maps/";
    std::vector<Transition> transitions;
    Character* m_player = nullptr;

};
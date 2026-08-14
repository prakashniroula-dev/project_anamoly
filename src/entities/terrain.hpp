#pragma once
#include <SFML/Graphics/RenderWindow.hpp>
#include <unordered_map>
#include <vector>
#include <utility>
#include <functional>
#include <graphics/tiles.hpp>
#include <entities/objects.hpp>

using SolidMap = std::map<std::pair<int, int>, int>;
struct NpcSpawn {
    sf::Vector2f position;
    std::string characterKey;   // matches Characters::... constants
};

struct SpawnProps {
    std::string characterKey;
    float scale = 1.f;
    float rotation = 0.f;
    bool flipX = false;
    bool flipY = false;
    std::string npcTypeId;
    std::string uniqueID;
    std::string scriptName;   // NEW: if not empty, this NPC will run this script on spawn/trigger
    std::vector<sf::Vector2f> waypoints;
};

using SpawnMap = std::map<std::pair<float, float>, SpawnProps>;

namespace Terrain {
    void draw(sf::RenderWindow& win, float dt);
    void setMap(const TileMap& new_map);
    const TileMap& getMap();
    void update(float dt);
    void loadFromFile(const std::string& filename);
    void saveToFile(const std::string& filename);
    // New overloads – keep existing ones for backward compatibility if needed
    void setTile(int x, int y, int encodedTile);              // replaces old setTile
    void addTile(int x, int y, int encodedTile);              // replaces old addTile
    void setTileVector(int x, int y, const std::vector<int>& encodedTiles);
    std::vector<int> getTile(int x, int y);                   // returns encoded ints
    void eraseTile(int x, int y);

    const SolidMap& getSolidMap();
    void setSolidTile(int x, int y, int type);   // type > 0 = solid
    void eraseSolidTile(int x, int y);
    int getSolidTile(int x, int y);              // returns 0 if none
    void loadSolidFromFile(const std::string& filename);
    void saveSolidToFile(const std::string& filename);

    // Update isSolidTile to check both normal tiles and solid layer
    bool isSolidTile(int x, int y);  // already declared; we'll modify its implementation

    // Object functions
    const ObjectMap& getObjectMap();                         // for lookups (may be unordered)
    const std::vector<std::pair<float,float>>& getObjectOrder();  // insertion order
    void setObject(float x, float y, const ObjectProps& props);
    void eraseObject(float x, float y);
    void loadObjectsFromFile(const std::string& filename);
    void saveObjectsToFile(const std::string& filename);
    ObjectProps getObject(float x, float y);

    const SpawnMap& getSpawnMap();
    void setSpawn(float x, float y, const SpawnProps& props);
    void eraseSpawn(float x, float y);
    SpawnProps getSpawn(float x, float y);  // returns empty key if not found
    void clearSpawns();

    void loadSpawnsFromFile(const std::string& filename);
    void saveSpawnsToFile(const std::string& filename);

    // Unified load/save for all spawns (replaces the old single-spawn functions)
    void loadSpawnsFromFile(const std::string& filename);
    void saveSpawnsToFile(const std::string& filename);

    sf::Vector2f getPlayerSpawnPosition();  // returns default if none set
    void setPlayerSpawnPosition(const sf::Vector2f& pos);


}
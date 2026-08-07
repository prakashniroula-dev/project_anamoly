#pragma once
#include <SFML/Graphics/RenderWindow.hpp>
#include <unordered_map>
#include <vector>
#include <utility>
#include <functional>
#include <graphics/tiles.hpp>
#include <entities/objects.hpp>

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
    // In Terrain namespace
    bool isSolidTile(int x, int y); // Check if any tile at (x,y) is solid

    // Object functions
    const ObjectMap& getObjectMap();                         // for lookups (may be unordered)
    const std::vector<std::pair<float,float>>& getObjectOrder();  // insertion order
    void setObject(float x, float y, const ObjectProps& props);
    void eraseObject(float x, float y);
    void loadObjectsFromFile(const std::string& filename);
    void saveObjectsToFile(const std::string& filename);
    ObjectProps getObject(float x, float y);

}
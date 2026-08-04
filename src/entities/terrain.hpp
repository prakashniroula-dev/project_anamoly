// src/entities/Terrain.hpp
#pragma once
#include <SFML/Graphics/RenderWindow.hpp>
#include <map>
#include <utility>

// Forward declare TileMap (we don't need the full definition here)
using TileMap = std::map<std::pair<int, int>, int>;

namespace Terrain {
    // Declarations only - no function bodies!
    void draw(sf::RenderWindow& win, float dt);
    void setMap(const TileMap& new_map);
    const TileMap& getMap();
    void update(float dt);
    void loadFromFile(const std::string& filename);
    void saveToFile(const std::string& filename);
    void setTile(int x, int y, int id);
    void eraseTile(int x, int y);
    int getTile(int x, int y); // Added getter for tile ID
}
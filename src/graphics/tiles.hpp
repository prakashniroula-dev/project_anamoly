// src/graphics/Tiles.hpp
#pragma once
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>
#include <map>
#include <utility>
#include <vector>
#include <set>

// This type alias is needed by terrain.hpp and others, so it stays in the header.
using TileMap = std::map<std::pair<int, int>, std::vector<int>>;

namespace Tiles {
    // Declarations only. No function bodies here!
    void load();
    int getCount();  // Returns total number of tile types (sum of all tileKeys counts)

    sf::Sprite getTileSprite(int tile_id);

    // In Tiles.hpp or a new utility header
    inline std::vector<int> getTileSafe(const TileMap& map, int tx, int ty) {
        auto it = map.find({tx, ty});
        return (it != map.end()) ? it->second : std::vector<int>();  // 0 = empty/no tile
    }

    bool isSolidTile(const TileMap& terrain_map, int tx, int ty);

    sf::Vector2f getTilePosition(int tx, int ty);

    sf::Vector2i getTileGridPosition(sf::Vector2f pos);
}
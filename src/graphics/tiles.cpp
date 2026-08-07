// src/graphics/Tiles.cpp
#include <graphics/tiles.hpp>
#include <graphics/textures.hpp>        // Now we safely include the implementation header
#include <core/scale.hpp> // Assuming you move scale.hpp to core/math/ later
#include <cmath>               // for std::floor
#include <vector>
#include <core/constants.hpp>           // For Constants::WORLD_HEIGHT_TILES
#include <unordered_map>

namespace Tiles {

    const std::vector<std::pair<std::string, int>> tileKeys = {
        {"power_station", 64},
        {"industrial_zone", 81}
    };
    const int tile_width = 32;
    const int tile_height = 32;

    int getCount() {
        int total = 0;
        for (const auto& [key, count] : tileKeys) {
            total += count;
        }
        return total;
    }

    static const std::vector<int> non_solid_tiles = 
    {
        9, 36
    };

    void load() {
        for (const auto& [key, count] : tileKeys) {
            std::string path = key + "/tile/tileset.png";
            Textures::load(key, path);
        }
    }

    sf::Sprite getTileSprite(int tile_id) {
        std::string tileKey;
        for (const auto& [key, count] : tileKeys) {
            if (tile_id < count) {
                tileKey = key;
                break;
            }
            tile_id -= count;
        }
        sf::Sprite s(Textures::get(tileKey));
        int tiles_per_row = Textures::get(tileKey).getSize().x / tile_width;
        int row = tile_id / tiles_per_row;
        int col = tile_id % tiles_per_row;
        s.setTextureRect(sf::IntRect(
            sf::Vector2i(col * tile_width, row * tile_height),
            sf::Vector2i(tile_width, tile_height)
        ));
        return s;
    }

    bool isSolidTile(const TileMap& terrain_map, int tx, int ty) {
        auto it = terrain_map.find({tx, ty});
        if (it == terrain_map.end()) {
            return false;
        }
        bool isSolid = false;
        for (int tile_id : it->second) {
            if (tile_id <= 48 || tile_id >= 81) {
                isSolid = true;
                for (int nonSolidID : non_solid_tiles) {
                    if (tile_id == nonSolidID) {
                        isSolid = false;
                        break;
                    }
                }
                if (isSolid) {
                    return true; // If any tile is solid, return true
                }
            }
        }
        return isSolid;
    }

    sf::Vector2f getTilePosition(int tx, int ty) {
        float s = Scale::get();
        return sf::Vector2f(
            tx * 32.f * s,
            32.f * Constants::WORLD_HEIGHT_TILES * s - ty * 32.f * s
        );
    }

    sf::Vector2i getTileGridPosition(sf::Vector2f scaled_pos) {
        float s = Scale::get();
        scaled_pos /= s; // Convert back to unscaled coordinates

        const float EPSILON = 0.0001f;
        scaled_pos.y -= EPSILON;
        
        int tx = static_cast<int>(std::floor(scaled_pos.x / Constants::TILE_SIZE));
        int ty = static_cast<int>(std::floor((Constants::TILE_SIZE * Constants::WORLD_HEIGHT_TILES - scaled_pos.y) / Constants::TILE_SIZE));
        return sf::Vector2i(tx, ty);
    }
}
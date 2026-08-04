// src/graphics/Tiles.cpp
#include <graphics/tiles.hpp>
#include <graphics/textures.hpp>        // Now we safely include the implementation header
#include <core/scale.hpp> // Assuming you move scale.hpp to core/math/ later
#include <cmath>               // for std::floor
#include <vector>
#include <core/constants.hpp>           // For Constants::WORLD_HEIGHT_TILES

namespace Tiles {

    // This variable now lives ONLY in this .cpp file. No linker errors!
    std::string tileset_key = "tileset";
    const int tile_width = 32;
    const int tile_height = 32;

    static const std::vector<int> non_solid_tiles = 
    {
        9, 36
    };

    void load() {
        static std::string tiles_path = "power_station/tile/tileset.png";
        Textures::load("tileset", tiles_path);
    }

    sf::Sprite getTileSprite(int tile_id) {
        sf::Sprite s(Textures::get(tileset_key));
        int tiles_per_row = Textures::get(tileset_key).getSize().x / tile_width;
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
        bool isSolid = it != terrain_map.end() && it->second <= 48;
        if ( isSolid ) {
            for (int nonSolidID : non_solid_tiles) {
                if (it->second == nonSolidID) {
                    isSolid = false;
                    break;
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

    sf::Vector2i getTileGridPosition(sf::Vector2f pos) {
        float s = Scale::get();
        int tx = static_cast<int>(std::floor(pos.x / (32.f * s)));
        int ty = static_cast<int>(std::floor((32.f * Constants::WORLD_HEIGHT_TILES * s - pos.y) / (32.f * s)));
        return sf::Vector2i(tx, ty);
    }
}
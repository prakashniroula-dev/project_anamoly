// src/entities/Terrain.cpp
#include <core/scale.hpp>        // For Scale::getVec
#include <entities/terrain.hpp>
#include <graphics/tiles.hpp>    // For Tiles::getTile, getTilePosition
#include <SFML/Graphics/Sprite.hpp>
#include <fstream>
#include <sstream>
#include <core/constants.hpp>

namespace {
    // This is the ONE and ONLY terrain map in your entire program.
    // It's hidden inside this .cpp file so everyone shares it.
    TileMap terrain = {
        {{0, 1}, 1},
        {{1, 1}, 1},
        {{2, 1}, 1},
        {{3, 1}, 1},
        {{4, 1}, 10},
        {{5, 1}, 10},
        {{6, 1}, 10},
        {{4, 2}, 1},
        {{5, 2}, 1},
        {{6, 2}, 1},
    };
}

namespace Terrain {
    void draw(sf::RenderWindow& win, float dt) {
        for (const auto& [pos, tile_id] : terrain) {
            sf::Sprite s = Tiles::getTileSprite(tile_id);
            auto [x, y] = pos;
            s.setPosition(Tiles::getTilePosition(x, y));
            s.setScale(Scale::getVec());
            win.draw(s);
        }
    }

    void setMap(const TileMap& new_map) {
        terrain = new_map;
    }

    const TileMap& getMap() {
        return terrain;
    }

    void update(float dt) {
        // Update terrain here
    }
}

void Terrain::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return;
    }
    terrain.clear();
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        int x, y, id;
        char comma1, comma2;
        if (iss >> x >> comma1 >> y >> comma2 >> id) {
            if (comma1 == ',' && comma2 == ',') {
                terrain[{x, y}] = id;
            }
        }
    }
}

void Terrain::saveToFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    for (const auto& [pos, id] : terrain) {
        file << pos.first << "," << pos.second << "," << id << "\n";
    }
}

// src/entities/Terrain.cpp (add at the end)
void Terrain::setTile(int x, int y, int id) {
    terrain[{x, y + 1}] = id;
}

void Terrain::eraseTile(int x, int y) {
    terrain.erase({x, y + 1});
}
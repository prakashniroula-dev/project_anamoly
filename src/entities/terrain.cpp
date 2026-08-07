#include <debug/logs.hpp>
#include <core/scale.hpp>
#include <entities/terrain.hpp>
#include <graphics/tiles.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <fstream>
#include <sstream>
#include <core/constants.hpp>
#include <algorithm> // for std::find

namespace
{
    // Tile map
    TileMap terrain = {
        {{0, 1}, {1}},
        {{1, 1}, {1}},
        {{2, 1}, {1}},
        {{3, 1}, {1}},
        {{4, 1}, {10}},
        {{5, 1}, {10}},
        {{6, 1}, {10}},
        {{4, 2}, {1}},
        {{5, 2}, {1}},
        {{6, 2}, {1}},
    };

    // Object storage (unordered for O(1) lookup)
    ObjectMap objectMap;
    // Insertion order (used for drawing and save/load order)
    std::vector<std::pair<float, float>> objectOrder;
}

namespace Terrain
{
    // ------------------ Tile functions (unchanged) ------------------
    void draw(sf::RenderWindow &win, float dt)
    {
        // Draw tiles
        for (const auto &[pos, tile_ids] : terrain)
        {
            for (int tile_id : tile_ids)
            {
                sf::Sprite s = Tiles::getTileSprite(tile_id);
                auto [x, y] = pos;
                s.setPosition(Tiles::getTilePosition(x, y));
                s.setScale(Scale::getVec());
                win.draw(s);
            }
        }

        // Draw objects in insertion order (oldest first, newest last = on top)
        for (const auto &key : objectOrder)
        {
            auto it = objectMap.find(key);
            if (it == objectMap.end())
                continue; // safety
            const auto &props = it->second;
            sf::Sprite s = Objects::getObjectSprite(props.index);
            sf::FloatRect bounds = s.getLocalBounds();
            s.setOrigin(sf::Vector2f(bounds.size.x / 2.f, bounds.size.y / 2.f));
            sf::Vector2f worldPos(key.first * Scale::get(), key.second * Scale::get());
            s.setPosition(worldPos);
            s.setScale(Scale::getVec() * props.scale);
            win.draw(s);
        }
    }

    void setMap(const TileMap &new_map)
    {
        terrain = new_map;
    }

    const TileMap &getMap()
    {
        return terrain;
    }

    void update(float dt)
    {
        // Not used
    }

    // ------------------ Tile file I/O ------------------
    void loadFromFile(const std::string &filename)
    {
        std::ifstream file(filename);
        if (!file.is_open())
            return;
        terrain.clear();
        std::string line;
        while (std::getline(file, line))
        {
            if (line.empty())
                continue;
            std::istringstream iss(line);
            int x, y, id;
            char comma1, comma2;
            if (iss >> x >> comma1 >> y >> comma2 >> id)
            {
                if (comma1 == ',' && comma2 == ',')
                {
                    terrain[{x, y}].push_back(id);
                }
            }
        }
    }

    void saveToFile(const std::string &filename)
    {
        std::ofstream file(filename);
        if (!file.is_open())
            return;
        for (const auto &[pos, ids] : terrain)
        {
            for (const auto& id: ids ) {
                file << pos.first << "," << pos.second << "," << id << "\n";
            }
        }
    }

    void setTile(int x, int y, int id)
    {
        if (id < 0)
        {
            terrain.erase({x, y + 1});
        }
        else
        {
            terrain[{x, y + 1}] = {id};
        }
    }

    void addTile(int x, int y, int id) {
        std::vector<int>& vec = terrain[{x, y+1}];
        if (id < 0) {
            vec.erase(std::remove(vec.begin(), vec.end(), id), vec.end());
        } else {
            vec.push_back(id);
        }
    }

    void eraseTile(int x, int y)
    {
        terrain.erase({x, y + 1});
    }

    std::vector<int> getTile(int x, int y)
    {
        auto it = terrain.find({x, y + 1});
        if (it != terrain.end())
        {
            return it->second;
        }
        return {};
    }

    // ------------------ Object management (with order) ------------------
    const ObjectMap &getObjectMap()
    {
        return objectMap;
    }

    const std::vector<std::pair<float, float>> &getObjectOrder()
    {
        return objectOrder;
    }

    void setObject(float x, float y, const ObjectProps &props)
    {
        std::pair<float, float> key = {x, y};
        auto it = objectMap.find(key);
        if (it == objectMap.end())
        {
            // New key – add to order vector
            objectOrder.push_back(key);
        }
        objectMap[key] = props;
    }

    void eraseObject(float x, float y)
    {
        std::pair<float, float> key = {x, y};
        objectMap.erase(key);
        // Remove from order vector
        auto it = std::find(objectOrder.begin(), objectOrder.end(), key);
        if (it != objectOrder.end())
        {
            objectOrder.erase(it);
        }
    }

    ObjectProps getObject(float x, float y)
    {
        auto it = objectMap.find({x, y});
        if (it != objectMap.end())
        {
            return it->second;
        }
        return {0.f, -1};
    }

    // ------------------ Object file I/O (preserving order) ------------------
    void loadObjectsFromFile(const std::string &filename)
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            Log::error << "Failed to open objects file: " << filename << std::endl;
            return;
        }
        objectMap.clear();
        objectOrder.clear();
        std::string line;
        while (std::getline(file, line))
        {
            // Skip empty lines
            if (line.empty())
                continue;
            // Remove carriage return if present
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            // Split by comma
            std::vector<std::string> tokens;
            std::stringstream ss(line);
            std::string token;
            while (std::getline(ss, token, ','))
            {
                tokens.push_back(token);
            }
            if (tokens.size() != 4)
            {
                Log::warn << "Invalid object line (expected 4 tokens): " << line << std::endl;
                continue;
            }
            try
            {
                float x = std::stof(tokens[0]);
                float y = std::stof(tokens[1]);
                int index = std::stoi(tokens[2]);
                float scale = std::stof(tokens[3]);
                std::pair<float, float> key = {x, y};
                objectMap[key] = {scale, index};
                objectOrder.push_back(key);
            }
            catch (const std::exception &e)
            {
                Log::warn << "Failed to parse object line: " << line << " - " << e.what() << std::endl;
            }
        }
        Log::info << "Loaded " << objectMap.size() << " objects from " << filename << std::endl;
    }

    void saveObjectsToFile(const std::string &filename)
    {
        std::ofstream file(filename);
        if (!file.is_open())
            return;
        // Save in insertion order
        for (const auto &key : objectOrder)
        {
            auto it = objectMap.find(key);
            if (it == objectMap.end())
                continue;
            const auto &props = it->second;
            file << key.first << "," << key.second << "," << props.index << "," << props.scale << "\n";
        }
    }
}

void Terrain::setTileVector(int x, int y, const std::vector<int>& tiles) {
    if (tiles.empty()) {
        terrain.erase({x, y + 1});
    } else {
        terrain[{x, y + 1}] = tiles;
    }
}

bool Terrain::isSolidTile(int x, int y) {
    if (Tiles::isSolidTile(terrain, x, y)) {
        return true;
    }
    return false;
}
/* map\terrain.cpp */
#include <debug/logs.hpp>
#include <core/scale.hpp>
#include <map/terrain.hpp>
#include <graphics/tiles.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <fstream>
#include <sstream>
#include <core/constants.hpp>
#include <algorithm> // for std::find
#include <core/tile_encoding.hpp>
#include <entities/characters.hpp>
#include <map/map_manager.hpp>
namespace Terrain
{

    const SolidMap &getSolidMap() { return MapManager::get().getData().solids; }

    void setSolidTile(int x, int y, int type)
    {
        auto& solidMap = MapManager::get().getData().solids;
        if (type <= 0)
            solidMap.erase({x, y});
        else
            solidMap[{x, y}] = type;
    }

    void eraseSolidTile(int x, int y)
    {
        auto& solidMap = MapManager::get().getData().solids;
        solidMap.erase({x, y});
    }

    int getSolidTile(int x, int y)
    {
        auto& solidMap = MapManager::get().getData().solids;
        auto it = solidMap.find({x, y});
        return (it != solidMap.end()) ? it->second : 0;
    }

    const SpawnMap &getSpawnMap() { return MapManager::get().getData().spawns; }

    void setSpawn(float x, float y, const SpawnProps &props)
    {
        auto& spawnMap = MapManager::get().getData().spawns;
        auto key = std::make_pair(x, y);
        spawnMap[key] = props;
    }

    void eraseSpawn(float x, float y)
    {
        auto& spawnMap = MapManager::get().getData().spawns;
        spawnMap.erase({x, y});
    }

    SpawnProps getSpawn(float x, float y)
    {
        auto& spawnMap = MapManager::get().getData().spawns;
        auto it = spawnMap.find({x, y});
        return (it != spawnMap.end()) ? it->second : SpawnProps{};
    }

    sf::Vector2f getPlayerSpawnPosition() { return MapManager::get().getData().playerSpawnPos; }

    void clearSpawns() { MapManager::get().getData().spawns.clear(); }

    void setPlayerSpawnPosition(const sf::Vector2f &pos)
    {
        auto& playerSpawnPosition = MapManager::get().getData().playerSpawnPos;
        playerSpawnPosition = pos;
    }

    bool isSolidTile(int x, int y)
    {
        if (getSolidTile(x, y) > 0)
            return true;
        return false;
    }

    std::vector<int> getTile(int x, int y)
    {
        auto& terrain = MapManager::get().getData().tiles;
        auto it = terrain.find({x, y});
        if (it != terrain.end())
        {
            return it->second;
        }
        return {};
    }

    void setTile(int x, int y, int encodedTile)
    {
        auto& terrain = MapManager::get().getData().tiles;
        if (encodedTile < 0 || getTileIndex(encodedTile) < 0)
        {
            terrain.erase({x, y});
        }
        else
        {
            terrain[{x, y}] = {encodedTile};
        }
    }

    void addTile(int x, int y, int encodedTile)
    {
        auto& terrain = MapManager::get().getData().tiles;
        auto &vec = terrain[{x, y}];
        if (encodedTile < 0 || getTileIndex(encodedTile) < 0)
        {
            vec.erase(std::remove(vec.begin(), vec.end(), encodedTile), vec.end());
        }
        else
        {
            vec.push_back(encodedTile);
        }
    }

    void setTileVector(int x, int y, const std::vector<int> &encodedTiles)
    {
        auto& terrain = MapManager::get().getData().tiles;
        if (encodedTiles.empty())
        {
            terrain.erase({x, y});
        }
        else
        {
            terrain[{x, y}] = encodedTiles;
        }
    }

    // ------------------ Tile functions (unchanged) ------------------
    void draw(sf::RenderWindow &win, float dt)
    {
        auto& terrain = MapManager::get().getData().tiles;
        for (const auto &[pos, tile_ids] : terrain)
        {
            for (int encoded : tile_ids)
            {
                int idx = getTileIndex(encoded);
                int rot = getTileRotation(encoded);
                int flip = getTileFlip(encoded);
                sf::Sprite s = Tiles::getTileSprite(idx);
                auto [x, y] = pos;
                sf::FloatRect bounds = s.getLocalBounds();
                s.setOrigin({bounds.size.x / 2.f, bounds.size.y / 2.f});
                sf::Vector2f tilePos = Tiles::getTilePosition(x, y);
                s.setPosition(tilePos + sf::Vector2f(bounds.size.x / 2.f * Scale::get(),
                                                     bounds.size.y / 2.f * Scale::get()));
                s.setScale(Scale::getVec());
                s.setRotation(sf::Angle(sf::degrees(rot * 90.f)));
                if (flip & 1)
                    s.scale({-1.f, 1.f});
                if (flip & 2)
                    s.scale({1.f, -1.f});
                win.draw(s);
            }
        }

        const auto& objectOrder = MapManager::get().getData().objectOrder;
        const auto& objectMap = MapManager::get().getData().objects;
        // Draw objects in insertion order (oldest first, newest last = on top)
        for (const auto &key : objectOrder)
        {
            auto it = objectMap.find(key);
            if (it == objectMap.end())
                continue; // safety
            const auto &props = it->second;
            sf::Sprite s = Objects::getObjectSprite(props.index);
            sf::FloatRect bounds = s.getLocalBounds();
            sf::Vector2f worldPos(key.first * Scale::get(), key.second * Scale::get());
            s.setOrigin({bounds.size.x / 2.f, bounds.size.y / 2.f});
            s.setPosition(worldPos);
            s.setScale(Scale::getVec() * props.scale);
            s.setRotation(sf::Angle(sf::degrees(props.rotation)));
            if (props.flipX)
                s.scale({-1.f, 1.f});
            if (props.flipY)
                s.scale({1.f, -1.f});
            win.draw(s);
        }
    }

    void setMap(const TileMap &new_map)
    {
        MapManager::get().getData().tiles = new_map;
    }

    const TileMap &getMap()
    {
        return MapManager::get().getData().tiles;
    }

    void update(float dt)
    {
        // Not used
    }

    void eraseTile(int x, int y)
    {
        auto& terrain = MapManager::get().getData().tiles;
        terrain.erase({x, y});
    }

    // ------------------ Object management (with order) ------------------
    const ObjectMap &getObjectMap()
    {
        return MapManager::get().getData().objects;
    }

    const std::vector<std::pair<float, float>> &getObjectOrder()
    {
        return MapManager::get().getData().objectOrder;
    }

    void setObject(float x, float y, const ObjectProps &props)
    {
        auto& objectMap = MapManager::get().getData().objects;
        auto& objectOrder = MapManager::get().getData().objectOrder;
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
        auto& objectMap = MapManager::get().getData().objects;
        auto& objectOrder = MapManager::get().getData().objectOrder;
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
        auto& objectMap = MapManager::get().getData().objects;
        auto it = objectMap.find({x, y});
        if (it != objectMap.end())
        {
            return it->second;
        }
        return {0.f, -1};
    }

}

void Terrain::clearObjects() {
    auto& objectMap = MapManager::get().getData().objects;
    auto& objectOrder = MapManager::get().getData().objectOrder;
    objectMap.clear();
    objectOrder.clear();
}

void Terrain::clearSolidMap() {
    auto& solidMap = MapManager::get().getData().solids;
    solidMap.clear();
}
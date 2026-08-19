#pragma once
#include <map>
#include <vector>
#include <utility>
#include <optional>
#include <SFML/System/Vector2.hpp>
#include <graphics/tiles.hpp>
#include <map/objects.hpp>
#include <map/types.hpp>
#include <unordered_set>


class MapData {
public:
    // ---- Tile map (grid -> stack of encoded tile IDs) ----
    TileMap tiles;   // std::map<std::pair<int,int>, std::vector<int>>

    // ---- Solid tiles (grid -> 1 if solid) ----
    std::map<std::pair<int,int>, int> solids;

    // ---- Objects (floating point world coords) ----
    ObjectMap objects;   // unordered_map<pair<float,float>, ObjectProps>
    std::vector<std::pair<float,float>> objectOrder; // insertion order

    // ---- Spawns (floating point coords) ----
    std::map<std::pair<float,float>, SpawnProps> spawns;
    sf::Vector2f playerSpawnPos = {0.f, 0.f};

    // ---- Inspectables (pos -> clueId) ----
    std::map<std::pair<float,float>, std::string> inspectables;

    // ---- Transitions (trigger positions) ----
    std::vector<Transition> transitions;

    std::vector<CutsceneTrigger> cutsceneTriggers;
    std::unordered_set<std::string> triggeredCutscenes; // IDs already activated

    // ---- I/O ----
    bool loadFromDirectory(const std::string& mapDir);
    bool saveToDirectory(const std::string& mapDir) const;

    void clear();
};
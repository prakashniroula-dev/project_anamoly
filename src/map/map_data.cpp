#include <map/terrain.hpp>        // for legacy? no, we'll just parse
#include <debug/logs.hpp>
#include <fstream>
#include <sstream>
#include <core/tile_encoding.hpp>
#include <entities/npc_manager.hpp>
#include "map_data.hpp"

bool MapData::loadFromDirectory(const std::string& mapDir) {
    clear();
    bool success = true;

    auto loadFile = [&](const std::string& filename, auto loader) {
        std::ifstream file(mapDir + "/" + filename);
        if (!file.is_open()) {
            Log::warn << "Could not open " << filename << " in " << mapDir << "\n";
            return false;
        }
        loader(file);
        return true;
    };

    // 1. Tiles
    loadFile("map.txt", [&](std::ifstream& f) {
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            std::istringstream iss(line);
            int x, y, id;
            char comma1, comma2;
            if (iss >> x >> comma1 >> y >> comma2 >> id && comma1 == ',' && comma2 == ',') {
                tiles[{x,y}].push_back(id);
            }
        }
    });

    // 2. Solids
    loadFile("solid_tiles.txt", [&](std::ifstream& f) {
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            std::istringstream iss(line);
            int x, y, type;
            char comma1, comma2;
            if (iss >> x >> comma1 >> y >> comma2 >> type && comma1 == ',' && comma2 == ',') {
                solids[{x,y}] = type;
            }
        }
    });

    // 3. Objects
    loadFile("objects.txt", [&](std::ifstream& f) {
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            if (line.back() == '\r') line.pop_back();
            std::vector<std::string> tokens;
            std::stringstream ss(line);
            std::string token;
            while (std::getline(ss, token, ',')) tokens.push_back(token);
            if (tokens.size() != 4 && tokens.size() != 7) continue;
            float x = std::stof(tokens[0]);
            float y = std::stof(tokens[1]);
            int idx = std::stoi(tokens[2]);
            float scale = std::stof(tokens[3]);
            ObjectProps props;
            props.scale = scale;
            props.index = idx;
            if (tokens.size() == 7) {
                props.rotation = std::stof(tokens[4]);
                props.flipX = (std::stoi(tokens[5]) != 0);
                props.flipY = (std::stoi(tokens[6]) != 0);
            }
            auto key = std::make_pair(x,y);
            objects[key] = props;
            objectOrder.push_back(key);
        }
    });

    // 4. Spawns
    loadFile("spawns.txt", [&](std::ifstream& f) {
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty() || line[0] == '#') continue;
            if (line.back() == '\r') line.pop_back();
            std::istringstream iss(line);
            std::vector<std::string> tokens;
            std::string token;
            while (iss >> token) tokens.push_back(token);
            if (tokens.size() != 11) continue;
            float x = std::stof(tokens[0]);
            float y = std::stof(tokens[1]);
            SpawnProps props;
            props.characterKey = tokens[2];
            props.scale = std::stof(tokens[3]);
            props.rotation = std::stof(tokens[4]);
            props.flipX = (std::stoi(tokens[5]) != 0);
            props.flipY = (std::stoi(tokens[6]) != 0);
            props.npcTypeId = tokens[7];
            props.uniqueID = (tokens[8] == "_") ? "" : tokens[8];
            props.scriptName = (tokens[9] == "_") ? "" : tokens[9];
            // waypoints: tokens[10] is "x1,y1;x2,y2;..."
            if (tokens[10] != "_") {
                std::stringstream wss(tokens[10]);
                std::string pair;
                while (std::getline(wss, pair, ';')) {
                    if (pair.empty()) continue;
                    float wx, wy; char comma;
                    std::stringstream ps(pair);
                    if (ps >> wx >> comma >> wy && comma == ',') {
                        props.waypoints.push_back({wx, wy});
                    }
                }
            }
            if (props.npcTypeId == "player") playerSpawnPos = {x, y};
            spawns[{x,y}] = props;
        }
    });

    // 5. Inspectables
    loadFile("inspectables.txt", [&](std::ifstream& f) {
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream iss(line);
            float x, y; std::string clueId;
            if (iss >> x >> y >> clueId) {
                inspectables[{x,y}] = clueId;
            }
        }
    });

    loadFile("cutscenes.txt", [&](std::ifstream& f) {
      std::string line;
      while (std::getline(f, line)) {
          if (line.empty() || line[0] == '#') continue;
          std::istringstream iss(line);
          std::string id, scriptName, npcId;
          float x, y, radius;
          if (iss >> id >> x >> y >> radius >> scriptName >> npcId) {
              CutsceneTrigger trigger;
              trigger.id = id;
              trigger.position = {x, y};
              trigger.radius = radius;
              trigger.scriptName = scriptName;
              trigger.npcId = npcId;
              cutsceneTriggers.push_back(trigger);
          }
      }
  });

    // 6. Transitions
  loadFile("transitions.txt", [&](std::ifstream& f) {
    // line format: x y target label [condition [action [failMessage]]]
    // For backward compatibility, fields after 'label' are optional.
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        float x, y, toX, toY;
        std::string target, label, condition, action, failMsg;
        if (iss >> x >> y >> target >> label) {
            Transition tr;
            tr.triggerPos = {x, y};
            tr.targetMap = target;
            tr.label = label;
            tr.condition = "";
            tr.action = "";
            tr.failMessage = "";
            if ( iss >> toX && iss >> toY ) {
              tr.spawnPosition = sf::Vector2f(toX, toY);
            }
            if (iss >> condition) {
              if (condition != "_") {
                Log::info << "Transition condition: " << condition << "\n";
                tr.condition = condition;
              }
            };
            if (iss >> action) {
              if (action != "_")
                tr.action = action;
            };
            if (iss >> failMsg) {
              if (failMsg == "\"") {
                std::string word;
                while (iss >> word) {
                  if (word == "\"") break;
                  if (failMsg != "\"") {
                    failMsg += " ";
                    failMsg += word;
                  } else {
                    failMsg = word;
                  }
                }
                tr.failMessage = failMsg;
              } else if (failMsg != "_") {
                tr.failMessage = failMsg;
              }
              Log::info << "Transition failMessage: " << tr.failMessage << "\n";
            }
            // optional spawn position (existing logic)
            float x2, y2;
            if (iss >> x2 >> y2) tr.spawnPosition = sf::Vector2f(x2, y2);
            transitions.push_back(tr);
        }
    }
  });

    return success;
}

bool MapData::saveToDirectory(const std::string& mapDir) const {
    auto saveFile = [&](const std::string& filename, auto writer) {
        std::ofstream file(mapDir + "/" + filename);
        if (!file.is_open()) {
            Log::error << "Could not write " << filename << "\n";
            return false;
        }
        writer(file);
        return true;
    };


    // saveFile("cutscenes.txt", [&](std::ofstream& f) {
    //     for (const auto& tr : cutsceneTriggers) {
    //         f << tr.id << " " 
    //           << tr.position.x << " " << tr.position.y << " "
    //           << tr.radius << " "
    //           << tr.scriptName << " "
    //           << tr.npcId << "\n";
    //     }
    // });

    saveFile("map.txt", [&](std::ofstream& f) {
        for (const auto& [pos, ids] : tiles) {
            for (int id : ids) {
                f << pos.first << "," << pos.second << "," << id << "\n";
            }
        }
    });

    saveFile("solid_tiles.txt", [&](std::ofstream& f) {
        for (const auto& [pos, type] : solids) {
            f << pos.first << "," << pos.second << "," << type << "\n";
        }
    });

    saveFile("objects.txt", [&](std::ofstream& f) {
        for (const auto& key : objectOrder) {
            auto it = objects.find(key);
            if (it == objects.end()) continue;
            const auto& p = it->second;
            f << key.first << "," << key.second << "," << p.index << "," << p.scale
              << "," << p.rotation << "," << (p.flipX?1:0) << "," << (p.flipY?1:0) << "\n";
        }
    });

    saveFile("spawns.txt", [&](std::ofstream& f) {
        f << "# x y charKey scale rot flipX flipY npcTypeId uniqueID scriptName waypoints\n";
        for (const auto& [pos, props] : spawns) {
            std::string wpStr;
            for (size_t i=0; i<props.waypoints.size(); ++i) {
                if (i) wpStr += ";";
                wpStr += std::to_string(props.waypoints[i].x) + "," + std::to_string(props.waypoints[i].y);
            }
            if (wpStr.empty()) wpStr = "_";
            std::string uid = props.uniqueID.empty() ? "_" : props.uniqueID;
            std::string scr = props.scriptName.empty() ? "_" : props.scriptName;
            f << pos.first << " " << pos.second << " "
              << props.characterKey << " "
              << props.scale << " "
              << props.rotation << " "
              << (props.flipX?1:0) << " "
              << (props.flipY?1:0) << " "
              << props.npcTypeId << " "
              << uid << " "
              << scr << " "
              << wpStr << "\n";
        }
    });

    saveFile("inspectables.txt", [&](std::ofstream& f) {
        for (const auto& [pos, clueId] : inspectables) {
            f << pos.first << " " << pos.second << " " << clueId << "\n";
        }
    });

    saveFile("transitions.txt", [&](std::ofstream& f) {
        for (const auto& tr : transitions) {
            f << tr.triggerPos.x << " " << tr.triggerPos.y << " "
              << tr.targetMap << " " << tr.label;
            if (!tr.condition.empty() || !tr.action.empty() || !tr.failMessage.empty()) {
                f << " " << tr.condition << " " << tr.action << " " << tr.failMessage;
            }
            if (tr.spawnPosition) {
                f << " " << tr.spawnPosition->x << " " << tr.spawnPosition->y;
            }
            f << "\n";
        }
    });

    return true;
}

void MapData::clear() {
    tiles.clear();
    solids.clear();
    objects.clear();
    objectOrder.clear();
    spawns.clear();
    inspectables.clear();
    transitions.clear();
    playerSpawnPos = {0.f,0.f};
}
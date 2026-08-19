#include <map/objects.hpp>
#include <graphics/textures.hpp>
#include <SFML/Graphics.hpp>
#include <unordered_map>
namespace Objects
{
  namespace {
    static int totalObjects = 0;

    std::string getObjectKey(int index) {
      return "object_" + std::to_string(index + 1);
    }

    std::unordered_map<int, std::string> specialObjects;
  }

  void load() {
    static const std::vector<std::pair<std::string, int>> objectKeys = {
      // powerstation
        {"power_station/objects/decoration", 27},
        {"power_station/objects/tube", 11},

      // industrial
        {"industrial_zone/objects/barrel", 4},
        {"industrial_zone/objects/bench", 1},
        {"industrial_zone/objects/board", 3},
        {"industrial_zone/objects/box", 8},
        {"industrial_zone/objects/bucket", 1},
        {"industrial_zone/objects/fence", 1},
        {"industrial_zone/objects/fire-extinguisher", 3},
        {"industrial_zone/objects/flag", 1},
        {"industrial_zone/objects/ladder", 3},
        {"industrial_zone/objects/locker", 4},
        {"industrial_zone/objects/mop", 1},
        {"industrial_zone/objects/pointer", 2},
        {"industrial_zone/objects/numbers", 10},

      // exclusion zone
        {"exclusion_zone/objects/grass", 24},
        {"exclusion_zone/objects/stones", 6},
        {"exclusion_zone/objects/trees", 18},
        {"exclusion_zone/objects/other/box", 4},
        {"exclusion_zone/objects/other/pointer", 3},

      // factory
        {"factory/objects/barrel", 4},
        {"factory/objects/boxes", 6},
        {"factory/objects/icons", 12},
        {"factory/objects/ladders", 8},
        {"factory/objects/monitors", 9},

      // billboards
        {"billboards/billboards", 8},
        {"billboards/ads/22x40", 15},
        {"billboards/ads/64x64", 15},
        {"billboards/ads/128x64", 15},

        // vehicle bodies
        {"vehicle_body/type1", 8},
        {"vehicle_body/type2", 2},
        {"vehicle_body/type3", 4},
        {"vehicle_body/type4", 2},
        {"vehicle_body/type5", 4},
        {"vehicle_body/type6", 4},
        {"vehicle_body/type7", 2},
        {"vehicle_body/type8", 2},
        {"vehicle_body/type9", 2},
        {"vehicle_body/type10", 3},
        {"vehicle_body/type11", 1},
        {"vehicle_body/type12", 2},
        {"vehicle_body/type13", 2},

        
    };
    
    for ( const auto& [path, count] : objectKeys) {
      for (int i = 0; i < count; ++i) {
        std::string key = getObjectKey(totalObjects);
        std::string fullPath = path + "/" + std::to_string(i + 1) + ".png";
        Textures::load(key, fullPath);
        totalObjects++;
      }
    }

    /* Special objects */
    std::string path = "vehicle_tires/1.png";
    Textures::load(getObjectKey(totalObjects), path);
    specialObjects[totalObjects++] = "vehicle_tires";

    path = "vehicle_tires/2.png";
    Textures::load(getObjectKey(totalObjects), path);
    specialObjects[totalObjects++] = "vehicle_tires";
  }

  sf::Sprite getObjectSprite(int index) {
    std::string key = getObjectKey(index);
    if (specialObjects.find(index) != specialObjects.end()) {
      std::string type = specialObjects[index];
      if (type == "vehicle_tires") {
        sf::Sprite s(Textures::get(key));
        s.setTextureRect(sf::IntRect({0,0},{48,48})); // Use only the first 48x48
        return s;
      }
    }
    return sf::Sprite(Textures::get(key));
  }

  int getCount() {
    return totalObjects;
  }
}
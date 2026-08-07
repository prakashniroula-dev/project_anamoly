#include <entities/objects.hpp>
#include <graphics/textures.hpp>
#include <SFML/Graphics.hpp>

namespace Objects
{
  namespace {
    static int totalObjects = 0;

    std::string getObjectKey(int index) {
      return "object_" + std::to_string(index + 1);
    }
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
        {"industrial_zone/objects/pointer", 2}
    };
    
    for ( const auto& [path, count] : objectKeys) {
      for (int i = 0; i < count; ++i) {
        std::string key = getObjectKey(totalObjects);
        std::string fullPath = path + "/" + std::to_string(i + 1) + ".png";
        Textures::load(key, fullPath);
        totalObjects++;
      }
    }
  }

  sf::Sprite getObjectSprite(int index) {
    std::string key = getObjectKey(index);
    return sf::Sprite(Textures::get(key));
  }

  int getCount() {
    return totalObjects;
  }
}
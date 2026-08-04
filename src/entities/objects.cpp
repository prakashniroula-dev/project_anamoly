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
    static const std::pair<std::string, int> decorations = {"power_station/objects/decoration/", 27};
    static const std::pair<std::string, int> tubes = {"power_station/objects/tube/", 11};
    
    for (int i = 0; i < decorations.second; ++i) {
      std::string key = getObjectKey(totalObjects++);
      std::string path = decorations.first + std::to_string(i+1) + ".png";
      Textures::load(key, path);
    }
    
    for (int i = 0; i < tubes.second; ++i) {
      std::string key = getObjectKey(totalObjects++);
      std::string path = tubes.first + std::to_string(i+1) + ".png";
      Textures::load(key, path);
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
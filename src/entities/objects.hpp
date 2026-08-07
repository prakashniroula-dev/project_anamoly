#pragma once

#include <graphics/textures.hpp>
#include <SFML/Graphics.hpp>
#include <map>

struct ObjectProps {
  float scale;
  int index;
  float rotation = 0.f;   // degrees
  bool flipX = false;
  bool flipY = false;
};

#include <unordered_map>
#include <utility>
#include <functional>

namespace std {
  // to fix unordered_map with std::pair<float, float> as key
    template<>
    struct hash<std::pair<float, float>> {
        std::size_t operator()(const std::pair<float, float>& p) const {
            // Combine the hash of both floats
            auto h1 = std::hash<float>{}(p.first);
            auto h2 = std::hash<float>{}(p.second);
            // Simple combine (XOR with shift)
            return h1 ^ (h2 << 1);
        }
    };
}

using ObjectMap = std::unordered_map<std::pair<float, float>, ObjectProps>;

namespace Objects {
  void load();
  sf::Sprite getObjectSprite(int index);
  int getCount();   // new
}
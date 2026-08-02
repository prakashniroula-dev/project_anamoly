#pragma once
#include <string>
#include <map>
#include <unordered_map>
#include <SFML/Graphics.hpp>
#include <graphics/textures.hpp>

namespace Animations {
    struct Info {
        std::string textureKey;
        std::string texturePath;
        int totalFrames;
        int startFrame;
        int endFrame;
        float duration;

        Info();
        Info(std::string texture_p, int total_f, int dur);
        Info(std::string texture_k, std::string texture_p, int total_f, int dur, int start_f, int end_f);
    };

    // Just declare the functions. NO GLOBAL MAP HERE.
    void add(std::string key, Info anim);
    void addList(std::string baseKey, std::string baseTexturePath, std::map<std::string, Info> map);
    Info get(std::string key);
}
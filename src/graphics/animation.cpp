#include <graphics/animation.hpp>
#include <graphics/textures.hpp>
#include <debug/logs.hpp> // Update your include path

namespace Animations {
    // This map now lives in ONE place: this .cpp file. 
    // No static, no anonymous namespace. It is private to this translation unit.
    std::unordered_map<std::string, Info> animations; 

    // Implement your constructors and functions here
    Info::Info() : textureKey(""), texturePath(""), totalFrames(0), startFrame(0), endFrame(0), duration(0.f) {}
    Info::Info(std::string texture_p, int total_f, int dur) : texturePath(texture_p), totalFrames(total_f), duration(dur) {
        startFrame = 0; endFrame = total_f - 1; textureKey = texture_p;
    }
    Info::Info(std::string texture_k, std::string texture_p, int total_f, int dur, int start_f, int end_f)
        : texturePath(texture_p), textureKey(texture_k), totalFrames(total_f), duration(dur), startFrame(start_f), endFrame(end_f) {}

    void add(std::string key, Info anim) { animations[key] = anim; }

    void addList(std::string baseKey, std::string baseTexturePath, std::map<std::string, Info> map) {
        for (const auto& [animKey, info] : map) {
            Textures::load(baseKey + animKey, baseTexturePath + info.texturePath);
            add(baseKey + animKey, Info(baseKey + animKey, baseTexturePath + info.texturePath, info.totalFrames, info.duration, info.startFrame, info.endFrame));
        }
    }

    Info get(std::string key) {
        auto it = animations.find(key);
        if (it != animations.end()) {
            return it->second;
        }
        static Log::Scope scope("Animations::get()");
        scope.error << "Animation key '" << key << "' not found. Returning dummy.\n";
        return Info();   // empty/default Info
    }
}
// src/graphics/Textures.cpp
#include <debug/logs.hpp>   // Updated path!
#include <unordered_map>
#include <memory>
#include <exception>
#include <graphics/textures.hpp>          // Our new header

namespace Textures {

    // The base path is now safely hidden here.
    const std::filesystem::path base_path = "assets/";

    // THIS is the ONE and ONLY map. No 'static inline' in a header.
    // By putting it inside this .cpp file, every call to load() and get() 
    // will use THIS EXACT SAME map.
    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> texture_maps;

    void load(const std::string& key, const std::filesystem::path& path) {
        static Log::Scope scope("Textures::load()"); 
        
        if (texture_maps.count(key)) {
            scope.warn << "multiple calls for texture key '" << key << "'\n";
            return;
        }
        try {
            texture_maps[key] = std::make_unique<sf::Texture>(base_path / path);
        } catch (const std::exception& e) {
            scope.error << "failed to load texture: " << e.what() << "\n";
        }
    }
    
    sf::Texture& get(const std::string& key) {
        static Log::Scope scope("Textures::get()");
        try {
            return *texture_maps.at(key);
        } catch (const std::exception& e) {
            scope.error << "No such texture with key '" << key << "' loaded\n";
            static sf::Texture empty; // This static local is fine, it's just a fallback.
            return empty;
        }
    }
}
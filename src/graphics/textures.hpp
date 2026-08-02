// src/graphics/Textures.hpp
#pragma once
#include <string>
#include <filesystem>
#include <SFML/Graphics/Texture.hpp> // We need the full type for the return reference

namespace Textures {
    // We only DECLARE the functions. No map, no static inline!
    void load(const std::string& key, const std::filesystem::path& path);
    sf::Texture& get(const std::string& key);
}
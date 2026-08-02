// src/core/constants.hpp
#pragma once

namespace Constants {
    constexpr int TILE_SIZE = 32;
    constexpr int WINDOW_WIDTH = 800;  // you can adjust
    constexpr int WINDOW_HEIGHT = 600; // you can adjust
    constexpr int WORLD_WIDTH_TILES = WINDOW_WIDTH * 2 / 32;   // you can adjust
    constexpr int WORLD_HEIGHT_TILES = 10;
    constexpr int WORLD_WIDTH_PIXELS = TILE_SIZE * WORLD_WIDTH_TILES;
    constexpr int WORLD_HEIGHT_PIXELS = TILE_SIZE * WORLD_HEIGHT_TILES;
    constexpr float MAX_STEP_HEIGHT = 16.f; // max height character can step up
}
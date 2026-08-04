// src/core/constants.hpp
#pragma once

namespace Constants {
    constexpr int TILE_SIZE = 32;
    constexpr int WINDOW_WIDTH = 800;  // you can adjust
    constexpr int WINDOW_HEIGHT = 600; // you can adjust
    constexpr int WORLD_WIDTH_TILES = 2000;   // you can adjust
    constexpr int VIEW_HEIGHT_TILES = 10;   // number of tiles visible vertically in the view
    constexpr int WORLD_HEIGHT_TILES = 20;
    constexpr int WORLD_WIDTH_PIXELS = TILE_SIZE * WORLD_WIDTH_TILES;
    constexpr int WORLD_HEIGHT_PIXELS = TILE_SIZE * WORLD_HEIGHT_TILES;
    constexpr float MAX_STEP_HEIGHT = 16.f; // max height character can step up
    const float GRAVITY        = 1200.0f;   // was likely 400–600
    const float JUMP_SPEED     = -550.0f;   // negative means up
    const float MAX_FALL_SPEED = 1200.0f;   // cap terminal velocity
    const float WALK_SPEED     = 300.0f;
    const float RUN_SPEED      = 500.0f;
}
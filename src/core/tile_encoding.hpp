#pragma once

#include <string>

// Tile ID encoding:
// Bits 0-7   : tile index (low byte)
// Bits 8-9   : rotation (0–3) = 0°, 90°, 180°, 270°
// Bits 10-11 : flip (0–3) = 0:none, 1:horizontal, 2:vertical, 3:both
// Bits 12-23 : tile index (high 12 bits) – supports up to 2^20 tiles

inline int encodeTile(int index, int rotation, int flip) {
    int low = index & 0xFF;          // lower 8 bits
    int high = (index >> 8) & 0xFFF; // next 12 bits (up to 4095)
    return low | (rotation << 8) | (flip << 10) | (high << 12);
}

inline int getTileIndex(int encoded) {
    int low = encoded & 0xFF;
    int high = (encoded >> 12) & 0xFFF;
    return low | (high << 8);
}

inline int getTileRotation(int encoded) {
    return (encoded >> 8) & 0x3;
}

inline int getTileFlip(int encoded) {
    return (encoded >> 10) & 0x3;
}

// For debugging: print human‑readable representation
inline std::string tileToString(int encoded) {
    return std::to_string(getTileIndex(encoded)) + 
           " (rot=" + std::to_string(getTileRotation(encoded) * 90) + 
           "°, flip=" + std::to_string(getTileFlip(encoded)) + ")";
}
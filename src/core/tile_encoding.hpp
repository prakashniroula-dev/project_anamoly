#pragma once

#include <string>
// Tile ID encoding:
// Bits 0-7   : tile index (0–255)
// Bits 8-9   : rotation (0–3) = 0°, 90°, 180°, 270°
// Bits 10-11 : flip (0–3) = 0:none, 1:horizontal, 2:vertical, 3:both
inline int encodeTile(int index, int rotation, int flip) {
    return index | (rotation << 8) | (flip << 10);
}

inline int getTileIndex(int encoded) { return encoded & 0xFF; }
inline int getTileRotation(int encoded) { return (encoded >> 8) & 0x3; }
inline int getTileFlip(int encoded) { return (encoded >> 10) & 0x3; }

// For debugging: print human‑readable representation
inline std::string tileToString(int encoded) {
    return std::to_string(getTileIndex(encoded)) + 
           " (rot=" + std::to_string(getTileRotation(encoded) * 90) + 
           "°, flip=" + std::to_string(getTileFlip(encoded)) + ")";
}
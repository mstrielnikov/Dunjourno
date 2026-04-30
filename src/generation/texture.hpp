#pragma once
#include <generation/generation.hpp>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace generation {

/**
 * @brief Terrain Texture Generator
 *
 * This pass computes the slope/steepness at each cell and uses it alongside
 * elevation to determine the terrain's biome/color (e.g., grass, rock, snow).
 * This establishes the base "texture" for the 3D continuous mesh.
 */
struct TerrainTextureGenerator {
    TerrainTextureGenerator() {
        std::cout << "[INIT] TerrainTextureGenerator ready.\n";
    }

    std::expected<void, GenError> apply(Grid& grid) {
        std::cout << "[WORK] TerrainTextureGenerator assigning colors...\n";

        int w = (int)grid.width();
        int h = (int)grid.height();

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                Cell& cell = grid(x, y);

                // Compute local steepness by sampling neighbors
                float dx = 0.0f;
                float dy = 0.0f;
                if (x > 0 && x < w - 1) {
                    dx = (grid(x + 1, y).height - grid(x - 1, y).height) * 0.5f;
                }
                if (y > 0 && y < h - 1) {
                    dy = (grid(x, y + 1).height - grid(x, y - 1).height) * 0.5f;
                }
                float steepness = std::sqrt(dx * dx + dy * dy);

                // Default colors
                uint8_t r = 60, g = 140, b = 60; // Base Grass

                if (cell.terrain == TerrainType::Tree) {
                    // Darker green for trees
                    r = 30; g = 100; b = 30;
                } else if (cell.terrain == TerrainType::Mountain) {
                    // For mountains, color depends on steepness and height
                    if (steepness > 0.05f) {
                        // Steep rock: Grey
                        r = 100; g = 100; b = 105;
                    } else if (cell.height > 1.8f) { // If peak is ~3.0, >1.8 is high
                        // Flat high ground: Snow
                        r = 240; g = 240; b = 250;
                    } else {
                        // Flat lower mountain ground: Highland grass / dirt
                        r = 90; g = 130; b = 80;
                    }
                } else if (cell.terrain == TerrainType::Grass) {
                    // Optional: Dirt patches based on steepness even in grass
                    if (steepness > 0.08f) {
                        r = 139; g = 115; b = 85; // Dirt
                    }
                }

                cell.r = r;
                cell.g = g;
                cell.b = b;
            }
        }

        return {};
    }

    std::string get_name() const { return "TerrainTexture"; }
};

} // namespace generation

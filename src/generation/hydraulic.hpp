#pragma once
#include <generation/generation.hpp>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <random>
#include <vector>

// Include for noise::fbm_2d (defined in mountain.hpp noise namespace)
#include "mountain.hpp"

namespace generation {

/**
 * @brief Hydraulic Terrain Generator
 *
 * Replaces MountainPlacement + MountainRidge + ThermalErosion with a single
 * physically-based layer using Newtonian droplet simulation.
 *
 * Based on the approach from weigert/SimpleErosion:
 *   - Droplets have 2D velocity vectors (not just direction)
 *   - Acceleration from surface normal, damped by friction
 *   - Unified deposition formula: dt * depositionRate * (capacity - sediment)
 *   - Density gives droplets physical mass for realistic inertia
 *
 *   Phase 1: Generate initial heightmap using multi-octave 2D fBm hash noise
 *   Phase 2: Simulate water droplets eroding and depositing sediment
 *   Phase 3: Classify terrain and assign roughness from erosion patterns
 */
struct HydraulicTerrainGenerator {
    // Initial heightmap parameters
    float noise_scale    = 0.02f;   // Controls feature size (lower = larger mountains)
    int   noise_octaves  = 6;       // fBm octaves
    float base_height    = 3.0f;    // Maximum initial height

    // Erosion simulation parameters (tuned from SimpleErosion defaults)
    int   num_droplets       = 70000;   // Number of water droplets to simulate
    float dt                 = 1.2f;    // Integration timestep (higher = more erosion)
    float friction           = 0.05f;   // Speed loss factor per timestep
    float density            = 1.0f;    // Droplet density (mass = volume * density)
    float deposition_rate    = 0.1f;    // Rate of approach to equilibrium sediment
    float evaporation_rate   = 0.01f;   // Volume loss per step [0,1]
    float min_volume         = 0.01f;   // Volume below which droplet dies

    // Terrain classification
    float mountain_threshold = 0.3f;    // Height above which cells become Mountain

    uint32_t seed = 42;

    HydraulicTerrainGenerator() {
        seed = std::random_device{}();
        std::cout << "[INIT] HydraulicTerrainGenerator seed=" << seed << "\n";
    }

    std::expected<void, GenError> apply(Grid& grid) {
        int w = (int)grid.width();
        int h = (int)grid.height();
        std::cout << "[WORK] HydraulicTerrainGenerator on " << w << "x" << h << " grid\n";

        // ═══════════════════════════════════════════════════════════════
        // Phase 1: Generate initial heightmap with 2D fBm
        // ═══════════════════════════════════════════════════════════════
        std::cout << "  Phase 1: Generating initial heightmap...\n";

        std::vector<float> heightmap(w * h, 0.0f);

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                float nx = (float)x * noise_scale;
                float ny = (float)y * noise_scale;
                float val = noise::fbm_2d(nx, ny, seed, noise_octaves, 2.0f, 0.5f);
                // Remap from [0,1] to [-0.3, 1.0] so valleys dip below the base
                val = val * 1.3f - 0.3f;
                val = std::clamp(val, 0.0f, 1.0f);
                heightmap[y * w + x] = val * base_height;
            }
        }

        // ═══════════════════════════════════════════════════════════════
        // Phase 2: Hydraulic erosion — Newtonian droplet simulation
        // ═══════════════════════════════════════════════════════════════
        std::cout << "  Phase 2: Simulating " << num_droplets << " water droplets...\n";

        // Track cumulative erosion/deposition per cell for roughness
        std::vector<float> erosion_map(w * h, 0.0f);

        std::mt19937 rng(seed + 1);
        std::uniform_int_distribution<int> rand_x(0, w - 1);
        std::uniform_int_distribution<int> rand_y(0, h - 1);

        // Lambda: compute surface normal at integer cell position
        // Returns (nx, ny, nz) where nx,nz are the horizontal components
        // used to accelerate the droplet, and ny is the vertical component
        auto surface_normal = [&](int x, int y) -> std::tuple<float, float, float> {
            // Central differences with boundary clamping
            float scale = 1.0f; // Cell spacing
            float hL = heightmap[y * w + std::max(x - 1, 0)];
            float hR = heightmap[y * w + std::min(x + 1, w - 1)];
            float hD = heightmap[std::max(y - 1, 0) * w + x];
            float hU = heightmap[std::min(y + 1, h - 1) * w + x];

            // Cross-product normal from height differences
            float nx = (hL - hR) / (2.0f * scale);
            float ny = 1.0f;  // Vertical
            float nz = (hD - hU) / (2.0f * scale);

            float len = std::sqrt(nx * nx + ny * ny + nz * nz);
            return {nx / len, ny / len, nz / len};
        };

        // Lambda: modify height at integer position and track erosion
        auto modify_cell = [&](int x, int y, float amount) {
            x = std::clamp(x, 0, w - 1);
            y = std::clamp(y, 0, h - 1);
            heightmap[y * w + x] += amount;
            erosion_map[y * w + x] += std::abs(amount);
        };

        for (int drop = 0; drop < num_droplets; ++drop) {
            // Spawn droplet at random integer position
            float px = (float)rand_x(rng);
            float py = (float)rand_y(rng);
            float sx = 0.0f, sy = 0.0f;  // 2D speed vector
            float volume = 1.0f;
            float sediment = 0.0f;

            while (volume > min_volume) {
                int ix = (int)px;
                int iy = (int)py;

                // Get surface normal at current cell
                auto [nx, ny, nz] = surface_normal(ix, iy);

                // Accelerate: speed += dt * normal_horizontal / (volume * density)
                float mass = volume * density;
                sx += dt * nx / mass;
                sy += dt * nz / mass;

                // Move
                px += dt * sx;
                py += dt * sy;

                // Friction
                sx *= (1.0f - dt * friction);
                sy *= (1.0f - dt * friction);

                // Bounds check
                if (px < 0 || px >= w || py < 0 || py >= h) break;

                int new_ix = (int)px;
                int new_iy = (int)py;
                new_ix = std::clamp(new_ix, 0, w - 1);
                new_iy = std::clamp(new_iy, 0, h - 1);

                // Sediment capacity = volume * |speed| * height_delta
                float speed = std::sqrt(sx * sx + sy * sy);
                float h_diff = heightmap[iy * w + ix] - heightmap[new_iy * w + new_ix];
                float max_sediment = volume * speed * std::max(h_diff, 0.0f);
                if (max_sediment < 0.0f) max_sediment = 0.0f;
                float sdiff = max_sediment - sediment;

                // Unified erosion/deposition: approach equilibrium
                sediment += dt * deposition_rate * sdiff;
                modify_cell(ix, iy, -dt * volume * deposition_rate * sdiff);

                // Evaporate
                volume *= (1.0f - dt * evaporation_rate);
            }
        }

        // ═══════════════════════════════════════════════════════════════
        // Phase 3: Write results back to grid + classify terrain
        // ═══════════════════════════════════════════════════════════════
        std::cout << "  Phase 3: Classifying terrain...\n";

        // Find max erosion for normalization
        float max_erosion = 0.01f;
        for (float e : erosion_map) max_erosion = std::max(max_erosion, e);

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                Cell& cell = grid(x, y);
                float height = std::max(heightmap[y * w + x], 0.0f);
                cell.height = height;

                // Classify terrain
                if (height > mountain_threshold) {
                    cell.terrain = TerrainType::Mountain;
                } else {
                    cell.terrain = TerrainType::Grass;
                }

                // Roughness from erosion activity, scaled by height
                float erosion_intensity = erosion_map[y * w + x] / max_erosion;
                cell.roughness = std::clamp(erosion_intensity * std::clamp(height, 0.0f, 1.0f), 0.0f, 1.0f);
            }
        }

        std::cout << "  Hydraulic erosion complete.\n";
        return {};
    }

    std::string get_name() const { return "HydraulicTerrain"; }
};

} // namespace generation

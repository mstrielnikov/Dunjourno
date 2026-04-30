#pragma once
#include <generation/generation.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <numeric>

namespace generation {

#include <stb_perlin.h>

namespace noise {
// Multi-octave fractal noise (fBm) using stb_perlin
inline float fbm(float x, float y, uint32_t seed, int octaves = 4, float lacunarity = 2.0f, float gain = 0.5f) {
    float sum = 0.0f;
    float amp = 1.0f;
    float freq = 1.0f;
    float max_amp = 0.0f;
    for (int i = 0; i < octaves; ++i) {
        // stb_perlin_noise3_seed returns [-1, 1], map it to [0, 1] for FBM consistency with previous code
        float n = stb_perlin_noise3_seed(x * freq, y * freq, 0.0f, 0, 0, 0, seed + i * 31);
        float mapped_n = (n + 1.0f) * 0.5f; 
        sum += amp * mapped_n;
        max_amp += amp;
        amp *= gain;
        freq *= lacunarity;
    }
    return sum / max_amp;
}
} // namespace noise

/**
 * @brief Mountain ridge generator using Perlin-noise perturbed spine paths
 *
 * Algorithm:
 *   1. Collect all cells currently marked as Mountain
 *   2. Pick two random mountain cells as ridge endpoints
 *   3. Walk from A to B using linear interpolation, perturbed laterally by
 *      fractal value noise to form a worm-like spine
 *   4. For every mountain cell, compute the minimum distance to the nearest
 *      spine point
 *   5. Height = peak * falloff(distance) with randomized jitter for natural
 *      edges: smooth descent from spine outward to the mountain boundary
 */
struct MountainRidgeGenerator {
    float peak_height     = 3.0f;     // Maximum height at the ridge spine
    float noise_amplitude = 8.0f;     // Lateral wander amplitude (in cells)
    float noise_frequency = 0.08f;    // Noise frequency along the spine
    int   spine_samples   = 256;      // Resolution of the sampled spine
    float falloff_power   = 2.5f;     // Exponent for falloff sharpness (higher = steeper slopes)
    float edge_jitter     = 0.20f;    // Randomized roughness at descent edges

    MountainRidgeGenerator(float peak = 3.0f, float amp = 8.0f, float freq = 0.08f, float power = 2.5f)
        : peak_height(peak), noise_amplitude(amp), noise_frequency(freq), falloff_power(power) {
        std::cout << "[INIT] MountainRidgeGenerator peak=" << peak_height
                  << " amp=" << noise_amplitude
                  << " freq=" << noise_frequency
                  << " power=" << falloff_power << "\n";
    }

    std::expected<void, GenError> apply(Grid& grid) {
        std::cout << "[WORK] MountainRidgeGenerator sculpting ridge...\n";

        std::mt19937 rng(std::random_device{}());
        uint32_t noise_seed = rng();

        // 1. Collect all mountain cell coordinates
        struct Pt { float x, y; };
        std::vector<Pt> mountain_cells;
        for (auto [x, y, cell] : grid.iter()) {
            if (cell.terrain == TerrainType::Mountain) {
                mountain_cells.push_back({ static_cast<float>(x), static_cast<float>(y) });
            }
        }

        if (mountain_cells.size() < 2) {
            std::cout << "[WARN] MountainRidgeGenerator: fewer than 2 mountain cells, skipping\n";
            return {};
        }

        // 2. Pick two random mountain cells as ridge endpoints
        std::uniform_int_distribution<size_t> pick(0, mountain_cells.size() - 1);
        Pt a = mountain_cells[pick(rng)];
        Pt b = mountain_cells[pick(rng)];

        // Ensure they're not the same point
        int attempts = 0;
        while (a.x == b.x && a.y == b.y && attempts++ < 20) {
            b = mountain_cells[pick(rng)];
        }

        std::cout << "  Spine endpoints: (" << a.x << "," << a.y << ") -> ("
                  << b.x << "," << b.y << ")\n";

        // 3. Build the worm-like spine via noise-perturbed interpolation
        //    Compute a perpendicular direction for lateral displacement
        float dx = b.x - a.x;
        float dy = b.y - a.y;
        float chord_len = std::sqrt(dx * dx + dy * dy);
        if (chord_len < 1.0f) return {}; // degenerate

        // Unit perpendicular to the chord
        float perp_x = -dy / chord_len;
        float perp_y =  dx / chord_len;

        std::vector<Pt> spine;
        spine.reserve(spine_samples);
        for (int i = 0; i <= spine_samples; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(spine_samples);
            // Linear interpolation along chord
            float cx = a.x + t * dx;
            float cy = a.y + t * dy;

            // Lateral displacement via noise — produces worm-like wander
            float n = noise::fbm(t * chord_len * noise_frequency, 0.5f, noise_seed, 4);
            float displacement = (n - 0.5f) * 2.0f * noise_amplitude;

            cx += perp_x * displacement;
            cy += perp_y * displacement;
            spine.push_back({ cx, cy });
        }

        // 4. For each mountain cell, find minimum distance to the spine
        //    and compute the maximum distance (for normalization)
        std::vector<float> min_dist(grid.width() * grid.height(), -1.0f);
        float max_mountain_dist = 0.0f;

        for (auto [x, y, cell] : grid.iter()) {
            if (cell.terrain != TerrainType::Mountain) continue;

            float best = 1e9f;
            for (const auto& sp : spine) {
                float ddx = static_cast<float>(x) - sp.x;
                float ddy = static_cast<float>(y) - sp.y;
                float d = ddx * ddx + ddy * ddy; // squared
                if (d < best) best = d;
            }
            best = std::sqrt(best);
            min_dist[y * grid.width() + x] = best;
            if (best > max_mountain_dist) max_mountain_dist = best;
        }

        if (max_mountain_dist < 0.01f) max_mountain_dist = 1.0f;

        // 5. Assign heights: peak at spine, smooth falloff to edge with jitter
        std::uniform_real_distribution<float> jitter_dist(-edge_jitter, edge_jitter);

        for (auto [x, y, cell] : grid.iter()) {
            if (cell.terrain != TerrainType::Mountain) continue;

            float d = min_dist[y * grid.width() + x];
            float norm_d = d / max_mountain_dist; // 0 = spine, 1 = furthest edge

            // Sharp falloff: cosine base raised to a power for dramatic spine prominence
            // At spine (norm_d=0): cos(0)=1 → 1^power = 1.0  (full peak)
            // At edge  (norm_d=1): cos(π)=-1 → mapped to 0   (ground level)
            float base_falloff = 0.5f * (1.0f + std::cos(norm_d * 3.14159265f));
            float falloff = std::pow(base_falloff, falloff_power);

            // Add per-cell jitter for rough natural edges
            float jitter = jitter_dist(rng);
            falloff = std::clamp(falloff + jitter, 0.0f, 1.0f);

            cell.height = peak_height * falloff;
        }

        std::cout << "  Ridge sculpted: " << mountain_cells.size()
                  << " mountain cells, max_dist=" << max_mountain_dist << "\n";
        return {};
    }

    std::string get_name() const { return "MountainRidgeGenerator"; }
};

} // namespace generation

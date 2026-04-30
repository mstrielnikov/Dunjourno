#pragma once
#include <generation/generation.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <numeric>

namespace generation {

namespace noise {

inline float hash1d(uint32_t n) {
    n = (n << 13U) ^ n;
    n = n * (n * n * 15731U + 789221U) + 1376312589U;
    return static_cast<float>(n & 0x7fffffffU) / static_cast<float>(0x7fffffff);
}

inline float value_noise_1d(float x, uint32_t seed) {
    float i = std::floor(x);
    float f = x - i;
    float u = f * f * (3.0f - 2.0f * f); // Smoothstep

    uint32_t n0 = static_cast<uint32_t>(i) + seed;
    uint32_t n1 = n0 + 1;

    return hash1d(n0) * (1.0f - u) + hash1d(n1) * u;
}

// Multi-octave 1D fractal noise (fBm)
inline float fbm_1d(float x, uint32_t seed, int octaves = 4, float lacunarity = 2.0f, float gain = 0.5f) {
    float sum = 0.0f;
    float amp = 1.0f;
    float freq = 1.0f;
    float max_amp = 0.0f;
    for (int i = 0; i < octaves; ++i) {
        sum += amp * value_noise_1d(x * freq, seed + i * 137);
        max_amp += amp;
        amp *= gain;
        freq *= lacunarity;
    }
    return sum / max_amp;
}

// 2D value noise using hash-based lattice interpolation
inline float value_noise_2d(float x, float y, uint32_t seed) {
    float ix = std::floor(x);
    float iy = std::floor(y);
    float fx = x - ix;
    float fy = y - iy;
    float ux = fx * fx * (3.0f - 2.0f * fx);
    float uy = fy * fy * (3.0f - 2.0f * fy);

    uint32_t nx = static_cast<uint32_t>(ix);
    uint32_t ny = static_cast<uint32_t>(iy);

    float v00 = hash1d(nx + ny * 57U + seed);
    float v10 = hash1d((nx + 1) + ny * 57U + seed);
    float v01 = hash1d(nx + (ny + 1) * 57U + seed);
    float v11 = hash1d((nx + 1) + (ny + 1) * 57U + seed);

    float a = v00 + (v10 - v00) * ux;
    float b = v01 + (v11 - v01) * ux;
    return a + (b - a) * uy;
}

// Multi-octave 2D fractal noise (fBm)
inline float fbm_2d(float x, float y, uint32_t seed, int octaves = 6, float lacunarity = 2.0f, float gain = 0.5f) {
    float sum = 0.0f;
    float amp = 1.0f;
    float freq = 1.0f;
    float max_amp = 0.0f;
    for (int i = 0; i < octaves; ++i) {
        sum += amp * value_noise_2d(x * freq, y * freq, seed + i * 31);
        max_amp += amp;
        amp *= gain;
        freq *= lacunarity;
    }
    return sum / max_amp;
}

} // namespace noise

/**
 * @brief Mountain Placement Generator
 *
 * Owns the full placement rule for mountains:
 *   1. Randomly selects N center positions on the grid
 *   2. For each center, fills a circular base area (radius in cells)
 *      marking cells as TerrainType::Mountain
 *   3. Sets initial height using a radial falloff from each center
 *      so the peak is at the center and slopes to 0 at the edge
 */
struct MountainPlacementGenerator {
    int   num_mountains = 3;       // Number of mountain centers to place
    float base_radius   = 12.0f;   // Radius of each mountain base (in cells)

    MountainPlacementGenerator(int count, float radius)
        : num_mountains(count), base_radius(radius) {
        std::cout << "[INIT] MountainPlacementGenerator count=" << num_mountains
                  << " base_radius=" << base_radius << "\n";
    }

    std::expected<void, GenError> apply(Grid& grid) {
        std::cout << "[WORK] MountainPlacementGenerator placing " << num_mountains << " mountains...\n";

        if (num_mountains <= 0) return {};

        std::mt19937 rng(std::random_device{}());
        int w = (int)grid.width();
        int h = (int)grid.height();

        // Margin so mountains don't spawn clipped at edges
        int margin = (int)std::ceil(base_radius * 0.5f);
        std::uniform_int_distribution<int> dist_x(margin, std::max(margin, w - margin - 1));
        std::uniform_int_distribution<int> dist_y(margin, std::max(margin, h - margin - 1));
        std::uniform_real_distribution<float> axis_scale(0.5f, 1.5f);
        std::uniform_real_distribution<float> angle_dist(0.0f, 3.14159265f);

        struct Center { float x, y, rx, ry, cos_a, sin_a; uint32_t seed; };
        std::vector<Center> centers;
        centers.reserve(num_mountains);

        for (int i = 0; i < num_mountains; ++i) {
            float rx = base_radius * axis_scale(rng);  // Random semi-major axis
            float ry = base_radius * axis_scale(rng);  // Random semi-minor axis
            float a  = angle_dist(rng);                 // Random rotation
            centers.push_back({
                (float)dist_x(rng), (float)dist_y(rng),
                rx, ry,
                std::cos(a), std::sin(a),
                static_cast<uint32_t>(rng())
            });
        }

        // For each cell, check elliptical distance to the nearest mountain center
        for (auto [x, y, cell] : grid.iter()) {
            float best_falloff = 0.0f;

            for (const auto& c : centers) {
                // Translate to center-relative coordinates
                float dx = (float)x - c.x;
                float dy = (float)y - c.y;

                // Rotate into the ellipse's local frame
                float lx = dx * c.cos_a + dy * c.sin_a;
                float ly = -dx * c.sin_a + dy * c.cos_a;

                // Normalized elliptical distance: 1.0 at the boundary
                float ellipse_dist = std::sqrt((lx * lx) / (c.rx * c.rx) + (ly * ly) / (c.ry * c.ry));

                // Angular perturbation on top of the ellipse shape
                float angle = std::atan2(ly, lx);
                float n1 = noise::fbm_1d(angle * 6.0f, c.seed, 4, 2.0f, 0.5f);
                float n2 = noise::fbm_1d(angle * 12.0f, c.seed + 7777, 2, 2.0f, 0.4f);
                float angular_noise = n1 * 0.7f + n2 * 0.3f;
                float perturbed_boundary = 0.4f + 1.2f * angular_noise;  // ~0.4 to ~1.6

                if (ellipse_dist < perturbed_boundary) {
                    float norm = ellipse_dist / perturbed_boundary;
                    float falloff = 0.5f * (1.0f + std::cos(norm * 3.14159265f));
                    best_falloff = std::max(best_falloff, falloff);
                }
            }

            if (best_falloff > 0.01f) {
                cell.terrain = TerrainType::Mountain;
                cell.height = std::max(cell.height, best_falloff);
            }
        }

        std::cout << "  Placed " << centers.size() << " mountain centers\n";
        return {};
    }

    std::string get_name() const { return "MountainPlacement"; }
};


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
            float n = noise::fbm_1d(t * chord_len * noise_frequency, noise_seed, 4);
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
            float base_falloff = 0.5f * (1.0f + std::cos(norm_d * 3.14159265f));
            float falloff = std::pow(base_falloff, falloff_power);

            // Add position-based hash noise to break uniform radial slopes
            uint32_t px = static_cast<uint32_t>(x) * 374761393U;
            uint32_t py = static_cast<uint32_t>(y) * 668265263U;
            float pos_noise = noise::hash1d(px ^ py ^ noise_seed);
            falloff *= (0.7f + 0.6f * pos_noise);  // ±30% per-cell variation

            // Add per-cell jitter for rough natural edges
            float jitter = jitter_dist(rng);
            falloff = std::clamp(falloff + jitter, 0.0f, 1.0f);

            cell.height = peak_height * falloff;

            // Assign roughness proportional to height using position hash
            uint32_t rx = static_cast<uint32_t>(x) * 374761393U;
            uint32_t ry = static_cast<uint32_t>(y) * 668265263U;
            float rng_val = noise::hash1d(rx ^ ry ^ noise_seed);
            cell.roughness = std::clamp(rng_val * falloff * 1.5f, 0.0f, 1.0f);
        }

        std::cout << "  Ridge sculpted: " << mountain_cells.size()
                  << " mountain cells, max_dist=" << max_mountain_dist << "\n";
        return {};
    }

    std::string get_name() const { return "MountainRidgeGenerator"; }
};

} // namespace generation

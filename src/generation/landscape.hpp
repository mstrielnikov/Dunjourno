#pragma once
#include <generation/generation.hpp>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <random>
#include <stb_perlin.h>

namespace generation {

struct LandscapeGenerator {
    float freequency = {};
    
    LandscapeGenerator(float f) : freequency(f) {
        std::cout << "[INIT] LandscapeGenerator created with freq: " << freequency << "\n";
    }
    
    std::expected <void, GenError> apply(Grid& grid) {
        std::cout << "[WORK] LandscapeGenerator applying transformation...\n";
        float centerX = grid.width() / 2.0f;
        float centerY = grid.height() / 2.0f;
        for (auto [x, y, cell] : grid.iter()) {
            float dx = (x - centerX) / centerX;
            float dy = (y - centerY) / centerY;
            float distance = std::sqrt(dx * dx + dy * dy);
            
            cell.height = std::clamp(0.5f + 0.5f * std::sin(distance + freequency), 0.0f, 1.0f);
        }
        return {};
    }

    std::string get_name() const { return "LandscapeGenerator"; }
};

struct RippleTerrainGenerator {
    float rippleFreq = {};

    RippleTerrainGenerator(float f) : rippleFreq(f) {
        std::cout << "[INIT] RippleTerrainGenerator created with freq: " << rippleFreq << "\n";
    }

    std::expected<void, GenError> apply(Grid& grid) {
        std::cout << "[WORK] RippleTerrainGenerator creating base landmass...\n";
        // Random seed per generation so terrain varies on each "Generate" press
        uint32_t seed = std::random_device{}();
        for (auto [x, y, cell] : grid.iter()) {
            float nx = static_cast<float>(x) / grid.width();
            float ny = static_cast<float>(y) / grid.height();
            // Base ripple pattern
            float ripple = std::sin(nx * rippleFreq) * std::cos(ny * rippleFreq);
            // Perlin noise perturbation for natural variation
            float noise = stb_perlin_noise3_seed(nx * 4.0f, ny * 4.0f, 0.0f, 0, 0, 0, seed);
            float val = ripple * 0.6f + noise * 0.4f;
            cell.height = std::clamp(0.5f + 0.5f * val, 0.0f, 1.0f);
        }
        return {};
    }

    std::string get_name() const { return "RippleTerrainGenerator"; }
};

} // namespace generation

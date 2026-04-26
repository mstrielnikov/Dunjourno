#pragma once
#include <generation.hpp>
#include <iostream>

struct LandscapeGenerator {
    float freequency = {};
    
    LandscapeGenerator(float f) : freequency(f) {
        std::cout << "[INIT] LandscapeGenerator created with freq: " << freequency << "\n";
    }
    
    std::expected <void, GenError> apply(Grid& grid) {
        std::cout << "[WORK] LandscapeGenerator applying transformation...\n";
        float centerX = grid.get_width() / 2.0f;
        float centerY = grid.get_height() / 2.0f;
        for (size_t y = 0; y < grid.get_height(); y++){
            for (size_t x = 0; x < grid.get_width(); x++){
                float dx = (x - centerX) / centerX;
                float dy = (y - centerY) / centerY;
                float distance = std::sqrt(dx * dx + dy * dy);
                grid(x, y).height = std::clamp(0.5f + 0.5f * std::sin(distance + freequency), 0.0f, 1.0f);
            }
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
        for (size_t y = 0; y < grid.get_height(); ++y) {
            for (size_t x = 0; x < grid.get_width(); ++x) {
                float nx = static_cast<float>(x) / grid.get_width();
                float ny = static_cast<float>(y) / grid.get_height();
                float val = std::sin(nx * rippleFreq) * std::cos(ny * rippleFreq);
                grid(x, y).height = std::clamp(0.5f + 0.5f * val, 0.0f, 1.0f);
            }
        }
        return {};
    }

    std::string get_name() const { return "RippleTerrainGenerator"; }
};

#pragma once

namespace generation {

struct GenConfig {
    float grid_size        = 128.0f;  // Grid width/height (32 to 256)
    float num_mountains    = 3.0f;    // Number of mountain pools (1 to 10)
    float terrain_coverage = 0.25f;   // Fraction of grid allocated to mountains (0.05 to 1.00)
    float peak_height      = 3.0f;    // Maximum terrain height
    float erosion_drops    = 70.0f;   // Erosion droplets in thousands (10 to 200)
    float forest_density   = 0.4f;    // Forest placement density
    float time_of_day      = 15.0f;   // Sun angle (6 to 18 hours)
    float mesh_resolution  = 4.0f;    // Subdivision resolution per grid cell (1 to 64)
};

}

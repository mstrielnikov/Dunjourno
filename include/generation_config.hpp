#pragma once

namespace generation {

struct GenConfig {
    float grid_size        = 128.0f;  // Grid width/height (32 to 256)
    float peak_height      = 3.0f;    // Maximum terrain height
    float noise_scale      = 0.02f;   // Noise feature size (smaller = larger features)
    float erosion_drops    = 70.0f;   // Erosion droplets in thousands (10 to 200)
    float forest_density   = 0.4f;    // Forest placement density
    float time_of_day      = 15.0f;   // Sun angle (0 to 24 hours, 12 is noon)
    float mesh_resolution  = 4.0f;    // Subdivision resolution per grid cell (e.g. 4 to 64)
};

}

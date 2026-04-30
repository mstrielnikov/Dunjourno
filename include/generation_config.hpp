#pragma once

namespace generation {

struct GenConfig {
    float grid_size        = 128.0f;  // Grid width/height (32 to 256)
    float num_mountains    = 3.0f;    // Number of mountain placements (0+)
    float mountain_base    = 12.0f;   // Base radius in cells per mountain
    float mountain_peak    = 3.0f;    // Mountain peak height
    float mountain_power   = 2.5f;    // Mountain falloff power
    float forest_density   = 0.4f;    // Forest placement density
};

}

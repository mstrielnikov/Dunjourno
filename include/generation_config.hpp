#pragma once

namespace generation {

struct GenConfig {
    float grid_size = 128.0f; // Grid width/height (64 to 256)
    float ripple_amp = 6.0f;  // Ripple amplitude
    float mountain_peak = 3.0f; // Mountain peak height
    float mountain_power = 2.5f; // Mountain falloff power
    float forest_density = 0.4f; // Forest placement density
};

}

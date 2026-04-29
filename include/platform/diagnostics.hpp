#pragma once

#include <string>

#if defined(PLATFORM_WEB)

// WebAssembly/WebGL Diagnostic Stub
namespace diagnostics {
    inline std::string get_gpu_name() {
        return "WebGL (Wasm / Emscripten)";
    }
    
    inline float get_vram_usage_percent() {
        // VRAM metrics are not securely accessible via browser APIs
        return 0.0f; 
    }
}

#else

// Native Linux Diagnostic Implementation
#include <fstream>
extern "C" const unsigned char *glGetString(unsigned int name);

namespace diagnostics {
    inline std::string get_gpu_name() {
        // 0x1F01 corresponds to GL_RENDERER
        const char* renderer = (const char*)glGetString(0x1F01); 
        return renderer ? std::string(renderer) : "Unknown Native GPU";
    }

    inline float get_vram_usage_percent() {
        auto get_vram_total = []() -> uint64_t {
            std::ifstream file("/sys/class/drm/card0/device/mem_info_vram_total");
            uint64_t bytes = 0;
            if (file >> bytes) return bytes;
            return 3072ULL * 1024ULL * 1024ULL; // Fallback to 3GB
        };

        auto get_vram_used = []() -> uint64_t {
            std::ifstream file("/sys/class/drm/card0/device/mem_info_vram_used");
            uint64_t bytes = 0;
            if (file >> bytes) return bytes;
            return 0;
        };

        uint64_t total = get_vram_total();
        if (total == 0) return 0.0f;
        return (float)get_vram_used() / (float)total * 100.0f;
    }
}

#endif

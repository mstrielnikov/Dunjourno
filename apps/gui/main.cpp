#include <iostream>
#include <algorithm>
#include <unistd.h>

#include <raylib.h>
#include <raymath.h>

#include "platform/diagnostics.hpp"
#include "generation/pipeline.hpp"
#include "generation/landscape.hpp"
#include "generation/forest.hpp"
#include "generation/grid.hpp"

using namespace generation;

void DrawGPUTile(float x, float y, float w, float h, Color baseColor) {
    Vector2 top = { x, y };
    Vector2 left = { x - w / 2.0f, y + h / 2.0f };
    Vector2 right = { x + w / 2.0f, y + h / 2.0f };
    Vector2 bottom = { x, y + h };
    DrawTriangle(top, left, bottom, baseColor);
    DrawTriangle(top, bottom, right, ColorBrightness(baseColor, -0.15f));
}

int main() {
    const int screenWidth = 1200;
    const int screenHeight = 840;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Donjourno");
    SetTargetFPS(60);

    auto grid_res = Grid::create(128, 128);
    if (!grid_res.has_value()) {
        std::cerr << "Failed to allocate Grid\n";
        return 1;
    }
    Grid& grid = grid_res.value();
    GridView view = grid.view();
    auto pipeline = Pipeline(RippleTerrainGenerator{6.0f}, ForestMaskGenerator{0.7f, 2.5f}, ForestPlacementGenerator{0.4f});
    pipeline.execute(grid);

    Camera2D camera = { .offset = { 600, 200 }, .target = { 0, 0 }, .rotation = 0.0f, .zoom = 0.5f };

    while (!WindowShouldClose()) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 delta = GetMouseDelta();
            camera.target.x -= delta.x / camera.zoom;
            camera.target.y -= delta.y / camera.zoom;
        }
        camera.zoom = std::clamp(camera.zoom + GetMouseWheelMove() * 0.05f, 0.1f, 3.0f);

        BeginDrawing();
            ClearBackground({ 30, 30, 35, 255 });
            
            BeginMode2D(camera);
                for (size_t y = 0; y < view.height(); ++y) {
                    for (size_t x = 0; x < view.width(); ++x) {
                        float isoX = (float)((int)x - (int)y) * 32.0f;
                        float isoY = (float)((int)x + (int)y) * 16.0f;
                        
                        const Cell& cell = view(x, y);
                        Color tint = {40, (unsigned char)(cell.height * 120.0f + 60.0f), 40, 255};
                        if (cell.terrain == TerrainType::Mountain) tint = {110, 100, 90, 255};
                        
                        DrawGPUTile(isoX, isoY, 64.0f, 32.0f, tint);
                    }
                }
            EndMode2D();

            DrawRectangle(0, 0, GetScreenWidth(), 60, Fade(BLACK, 0.8f));
            DrawText("Donjourno", 20, 10, 20, GOLD);
            DrawText(TextFormat("Renderer: %s | VRAM: %.1f%%", diagnostics::get_gpu_name().c_str(), diagnostics::get_vram_usage_percent()), 20, 35, 14, LIME);
            DrawFPS(GetScreenWidth() - 80, 10);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}

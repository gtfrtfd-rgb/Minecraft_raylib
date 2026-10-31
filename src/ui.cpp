#include "ui.h"
#include "config.h"
#include "world.h"
#include "raylib.h"

namespace UI {

void Draw() {
    DrawFPS(10, 10);
    DrawText("Minecraft Clone (raylib)", 10, 40, 20, RAYWHITE);
    DrawText("WASD / Arrows - Move | Mouse - Look | Space - Jump", 10, 70, 16, LIGHTGRAY);
    DrawText("LMB - Break | RMB - Place", 10, 90, 16, LIGHTGRAY);
}

void DrawBlockHighlight(Vector3 blockPos) {
    Vector3 center = { blockPos.x + 0.5f, blockPos.y + 0.5f, blockPos.z + 0.5f };
    
    // Немного увеличиваем куб, чтобы линии не сливались с текстурой самого блока
    float expand = 0.005f;
    
    // Жирная черная обводка (без заливки)
    float t = 0.008f;
    for (float x = -t; x <= t; x += t) {
        for (float y = -t; y <= t; y += t) {
            for (float z = -t; z <= t; z += t) {
                DrawCubeWires(center, 1.0f + expand + x, 1.0f + expand + y, 1.0f + expand + z, BLACK);
            }
        }
    }
}

} // namespace UI
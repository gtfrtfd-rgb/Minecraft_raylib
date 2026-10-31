#include "inventory.h"
#include "texture_atlas.h"
#include "block.h"

Inventory::Inventory() : selectedSlot(0) {
    slots = {
        BlockType::Grass,
        BlockType::Dirt,
        BlockType::Stone,
        BlockType::Sand,
        BlockType::Wood,
        BlockType::Leaves
    };
    while (slots.size() < 9) {
        slots.push_back(BlockType::Air);
    }
}

void Inventory::Update() {
    if (IsKeyPressed(KEY_ONE)) selectedSlot = 0;
    if (IsKeyPressed(KEY_TWO)) selectedSlot = 1;
    if (IsKeyPressed(KEY_THREE)) selectedSlot = 2;
    if (IsKeyPressed(KEY_FOUR)) selectedSlot = 3;
    if (IsKeyPressed(KEY_FIVE)) selectedSlot = 4;
    if (IsKeyPressed(KEY_SIX)) selectedSlot = 5;
    if (IsKeyPressed(KEY_SEVEN)) selectedSlot = 6;
    if (IsKeyPressed(KEY_EIGHT)) selectedSlot = 7;
    if (IsKeyPressed(KEY_NINE)) selectedSlot = 8;

    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        selectedSlot -= (int)wheel;
        if (selectedSlot < 0) selectedSlot = 8;
        if (selectedSlot > 8) selectedSlot = 0;
    }
}

void Inventory::Draw(int screenWidth, int screenHeight) {
    Texture2D atlas = TextureAtlas::GetTexture();
    int slotSize = 50;
    int padding = 4;
    int totalWidth = 9 * (slotSize + padding);
    int startX = (screenWidth - totalWidth) / 2;
    int startY = screenHeight - slotSize - 10;

    for (int i = 0; i < 9; i++) {
        int x = startX + i * (slotSize + padding);
        DrawRectangle(x, startY, slotSize, slotSize, Fade(GRAY, 0.6f));
        
        if (i == selectedSlot) {
            DrawRectangleLines(x - 2, startY - 2, slotSize + 4, slotSize + 4, WHITE);
        } else {
            DrawRectangleLines(x, startY, slotSize, slotSize, DARKGRAY);
        }

        if (slots[i] != BlockType::Air) {
            // Используем texSide для иконки (у дерева будет кора)
            int texIndex = BlockRegistry::Get(slots[i]).texSide;
            
            int tx = texIndex % 16;
            int ty = texIndex / 16;
            Rectangle src = { (float)(tx * 32), (float)(ty * 32), 32.0f, 32.0f };
            Rectangle dest = { (float)x + 4, (float)startY + 4, (float)slotSize - 8, (float)slotSize - 8 };
            
            DrawTexturePro(atlas, src, dest, {0, 0}, 0.0f, WHITE);
        }
        
        DrawText(TextFormat("%d", i + 1), x + 4, startY + slotSize - 16, 10, LIGHTGRAY);
    }
}

BlockType Inventory::GetSelectedBlock() const {
    return slots[selectedSlot];
}
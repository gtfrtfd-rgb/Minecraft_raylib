#pragma once
#include "block.h"
#include "raylib.h"
#include <vector>

class Inventory {
public:
    Inventory();
    void Update();
    void Draw(int screenWidth, int screenHeight);
    BlockType GetSelectedBlock() const;

private:
    std::vector<BlockType> slots;
    int selectedSlot;
};
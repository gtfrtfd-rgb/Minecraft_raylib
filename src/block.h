#pragma once

#include "raylib.h"

enum class BlockType : unsigned char {
    Air = 0,
    Grass,
    Dirt,
    Stone,
    Sand,
    Wood,
    Leaves,
    Count
};

struct BlockInfo {
    const char* name;
    bool isSolid;
    int texTop;
    int texSide;
    int texBottom;
};

class BlockRegistry {
public:
    static void Init();
    static const BlockInfo& Get(BlockType type);
    static BlockType FromIndex(int index);
    static int ToIndex(BlockType type);

private:
    static BlockInfo blocks[static_cast<int>(BlockType::Count)];
};
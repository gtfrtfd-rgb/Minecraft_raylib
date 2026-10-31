#pragma once
#include "raylib.h"

class TextureAtlas {
public:
    static void Init();
    static void Unload();
    static Texture2D GetTexture();

    enum Tile {
        TILE_GRASS_TOP = 0,
        TILE_GRASS_SIDE = 1,
        TILE_DIRT = 2,
        TILE_STONE = 3,
        TILE_SAND = 4,
        TILE_WOOD_SIDE = 5,
        TILE_LEAVES = 6,
        TILE_WOOD_TOP = 7 // ⬅️ Добавлен новый тайл
    };

    static Rectangle GetUV(int tileIndex);

private:
    static Texture2D atlas;
};
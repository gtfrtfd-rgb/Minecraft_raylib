#include "texture_atlas.h"
#include "raymath.h"
#include <algorithm> 
#include <cmath>     

Texture2D TextureAtlas::atlas = { 0 };

static unsigned int Hash2D(int x, int y, int seed) {
    unsigned int h = (unsigned int)x * 374761393U + (unsigned int)y * 668265263U + (unsigned int)seed * 1442695040U;
    h = (h ^ (h >> 13U)) * 1274126177U;
    return h ^ (h >> 16U);
}

void TextureAtlas::Init() {
    Image img = GenImageColor(512, 512, BLANK);
    const int TILE_SIZE = 32;

    // 1. ЗЕМЛЯ (Dirt)
    for (int x = 0; x < TILE_SIZE; x++) {
        for (int y = 0; y < TILE_SIZE; y++) {
            unsigned int n = Hash2D(x / 2, y / 2, 1) % 100; 
            Color c = { 134, 96, 67, 255 }; 
            if (n < 20) c = { 104, 66, 37, 255 };       
            else if (n < 40) c = { 150, 110, 80, 255 }; 
            else if (n < 45) c = { 160, 120, 90, 255 }; 
            if (Hash2D(x, y, 2) % 100 < 10) c.r -= 10, c.g -= 10, c.b -= 10;
            ImageDrawPixel(&img, 2 * TILE_SIZE + x, 0 * TILE_SIZE + y, c);
        }
    }

    // 2. КАМЕНЬ (Stone)
    for (int x = 0; x < TILE_SIZE; x++) {
        for (int y = 0; y < TILE_SIZE; y++) {
            unsigned int n = Hash2D(x / 3, y / 3, 3) % 100; 
            Color c = { 128, 128, 128, 255 };
            if (n < 15) c = { 98, 98, 98, 255 };
            else if (n < 30) c = { 148, 148, 148, 255 };
            else if (n < 40) c = { 80, 80, 80, 255 }; 
            if (Hash2D(x, y, 4) % 100 < 15) { c.r -= 15; c.g -= 15; c.b -= 15; }
            ImageDrawPixel(&img, 3 * TILE_SIZE + x, 0 * TILE_SIZE + y, c);
        }
    }

    // 3. ПЕСОК (Sand)
    for (int x = 0; x < TILE_SIZE; x++) {
        for (int y = 0; y < TILE_SIZE; y++) {
            unsigned int n = Hash2D(x, y, 5) % 100;
            Color c = { 218, 204, 155, 255 };
            if (n < 15) c = { 198, 184, 135, 255 };
            else if (n < 25) c = { 238, 224, 175, 255 };
            ImageDrawPixel(&img, 4 * TILE_SIZE + x, 0 * TILE_SIZE + y, c);
        }
    }

    // 4. ДЕРЕВО СБОКУ (Wood Side)
    for (int x = 0; x < TILE_SIZE; x++) {
        unsigned int colNoise = Hash2D(x, 0, 6) % 100;
        Color baseC = { 102, 67, 42, 255 };
        if (colNoise < 20) baseC = { 80, 50, 30, 255 }; 
        else if (colNoise < 40) baseC = { 122, 87, 62, 255 }; 
        for (int y = 0; y < TILE_SIZE; y++) {
            Color c = baseC;
            if (Hash2D(x, y / 4, 7) % 100 < 10) c = { 70, 40, 20, 255 };
            ImageDrawPixel(&img, 5 * TILE_SIZE + x, 0 * TILE_SIZE + y, c);
        }
    }

    // 5. ЛИСТВА (Leaves)
    for (int x = 0; x < TILE_SIZE; x++) {
        for (int y = 0; y < TILE_SIZE; y++) {
            unsigned int n = Hash2D(x, y, 8) % 100;
            Color c;
            if (n < 20) c = { 30, 80, 20, 255 };        
            else if (n < 70) c = { 55, 120, 40, 255 };  
            else c = { 80, 160, 60, 255 };              
            if (Hash2D(x, y, 9) % 15 == 0) c = { 110, 190, 80, 255 };
            if (x == 0 || y == 0 || x == TILE_SIZE-1 || y == TILE_SIZE-1) { c.r -= 15; c.g -= 15; c.b -= 15; }
            ImageDrawPixel(&img, 6 * TILE_SIZE + x, 0 * TILE_SIZE + y, c);
        }
    }

    // 6. ТРАВА СВЕРХУ (Grass Top)
    for (int x = 0; x < TILE_SIZE; x++) {
        for (int y = 0; y < TILE_SIZE; y++) {
            unsigned int n = Hash2D(x / 2, y / 2, 10) % 100;
            Color c = { 95, 159, 53, 255 };
            if (n < 20) c = { 70, 130, 30, 255 };
            else if (n < 40) c = { 120, 180, 70, 255 };
            if (Hash2D(x, y, 11) % 100 < 15) c = { 140, 200, 90, 255 };
            ImageDrawPixel(&img, 0 * TILE_SIZE + x, 0 * TILE_SIZE + y, c);
        }
    }

    // 7. ТРАВА СБОКУ (Grass Side)
    for (int x = 0; x < TILE_SIZE; x++) {
        int grassHeight = 4 + (Hash2D(x, 0, 12) % 9); 
        for (int y = 0; y < TILE_SIZE; y++) {
            Color c;
            unsigned int n = Hash2D(x, y, 1) % 100;
            if (y < grassHeight - 1) {
                if (n < 25) c = { 75, 139, 33, 255 };
                else if (n < 50) c = { 115, 179, 73, 255 };
                else c = { 95, 159, 53, 255 };
            } else if (y == grassHeight - 1) {
                c = { 55, 100, 25, 255 }; 
            } else {
                if (n < 20) c = { 104, 66, 37, 255 };
                else if (n < 40) c = { 150, 110, 80, 255 };
                else c = { 134, 96, 67, 255 };
            }
            ImageDrawPixel(&img, 1 * TILE_SIZE + x, 0 * TILE_SIZE + y, c);
        }
    }

    // 8. ДЕРЕВО СВЕРХУ (Wood Top)
    for (int x = 0; x < TILE_SIZE; x++) {
        for (int y = 0; y < TILE_SIZE; y++) {
            int dx = x - 16;
            int dy = y - 16;
            int dist = std::max(std::abs(dx), std::abs(dy)); 
            Color c;
            if (dist > 14) c = { 80, 50, 30, 255 };       
            else if (dist > 11) c = { 102, 67, 42, 255 }; 
            else if (dist > 9) c = { 140, 100, 60, 255 }; 
            else if (dist > 6) c = { 120, 80, 45, 255 };  
            else if (dist > 3) c = { 140, 100, 60, 255 }; 
            else c = { 100, 65, 35, 255 };                
            if (Hash2D(x, y, 13) % 100 < 10) c.r -= 10, c.g -= 10, c.b -= 10;
            ImageDrawPixel(&img, 7 * TILE_SIZE + x, 0 * TILE_SIZE + y, c);
        }
    }

    atlas = LoadTextureFromImage(img);
    SetTextureFilter(atlas, TEXTURE_FILTER_POINT); 
    UnloadImage(img);
}

void TextureAtlas::Unload() { UnloadTexture(atlas); }
Texture2D TextureAtlas::GetTexture() { return atlas; }
Rectangle TextureAtlas::GetUV(int tileIndex) {
    int tx = tileIndex % 16;
    int ty = tileIndex / 16;
    return { (float)tx / 16.0f, (float)ty / 16.0f, 1.0f / 16.0f, 1.0f / 16.0f };
}
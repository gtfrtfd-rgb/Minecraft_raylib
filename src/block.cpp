#include "block.h"
#include "texture_atlas.h"

BlockInfo BlockRegistry::blocks[static_cast<int>(BlockType::Count)];

void BlockRegistry::Init() {
    blocks[static_cast<int>(BlockType::Air)]    = {"Air",    false, 0, 0, 0};
    
    blocks[static_cast<int>(BlockType::Grass)]  = {"Grass",  true,  
        TextureAtlas::TILE_GRASS_TOP, 
        TextureAtlas::TILE_GRASS_SIDE, 
        TextureAtlas::TILE_DIRT};
        
    blocks[static_cast<int>(BlockType::Dirt)]   = {"Dirt",   true,  
        TextureAtlas::TILE_DIRT, TextureAtlas::TILE_DIRT, TextureAtlas::TILE_DIRT};
        
    blocks[static_cast<int>(BlockType::Stone)]  = {"Stone",  true,  
        TextureAtlas::TILE_STONE, TextureAtlas::TILE_STONE, TextureAtlas::TILE_STONE};
        
    blocks[static_cast<int>(BlockType::Sand)]   = {"Sand",   true,  
        TextureAtlas::TILE_SAND, TextureAtlas::TILE_SAND, TextureAtlas::TILE_SAND};
        
    // ⬇️ Дерево теперь имеет годовые кольца сверху и кору сбоку ⬇️
    blocks[static_cast<int>(BlockType::Wood)]   = {"Wood",   true,  
        TextureAtlas::TILE_WOOD_TOP, 
        TextureAtlas::TILE_WOOD_SIDE, 
        TextureAtlas::TILE_WOOD_TOP};
        
    blocks[static_cast<int>(BlockType::Leaves)] = {"Leaves", true,  
        TextureAtlas::TILE_LEAVES, TextureAtlas::TILE_LEAVES, TextureAtlas::TILE_LEAVES};
}

const BlockInfo& BlockRegistry::Get(BlockType type) {
    return blocks[static_cast<int>(type)];
}

BlockType BlockRegistry::FromIndex(int index) {
    if (index < 0 || index >= static_cast<int>(BlockType::Count)) return BlockType::Air;
    return static_cast<BlockType>(index);
}

int BlockRegistry::ToIndex(BlockType type) {
    return static_cast<int>(type);
}
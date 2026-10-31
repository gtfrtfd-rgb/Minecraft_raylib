#include "chunk.h"
#include "config.h"
#include "utils.h"
#include "world.h"
#include "texture_atlas.h"
#include <cmath>
#include <cstring>

namespace {
    // Atlas 512x512, 16x16 тайлов по 32px. Inset против bleeding.
    constexpr float ATLAS_TILE_UV = 1.0f / 16.0f;
    constexpr float UV_INSET = 1.0f / 512.0f;

    constexpr int FACE_DX[6] = { 1, -1, 0, 0, 0, 0 };
    constexpr int FACE_DY[6] = { 0, 0, 1, -1, 0, 0 };
    constexpr int FACE_DZ[6] = { 0, 0, 0, 0, 1, -1 };
    constexpr unsigned char FACE_BRIGHTNESS[6] = { 180, 180, 255, 100, 180, 180 };
}

Chunk::Chunk(int cx, int cz) : cx(cx), cz(cz), state(ChunkState::Empty) {
    memset(blocks, 0, sizeof(blocks));
}

Chunk::~Chunk() {
    UnloadMesh();
}

void Chunk::GenerateData() {
    std::unique_lock lock(blocksMutex);

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            int worldX = cx * CHUNK_SIZE + x;
            int worldZ = cz * CHUNK_SIZE + z;

            float noise = Utils::Noise2D(worldX * TERRAIN_FREQUENCY, worldZ * TERRAIN_FREQUENCY);
            int height = static_cast<int>(TERRAIN_BASE_HEIGHT + noise * TERRAIN_AMPLITUDE);

            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                if (y > height) {
                    blocks[x][y][z] = BlockType::Air;
                } else if (y == height) {
                    blocks[x][y][z] = (height < 18) ? BlockType::Sand : BlockType::Grass;
                } else if (y > height - STONE_LAYER_DEPTH) {
                    blocks[x][y][z] = BlockType::Dirt;
                } else {
                    blocks[x][y][z] = BlockType::Stone;
                }
            }
        }
    }

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            if (x < 1 || x >= CHUNK_SIZE - 1 || z < 1 || z >= CHUNK_SIZE - 1) continue;

            int worldX = cx * CHUNK_SIZE + x;
            int worldZ = cz * CHUNK_SIZE + z;

            int height = CHUNK_HEIGHT - 1;
            while (height > 0 && blocks[x][height][z] == BlockType::Air) height--;

            float forestNoise = Utils::Noise2D(worldX * 0.01f, worldZ * 0.01f);
            if (forestNoise > 0.1f && blocks[x][height][z] == BlockType::Grass
                && (Utils::TreeHash(worldX, worldZ) % 120) == 0) {
                int trunkHeight = 5 + (Utils::TreeHash(worldX + 1, worldZ + 1) % 3);
                int topY = height + trunkHeight;

                for (int ty = 1; ty <= trunkHeight; ty++) {
                    if (height + ty < CHUNK_HEIGHT) blocks[x][height + ty][z] = BlockType::Wood;
                }

                for (int ly = -1; ly <= 2; ly++) {
                    int radius = (ly == 2) ? 1 : 2;
                    for (int lx = -radius; lx <= radius; lx++) {
                        for (int lz = -radius; lz <= radius; lz++) {
                            if (radius == 2 && std::abs(lx) == 2 && std::abs(lz) == 2) continue;

                            int leafX = x + lx;
                            int leafY = topY + ly;
                            int leafZ = z + lz;

                            if (leafX >= 0 && leafX < CHUNK_SIZE
                                && leafY >= 0 && leafY < CHUNK_HEIGHT
                                && leafZ >= 0 && leafZ < CHUNK_SIZE) {
                                if (blocks[leafX][leafY][leafZ] == BlockType::Air) {
                                    if (Utils::TreeHash(leafX + leafY * 17, leafZ + leafY * 23) % 15 != 0) {
                                        blocks[leafX][leafY][leafZ] = BlockType::Leaves;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

BlockType Chunk::GetBlock(int x, int y, int z) const {
    std::shared_lock lock(blocksMutex);
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE)
        return BlockType::Air;
    return blocks[x][y][z];
}

void Chunk::SetBlock(int x, int y, int z, BlockType type) {
    std::unique_lock lock(blocksMutex);
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE)
        return;
    blocks[x][y][z] = type;
}

void Chunk::MarkDirty() {
    // FIX: если чанк уже отрисован, переводим в Generated,
    // чтобы World::Update снова запустил мешинг.
    ChunkState s = state.load();
    switch (s) {
        case ChunkState::Loaded:
        case ChunkState::MeshReady:
            needsRemesh = true;
            state = ChunkState::Generated;
            break;
        case ChunkState::Meshing:
            needsRemesh = true;
            break;
        case ChunkState::Generated:
        case ChunkState::Generating:
        case ChunkState::Empty:
        default:
            // нечего делать — чанк всё равно будет/уже перестроен
            break;
    }
}

void Chunk::BuildMeshData() {
    std::unique_lock chunkLock(chunkMutex);

    pendingVertices.clear();
    pendingTexcoords.clear();
    pendingColors.clear();
    pendingIndices.clear();

    // FIX: снимок флага в начале мешинга — все изменения, случившиеся
    // в процессе, приведут к повторному мешу.
    needsRemesh.store(false);

    std::shared_lock blocksLock(blocksMutex);

    unsigned short vertexIndex = 0;

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                BlockType current = blocks[x][y][z];
                if (current == BlockType::Air) continue;
                const BlockInfo& info = BlockRegistry::Get(current);

                for (int f = 0; f < 6; f++) {
                    int nx = x + FACE_DX[f];
                    int ny = y + FACE_DY[f];
                    int nz = z + FACE_DZ[f];

                    bool neighborSolid = false;
                    if (nx >= 0 && nx < CHUNK_SIZE && ny >= 0 && ny < CHUNK_HEIGHT
                        && nz >= 0 && nz < CHUNK_SIZE) {
                        neighborSolid = (blocks[nx][ny][nz] != BlockType::Air);
                    } else {
                        BlockType neighbor = World::GetBlock(
                            cx * CHUNK_SIZE + nx, ny, cz * CHUNK_SIZE + nz);
                        neighborSolid = (neighbor != BlockType::Air);
                    }

                    if (neighborSolid) continue;

                    int texIndex = info.texSide;
                    if (f == 2) texIndex = info.texTop;
                    if (f == 3) texIndex = info.texBottom;

                    Rectangle uv = TextureAtlas::GetUV(texIndex);
                    uv.x += UV_INSET;           uv.y += UV_INSET;
                    uv.width -= UV_INSET * 2.0f; uv.height -= UV_INSET * 2.0f;

                    float fx = (float)x, fy = (float)y, fz = (float)z;
                    float verts[4][3];
                    float uvs[4][2];

                    switch (f) {
                        case 0:
                            verts[0][0]=fx+1; verts[0][1]=fy;   verts[0][2]=fz;
                            verts[1][0]=fx+1; verts[1][1]=fy+1; verts[1][2]=fz;
                            verts[2][0]=fx+1; verts[2][1]=fy+1; verts[2][2]=fz+1;
                            verts[3][0]=fx+1; verts[3][1]=fy;   verts[3][2]=fz+1;
                            uvs[0][0]=uv.x; uvs[0][1]=uv.y+uv.height; uvs[1][0]=uv.x; uvs[1][1]=uv.y;
                            uvs[2][0]=uv.x+uv.width; uvs[2][1]=uv.y; uvs[3][0]=uv.x+uv.width; uvs[3][1]=uv.y+uv.height;
                            break;
                        case 1:
                            verts[0][0]=fx; verts[0][1]=fy;   verts[0][2]=fz+1;
                            verts[1][0]=fx; verts[1][1]=fy+1; verts[1][2]=fz+1;
                            verts[2][0]=fx; verts[2][1]=fy+1; verts[2][2]=fz;
                            verts[3][0]=fx; verts[3][1]=fy;   verts[3][2]=fz;
                            uvs[0][0]=uv.x+uv.width; uvs[0][1]=uv.y+uv.height; uvs[1][0]=uv.x+uv.width; uvs[1][1]=uv.y;
                            uvs[2][0]=uv.x; uvs[2][1]=uv.y; uvs[3][0]=uv.x; uvs[3][1]=uv.y+uv.height;
                            break;
                        case 2:
                            verts[0][0]=fx; verts[0][1]=fy+1; verts[0][2]=fz;
                            verts[1][0]=fx; verts[1][1]=fy+1; verts[1][2]=fz+1;
                            verts[2][0]=fx+1; verts[2][1]=fy+1; verts[2][2]=fz+1;
                            verts[3][0]=fx+1; verts[3][1]=fy+1; verts[3][2]=fz;
                            uvs[0][0]=uv.x; uvs[0][1]=uv.y+uv.height; uvs[1][0]=uv.x+uv.width; uvs[1][1]=uv.y+uv.height;
                            uvs[2][0]=uv.x+uv.width; uvs[2][1]=uv.y; uvs[3][0]=uv.x; uvs[3][1]=uv.y;
                            break;
                        case 3:
                            verts[0][0]=fx; verts[0][1]=fy; verts[0][2]=fz+1;
                            verts[1][0]=fx; verts[1][1]=fy; verts[1][2]=fz;
                            verts[2][0]=fx+1; verts[2][1]=fy; verts[2][2]=fz;
                            verts[3][0]=fx+1; verts[3][1]=fy; verts[3][2]=fz+1;
                            uvs[0][0]=uv.x; uvs[0][1]=uv.y+uv.height; uvs[1][0]=uv.x+uv.width; uvs[1][1]=uv.y+uv.height;
                            uvs[2][0]=uv.x+uv.width; uvs[2][1]=uv.y; uvs[3][0]=uv.x; uvs[3][1]=uv.y;
                            break;
                        case 4:
                            verts[0][0]=fx+1; verts[0][1]=fy;   verts[0][2]=fz+1;
                            verts[1][0]=fx+1; verts[1][1]=fy+1; verts[1][2]=fz+1;
                            verts[2][0]=fx; verts[2][1]=fy+1; verts[2][2]=fz+1;
                            verts[3][0]=fx; verts[3][1]=fy;   verts[3][2]=fz+1;
                            uvs[0][0]=uv.x; uvs[0][1]=uv.y+uv.height; uvs[1][0]=uv.x; uvs[1][1]=uv.y;
                            uvs[2][0]=uv.x+uv.width; uvs[2][1]=uv.y; uvs[3][0]=uv.x+uv.width; uvs[3][1]=uv.y+uv.height;
                            break;
                        case 5:
                            verts[0][0]=fx; verts[0][1]=fy;   verts[0][2]=fz;
                            verts[1][0]=fx; verts[1][1]=fy+1; verts[1][2]=fz;
                            verts[2][0]=fx+1; verts[2][1]=fy+1; verts[2][2]=fz;
                            verts[3][0]=fx+1; verts[3][1]=fy;   verts[3][2]=fz;
                            uvs[0][0]=uv.x+uv.width; uvs[0][1]=uv.y+uv.height; uvs[1][0]=uv.x+uv.width; uvs[1][1]=uv.y;
                            uvs[2][0]=uv.x; uvs[2][1]=uv.y; uvs[3][0]=uv.x; uvs[3][1]=uv.y+uv.height;
                            break;
                    }

                    for (int i = 0; i < 4; i++) {
                        pendingVertices.push_back(verts[i][0]);
                        pendingVertices.push_back(verts[i][1]);
                        pendingVertices.push_back(verts[i][2]);
                        unsigned char shade = FACE_BRIGHTNESS[f];
                        pendingColors.push_back(shade);
                        pendingColors.push_back(shade);
                        pendingColors.push_back(shade);
                        pendingColors.push_back(255);
                        pendingTexcoords.push_back(uvs[i][0]);
                        pendingTexcoords.push_back(uvs[i][1]);
                    }

                    pendingIndices.push_back(vertexIndex);
                    pendingIndices.push_back(vertexIndex + 1);
                    pendingIndices.push_back(vertexIndex + 2);
                    pendingIndices.push_back(vertexIndex);
                    pendingIndices.push_back(vertexIndex + 2);
                    pendingIndices.push_back(vertexIndex + 3);
                    vertexIndex += 4;
                }
            }
        }
    }
}

void Chunk::UploadToGPU() {
    std::unique_lock chunkLock(chunkMutex);

    UnloadMesh();

    if (!pendingVertices.empty()) {
        mesh.vertexCount = static_cast<int>(pendingVertices.size() / 3);
        mesh.triangleCount = static_cast<int>(pendingIndices.size() / 3);

        mesh.vertices  = (float*)MemAlloc(pendingVertices.size() * sizeof(float));
        mesh.texcoords = (float*)MemAlloc(pendingTexcoords.size() * sizeof(float));
        mesh.colors    = (unsigned char*)MemAlloc(pendingColors.size() * sizeof(unsigned char));
        mesh.indices   = (unsigned short*)MemAlloc(pendingIndices.size() * sizeof(unsigned short));

        memcpy(mesh.vertices,  pendingVertices.data(),  pendingVertices.size() * sizeof(float));
        memcpy(mesh.texcoords, pendingTexcoords.data(), pendingTexcoords.size() * sizeof(float));
        memcpy(mesh.colors,    pendingColors.data(),    pendingColors.size() * sizeof(unsigned char));
        memcpy(mesh.indices,   pendingIndices.data(),   pendingIndices.size() * sizeof(unsigned short));

        UploadMesh(&mesh, false);
        model = LoadModelFromMesh(mesh);
        model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = TextureAtlas::GetTexture();
        meshBuilt = true;
    } else {
        meshBuilt = false;
    }

    // FIX: если во время меша блок менялся — вернуть Generated,
    // иначе загрузка завершена.
    if (needsRemesh.load()) {
        state = ChunkState::Generated;
    } else {
        state = ChunkState::Loaded;
    }
}

void Chunk::Draw() const {
    if (meshBuilt && mesh.vertexCount > 0) {
        Vector3 worldPos = { (float)(cx * CHUNK_SIZE), 0.0f, (float)(cz * CHUNK_SIZE) };
        DrawModel(model, worldPos, 1.0f, WHITE);
    }
}

void Chunk::UnloadMesh() {
    if (meshBuilt) {
        UnloadModel(model);
        meshBuilt = false;
    }
    model = { 0 };
    mesh  = { 0 };
}
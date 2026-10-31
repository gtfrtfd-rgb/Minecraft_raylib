#pragma once
#include "raylib.h"
#include "block.h"
#include "config.h"
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>
#include <shared_mutex>

enum class ChunkState {
    Empty, Generating, Generated, Meshing, MeshReady, Loaded
};

class Chunk {
public:
    Chunk(int cx, int cz);
    ~Chunk();

    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

    void GenerateData();
    void BuildMeshData();
    void UploadToGPU();
    void Draw() const;

    BlockType GetBlock(int x, int y, int z) const;
    void SetBlock(int x, int y, int z, BlockType type);
    void MarkDirty();

    std::atomic<ChunkState> state;

    std::vector<float> pendingVertices;
    std::vector<float> pendingTexcoords;
    std::vector<unsigned char> pendingColors;
    std::vector<unsigned short> pendingIndices;

    int GetCX() const { return cx; }
    int GetCZ() const { return cz; }

private:
    int cx, cz;
    BlockType blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];
    Mesh mesh = {0};
    Model model = {0};
    bool meshBuilt = false;

    std::mutex chunkMutex;                 // pending-векторы
    mutable std::shared_mutex blocksMutex; // массив blocks
    std::atomic<bool> needsRemesh{false};

    void UnloadMesh();
};
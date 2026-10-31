#pragma once
#include "chunk.h"
#include <unordered_map>
#include <memory>
#include <shared_mutex>
#include <mutex>
#include <vector>

class World {
public:
    static void Init();
    static void Shutdown(); // ⬅️ Новый метод
    static void Update(Vector3 playerPos);
    static void Draw(Camera3D camera);

    static BlockType GetBlock(int x, int y, int z);
    static void SetBlock(int x, int y, int z, BlockType type);
    static Vector3 Raycast(Vector3 origin, Vector3 direction, float maxDistance, bool& hit, Vector3& hitNormal);
    static std::shared_ptr<Chunk> GetOrCreateChunk(int cx, int cz);

private:
    static std::unordered_map<long long, std::shared_ptr<Chunk>> chunks;
    static std::vector<std::shared_ptr<Chunk>> chunksToDelete;
    static std::shared_mutex chunksMutex;
    static long long Hash(int cx, int cz);
    static std::shared_ptr<Chunk> GetChunk(int cx, int cz);
    static void UnloadDistantChunks(Vector3 playerPos);
};
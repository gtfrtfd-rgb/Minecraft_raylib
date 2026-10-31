#include "world.h"
#include "config.h"
#include "raymath.h"
#include "thread_pool.h"
#include <cmath>
#include <algorithm>
#include <vector>

std::unordered_map<long long, std::shared_ptr<Chunk>> World::chunks;
std::vector<std::shared_ptr<Chunk>> World::chunksToDelete;
std::shared_mutex World::chunksMutex;
static std::unique_ptr<ThreadPool> pool; // ⬅️ Умный указатель для управления жизнью

long long World::Hash(int cx, int cz) {
    unsigned long long ucx = static_cast<unsigned int>(cx);
    unsigned long long ucz = static_cast<unsigned int>(cz);
    return static_cast<long long>((ucx << 32) | ucz);
}

void World::Init() {
    std::unique_lock lock(chunksMutex);
    chunks.clear();
    chunksToDelete.clear();
    pool = std::make_unique<ThreadPool>(std::thread::hardware_concurrency() > 1 ? std::thread::hardware_concurrency() - 1 : 1);
}

void World::Shutdown() {
    if (pool) {
        pool->Shutdown(); // Останавливаем потоки
        pool.reset();
    }
    std::unique_lock lock(chunksMutex);
    chunks.clear(); // Деструкторы вызываются здесь, в главном потоке
    chunksToDelete.clear();
}

std::shared_ptr<Chunk> World::GetChunk(int cx, int cz) {
    std::shared_lock lock(chunksMutex);
    auto it = chunks.find(Hash(cx, cz));
    if (it != chunks.end()) return it->second;
    return nullptr;
}

std::shared_ptr<Chunk> World::GetOrCreateChunk(int cx, int cz) {
    long long key = Hash(cx, cz);
    {
        std::shared_lock lock(chunksMutex);
        auto it = chunks.find(key);
        if (it != chunks.end()) return it->second;
    }
    std::unique_lock lock(chunksMutex);
    auto it = chunks.find(key);
    if (it == chunks.end()) {
        auto chunk = std::make_shared<Chunk>(cx, cz);
        chunks[key] = chunk;
        return chunk;
    }
    return it->second;
}

void World::Update(Vector3 playerPos) {
    int pcx = static_cast<int>(std::floor(playerPos.x / CHUNK_SIZE));
    int pcz = static_cast<int>(std::floor(playerPos.z / CHUNK_SIZE));

    // ⬇️ Генерируем чанки с запасом (+1), чтобы соседи краевых чанков существовали ⬇️
    for (int dx = -RENDER_DISTANCE - 1; dx <= RENDER_DISTANCE + 1; dx++) {
        for (int dz = -RENDER_DISTANCE - 1; dz <= RENDER_DISTANCE + 1; dz++) {
            auto chunk = GetOrCreateChunk(pcx + dx, pcz + dz);
            if (chunk->state == ChunkState::Empty) {
                chunk->state = ChunkState::Generating;
                pool->enqueue([chunk]() { 
                    chunk->GenerateData(); 
                    chunk->state = ChunkState::Generated; 
                });
            }
        }
    }

    // ⬇️ Мешим только чанки в радиусе видимости ⬇️
    for (int dx = -RENDER_DISTANCE; dx <= RENDER_DISTANCE; dx++) {
        for (int dz = -RENDER_DISTANCE; dz <= RENDER_DISTANCE; dz++) {
            auto chunk = GetOrCreateChunk(pcx + dx, pcz + dz);
            if (chunk->state == ChunkState::Generated) {
                bool neighborsReady = true;
                int ndx[4] = {1, -1, 0, 0};
                int ndz[4] = {0, 0, 1, -1};
                for (int i = 0; i < 4; i++) {
                    auto neighbor = GetOrCreateChunk(pcx + dx + ndx[i], pcz + dz + ndz[i]);
                    if (neighbor->state == ChunkState::Empty || neighbor->state == ChunkState::Generating) {
                        neighborsReady = false;
                        break;
                    }
                }
                if (neighborsReady) {
                    chunk->state = ChunkState::Meshing;
                    pool->enqueue([chunk]() { 
                        chunk->BuildMeshData(); 
                    });
                }
            }
        }
    }

    {
        std::shared_lock lock(chunksMutex);
        for (auto& [key, chunk] : chunks) {
            if (chunk->state.load() == ChunkState::MeshReady) {
                chunk->UploadToGPU();
            }
        }
    }

    UnloadDistantChunks(playerPos);

    // Безопасная очистка удаленных чанков (только в главном потоке)
    if (!chunksToDelete.empty()) {
        chunksToDelete.clear();
    }
}

void World::Draw(Camera3D camera) {
    std::shared_lock lock(chunksMutex);
    Vector3 camForward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    const float chunkRadius = 32.5f;

    for (auto& [key, chunk] : chunks) {
        Vector3 chunkCenter = { chunk->GetCX() * CHUNK_SIZE + CHUNK_SIZE * 0.5f, CHUNK_HEIGHT * 0.5f, chunk->GetCZ() * CHUNK_SIZE + CHUNK_SIZE * 0.5f };
        Vector3 dirToChunk = Vector3Subtract(chunkCenter, camera.position);
        if (Vector3DotProduct(camForward, dirToChunk) < -chunkRadius) continue; 
        chunk->Draw();
    }
}

BlockType World::GetBlock(int x, int y, int z) {
    if (y < 0 || y >= CHUNK_HEIGHT) return BlockType::Air;
    int cx = static_cast<int>(std::floor(static_cast<float>(x) / CHUNK_SIZE));
    int cz = static_cast<int>(std::floor(static_cast<float>(z) / CHUNK_SIZE));
    int lx = x - cx * CHUNK_SIZE, lz = z - cz * CHUNK_SIZE;

    std::shared_lock lock(chunksMutex); 
    auto it = chunks.find(Hash(cx, cz));
    if (it == chunks.end()) return BlockType::Air;
    return it->second->GetBlock(lx, y, lz);
}

void World::SetBlock(int x, int y, int z, BlockType type) {
    if (y < 0 || y >= CHUNK_HEIGHT) return;
    int cx = static_cast<int>(std::floor(static_cast<float>(x) / CHUNK_SIZE));
    int cz = static_cast<int>(std::floor(static_cast<float>(z) / CHUNK_SIZE));
    int lx = x - cx * CHUNK_SIZE, lz = z - cz * CHUNK_SIZE;

    auto chunk = GetOrCreateChunk(cx, cz);
    
    // ⬇️ ФИКС ПОТЕРИ БЛОКОВ: Если чанк пустой, генерируем синхронно ⬇️
    if (chunk->state == ChunkState::Empty) {
        chunk->GenerateData();
        chunk->state = ChunkState::Generated;
    }

    chunk->SetBlock(lx, y, lz, type);
    chunk->MarkDirty();

    auto markNeighbor = [&](int ncx, int ncz) {
        auto c = GetChunk(ncx, ncz);
        if (c) c->MarkDirty();
    };
    if (lx == 0) markNeighbor(cx - 1, cz);
    if (lx == CHUNK_SIZE - 1) markNeighbor(cx + 1, cz);
    if (lz == 0) markNeighbor(cx, cz - 1);
    if (lz == CHUNK_SIZE - 1) markNeighbor(cx, cz + 1);
}

Vector3 World::Raycast(Vector3 origin, Vector3 direction, float maxDistance, bool& hit, Vector3& hitNormal) {
    hit = false;
    hitNormal = {0, 0, 0};
    for (float t = 0.001f; t < maxDistance; t += 0.05f) { // ⬅️ Сдвиг начала
        Vector3 checkPos = Vector3Add(origin, Vector3Scale(direction, t));
        int bx = static_cast<int>(std::floor(checkPos.x));
        int by = static_cast<int>(std::floor(checkPos.y));
        int bz = static_cast<int>(std::floor(checkPos.z));
        if (GetBlock(bx, by, bz) != BlockType::Air) {
            hit = true;
            Vector3 local = { checkPos.x - bx - 0.5f, checkPos.y - by - 0.5f, checkPos.z - bz - 0.5f };
            float ax = fabsf(local.x), ay = fabsf(local.y), az = fabsf(local.z);
            if (ax > ay && ax > az)      hitNormal = { (local.x > 0) ? 1.0f : -1.0f, 0, 0 };
            else if (ay > az)             hitNormal = { 0, (local.y > 0) ? 1.0f : -1.0f, 0 };
            else                          hitNormal = { 0, 0, (local.z > 0) ? 1.0f : -1.0f };
            return { (float)bx, (float)by, (float)bz };
        }
    }
    return {0, 0, 0};
}

void World::UnloadDistantChunks(Vector3 playerPos) {
    int pcx = static_cast<int>(std::floor(playerPos.x / CHUNK_SIZE));
    int pcz = static_cast<int>(std::floor(playerPos.z / CHUNK_SIZE));

    std::unique_lock lock(chunksMutex);
    std::vector<long long> toRemove;
    for (auto& [key, chunk] : chunks) {
        int dx = chunk->GetCX() - pcx, dz = chunk->GetCZ() - pcz;
        if (std::abs(dx) > RENDER_DISTANCE + 2 || std::abs(dz) > RENDER_DISTANCE + 2) {
            ChunkState s = chunk->state.load();
            // ⬇️ ФИКС: Удаляем только если на чанк больше нет ссылок в других потоках ⬇️
            if (s != ChunkState::Generating && s != ChunkState::Meshing && chunk.use_count() == 1) {
                auto markNeighbor = [&](int ncx, int ncz) {
                    auto it = chunks.find(Hash(ncx, ncz));
                    if (it != chunks.end()) it->second->MarkDirty();
                };
                markNeighbor(chunk->GetCX() + 1, chunk->GetCZ());
                markNeighbor(chunk->GetCX() - 1, chunk->GetCZ());
                markNeighbor(chunk->GetCX(), chunk->GetCZ() + 1);
                markNeighbor(chunk->GetCX(), chunk->GetCZ() - 1);
                
                toRemove.push_back(key);
            }
        }
    }
    for (long long key : toRemove) {
        chunksToDelete.push_back(chunks[key]);
        chunks.erase(key);
    }
}
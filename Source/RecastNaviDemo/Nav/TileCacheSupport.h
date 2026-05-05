#pragma once
/*
 * Nav/TileCacheSupport.h
 * ----------------------
 * TileCache 所需的三个接口实现：
 *   - LinearAllocator  : dtTileCacheAlloc  — 线性内存池
 *   - FastLZCompressor : dtTileCacheCompressor — 字节复制（无压缩）
 *   - NavMeshProcess   : dtTileCacheMeshProcess — 为多边形写入行走标志
 *
 * 这些类体积小、无外部依赖，直接以 inline 定义在头文件中。
 * 参考自 Recast Demo Sample_TempObstacles.cpp 的经典实现模式。
 */

#include "DetourTileCacheBuilder.h"   // dtTileCacheAlloc, dtTileCacheCompressor, dtTileCacheMeshProcess
#include "DetourNavMeshBuilder.h"     // dtNavMeshCreateParams
#include "DetourCommon.h"             // dtMax
#include "DetourAlloc.h"

#include <cstring>   // memcpy
#include <cstdlib>   // malloc / free

// =============================================================================
// LinearAllocator — 快速线性池，每帧 reset() 后重用
// =============================================================================
struct LinearAllocator : public dtTileCacheAlloc
{
    unsigned char* buffer   = nullptr;
    size_t         capacity = 0;
    size_t         top      = 0;
    size_t         high     = 0;   ///< 高水位（调试用）

    explicit LinearAllocator(const size_t cap)
        : capacity(cap)
    {
        buffer = static_cast<unsigned char*>(dtAlloc(cap, DT_ALLOC_PERM));
    }
    ~LinearAllocator() override { dtFree(buffer); }

    void reset() override
    {
        high = dtMax(high, top);
        top  = 0;
    }

    void* alloc(const size_t size) override
    {
        if (!buffer) return nullptr;
        if (top + size > capacity) return nullptr;
        unsigned char* mem = buffer + top;
        top += size;
        return mem;
    }

    void free(void* /*ptr*/) override { /* 线性分配器不支持单次释放 */ }
};

// =============================================================================
// FastLZCompressor — 无压缩实现（直接 memcpy）
// 生产项目中可替换为 fastlz / lz4 以节省内存
// =============================================================================
struct FastLZCompressor : public dtTileCacheCompressor
{
    int maxCompressedSize(const int bufferSize) override
    {
        return bufferSize + (bufferSize / 255) + 16;
    }

    dtStatus compress(const unsigned char* buffer, const int bufferSize,
                      unsigned char* compressed, const int /*maxCompressedSize*/,
                      int* compressedSize) override
    {
        std::memcpy(compressed, buffer, bufferSize);
        *compressedSize = bufferSize;
        return DT_SUCCESS;
    }

    dtStatus decompress(const unsigned char* compressed, const int compressedSize,
                        unsigned char* buffer, const int /*maxBufferSize*/,
                        int* bufferSize) override
    {
        std::memcpy(buffer, compressed, compressedSize);
        *bufferSize = compressedSize;
        return DT_SUCCESS;
    }
};

// =============================================================================
// NavMeshProcess — TileCache 构建完每个 tile 时设置多边形标志
// =============================================================================
struct NavMeshProcess : public dtTileCacheMeshProcess
{
    void process(struct dtNavMeshCreateParams* params,
                 unsigned char* polyAreas, unsigned short* polyFlags) override
    {
        for (int i = 0; i < params->polyCount; ++i)
        {
            if (polyAreas[i] == DT_TILECACHE_WALKABLE_AREA)
                polyFlags[i] = 0x01;
        }
    }
};

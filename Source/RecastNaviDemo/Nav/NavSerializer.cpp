/*
 * Nav/NavSerializer.cpp
 * ---------------------
 * NavMesh 二进制序列化实现。
 *
 * 文件格式（HNV1，小端序）：
 *   [0..3]  uint32  magic   = 0x484E5631  ('H','N','V','1')
 *   [4..7]  uint32  version = 1
 *   [8..11] uint32  size    = navData 字节数
 *   [12..]  byte[]  navData = dtNavMesh tile 原始数据
 *
 * 内存所有权说明：
 *   dtNavMesh::init(buf, n, DT_TILE_FREE_DATA) 会在内部持有 buf 并在析构时 dtFree(buf)。
 *   因此 Load 时必须将文件内容拷贝到 dtAlloc 分配的独立缓冲区，
 *   同时在 runtime.NavMeshData 中保留一份副本供后续 "Save NavMesh" 使用。
 */

#include "NavSerializer.h"
#include "NavBuilder.h"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>

// Detour
#include "DetourAlloc.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "DetourStatus.h"

namespace NavSerializer
{

// HNV1 文件格式常量
static constexpr std::uint32_t kMagic   = 0x484E5631u;  // 'HNV1'
static constexpr std::uint32_t kVersion = 1u;

bool SaveNavMesh(const NavRuntime& runtime, const char* path, std::string& errOut)
{
    if (runtime.NavMeshData.empty())
    {
        errOut = "no navmesh data (build first)";
        return false;
    }

    std::ofstream fs(path, std::ios::binary);
    if (!fs.is_open())
    {
        errOut = "open failed";
        return false;
    }

    const std::uint32_t size = static_cast<std::uint32_t>(runtime.NavMeshData.size());

    fs.write(reinterpret_cast<const char*>(&kMagic),                 sizeof(kMagic));
    fs.write(reinterpret_cast<const char*>(&kVersion),               sizeof(kVersion));
    fs.write(reinterpret_cast<const char*>(&size),                   sizeof(size));
    fs.write(reinterpret_cast<const char*>(runtime.NavMeshData.data()), size);

    if (!fs)
    {
        errOut = "write failed";
        return false;
    }

    return true;
}

bool LoadNavMesh(NavRuntime& runtime, const char* path, std::string& errOut)
{
    std::ifstream fs(path, std::ios::binary);
    if (!fs.is_open())
    {
        errOut = "open failed";
        return false;
    }

    // 读取并校验文件头
    std::uint32_t magic = 0, version = 0, size = 0;
    fs.read(reinterpret_cast<char*>(&magic),   sizeof(magic));
    fs.read(reinterpret_cast<char*>(&version), sizeof(version));
    fs.read(reinterpret_cast<char*>(&size),    sizeof(size));

    if (magic != kMagic || version != kVersion || size == 0)
    {
        errOut = "bad header";
        return false;
    }

    // 读取 navData 到临时缓冲
    std::vector<unsigned char> buf(size);
    fs.read(reinterpret_cast<char*>(buf.data()), size);
    if (!fs)
    {
        errOut = "read failed";
        return false;
    }

    // 清理旧 runtime
    NavBuilder::DestroyNavRuntime(runtime);

    // 保留一份副本（NavMeshData），供后续 SaveNavMesh 使用
    runtime.NavMeshData = buf;

    // 为 dtNavMesh 分配独立的 dtAlloc 缓冲（DT_TILE_FREE_DATA 会在内部 dtFree 掉它）
    // 此处使用 InitNavMeshFromData，它会从 runtime.NavMeshData 再 memcpy 一份
    if (!NavBuilder::InitNavMeshFromData(runtime))
    {
        errOut = "InitNavMeshFromData failed";
        runtime.NavMeshData.clear();
        return false;
    }

    return true;
}

} // namespace NavSerializer

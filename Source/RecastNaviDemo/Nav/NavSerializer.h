#pragma once
/*
 * Nav/NavSerializer.h
 * -------------------
 * NavMesh 二进制序列化接口：
 *   - SaveNavMesh：把 runtime.NavMeshData 写入文件（HNV1 格式）
 *   - LoadNavMesh：从 HNV1 文件加载，重建 dtNavMesh + dtNavMeshQuery
 *
 * 文件格式（小端序，共 12 字节头 + 数据）：
 *   uint32  magic   = 0x484E5631  ('H','N','V','1')
 *   uint32  version = 1
 *   uint32  size    = navData 字节数
 *   byte[]  navData = 原始 dtNavMesh tile 数据
 *
 * 依赖：Nav/NavTypes.h、Nav/NavBuilder.h
 * 不依赖：UI/ 或 Render/
 */

#include "NavTypes.h"
#include <string>

namespace NavSerializer
{

/**
 * @brief 把 runtime.NavMeshData 写入二进制文件。
 *
 * @param runtime  已构建的 NavRuntime（NavMeshData 不能为空）
 * @param path     目标文件路径
 * @param errOut   失败时填写错误描述
 * @return true 表示成功
 */
bool SaveNavMesh(const NavRuntime& runtime, const char* path, std::string& errOut);

/**
 * @brief 从二进制文件加载 NavMesh，重建 runtime 中的 NavMesh / NavQuery。
 *
 * 调用前 runtime 中的旧数据会被 DestroyNavRuntime 清理。
 * 成功后 runtime.bBuilt == true，runtime.NavMeshData 保存一份副本。
 *
 * @param runtime  目标 NavRuntime（会被完全重置后重建）
 * @param path     源文件路径
 * @param errOut   失败时填写错误描述
 * @return true 表示成功
 */
bool LoadNavMesh(NavRuntime& runtime, const char* path, std::string& errOut);

} // namespace NavSerializer

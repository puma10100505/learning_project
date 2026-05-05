#pragma once
/*
 * UI/FileDialog.h
 * ---------------
 * 平台条件文件对话框封装。
 *
 * 提供：
 *   - ShowOpenFileDialog  — 打开文件选择器，写入用户选择的路径
 *   - ShowSaveFileDialog  — 保存文件选择器，写入用户指定的路径
 *   - EllipsizePath       — 将过长路径裁短显示（保留尾部）
 *
 * 平台支持：
 *   _WIN32    → GetOpenFileNameA / GetSaveFileNameA (commdlg.h)
 *   __APPLE__ → osascript AppleScript (通过 popen)
 *   __linux__ → zenity (推荐) 或 kdialog fallback (通过 popen)
 *   其他      → 存根，始终返回 false
 *
 * 依赖：
 *   - <cstdio>   (popen / fgets / pclose — POSIX 平台)
 *   - <string>   (EllipsizePath 返回值)
 *   - <windows.h>（仅 Win32）
 *
 * 不依赖：AppState.h、NavTypes.h、imgui.h
 */

#include <cstddef>   // size_t
#include <string>

namespace FileDialog
{

/// 打开文件对话框。成功时将路径写入 outPath[0..outSize-1] 并返回 true。
/// @param title   对话框标题（Win32 使用；其他平台忽略）
/// @param filter  文件过滤字符串（Win32 格式: "OBJ\0*.obj\0All\0*.*\0\0"；其他平台忽略）
/// @param outPath 输出缓冲区（调用前可预填默认路径）
/// @param outSize 缓冲区字节数
bool ShowOpenFileDialog(const char* title, const char* filter,
                        char* outPath, size_t outSize);

/// 保存文件对话框。成功时将路径写入 outPath[0..outSize-1] 并返回 true。
/// @param title   对话框标题（Win32 使用；其他平台忽略）
/// @param filter  文件过滤字符串（Win32 使用；其他平台忽略）
/// @param defExt  默认扩展名，不含点（Win32: "bin"；macOS: 构造默认文件名）
/// @param outPath 输出缓冲区（调用前可预填默认路径）
/// @param outSize 缓冲区字节数
bool ShowSaveFileDialog(const char* title, const char* filter, const char* defExt,
                        char* outPath, size_t outSize);

/// 将过长路径裁短以供 UI 显示，保留尾部字符。
/// 若 path 为空/null 返回 "(none)"；长度超过 maxLen 时返回 "..." + 尾部。
std::string EllipsizePath(const char* path, size_t maxLen = 56);

} // namespace FileDialog

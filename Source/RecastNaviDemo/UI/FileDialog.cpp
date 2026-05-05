/*
 * UI/FileDialog.cpp
 * -----------------
 * 平台条件文件对话框封装实现。
 * 详细说明见 FileDialog.h。
 */

#include "FileDialog.h"

#include <cstring>    // strncpy, strlen
#include <cstdio>     // snprintf, popen, fgets, pclose (POSIX)
#include <string>

// =============================================================================
// Win32
// =============================================================================
#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  include <commdlg.h>

namespace FileDialog
{

bool ShowOpenFileDialog(const char* title, const char* filter,
                        char* outPath, size_t outSize)
{
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = GetActiveWindow();
    ofn.lpstrFilter = filter;        // e.g. "OBJ Files\0*.obj\0All Files\0*.*\0\0"
    ofn.lpstrFile   = outPath;
    ofn.nMaxFile    = static_cast<DWORD>(outSize);
    ofn.lpstrTitle  = title;
    ofn.Flags       = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    return GetOpenFileNameA(&ofn) == TRUE;
}

bool ShowSaveFileDialog(const char* title, const char* filter, const char* defExt,
                        char* outPath, size_t outSize)
{
    OPENFILENAMEA ofn{};
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = GetActiveWindow();
    ofn.lpstrFilter  = filter;
    ofn.lpstrFile    = outPath;
    ofn.nMaxFile     = static_cast<DWORD>(outSize);
    ofn.lpstrTitle   = title;
    ofn.lpstrDefExt  = defExt;
    ofn.Flags        = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    return GetSaveFileNameA(&ofn) == TRUE;
}

} // namespace FileDialog

// =============================================================================
// macOS
// =============================================================================
#elif defined(__APPLE__)

namespace FileDialog
{

/// 内部辅助：执行 shell 命令，读取第一行输出并去掉行尾换行符。
static bool PopenReadTrimmedPath(const char* command, char* outPath, size_t outSize)
{
    if (!outSize) return false;
    outPath[0] = '\0';
    if (!command) return false;

    FILE* pipe = ::popen(command, "r");
    if (!pipe) return false;

    if (!std::fgets(outPath, static_cast<int>(outSize), pipe))
    {
        ::pclose(pipe);
        return false;
    }
    const int rc = ::pclose(pipe);
    if (rc != 0) return false;

    // 去掉行尾 \n \r
    for (char* p = outPath; *p; ++p)
    {
        if (*p == '\n' || *p == '\r')
        {
            *p = '\0';
            break;
        }
    }
    return outPath[0] != '\0';
}

bool ShowOpenFileDialog(const char* /*title*/, const char* /*filter*/,
                        char* outPath, size_t outSize)
{
    return PopenReadTrimmedPath(
        "/usr/bin/osascript"
        " -e 'try'"
        " -e 'set f to (choose file)'"
        " -e 'return POSIX path of f'"
        " -e 'on error'"
        " -e 'return \"\"'"
        " -e 'end try'",
        outPath, outSize);
}

bool ShowSaveFileDialog(const char* /*title*/, const char* /*filter*/, const char* defExt,
                        char* outPath, size_t outSize)
{
    char defName[64] = "navmesh.bin";
    if (defExt && defExt[0])
        std::snprintf(defName, sizeof(defName), "untitled.%s", defExt);

    char script[512];
    std::snprintf(
        script, sizeof(script),
        "/usr/bin/osascript"
        " -e 'try'"
        " -e 'set p to (choose file name default name \"%s\")'"
        " -e 'return POSIX path of p'"
        " -e 'on error'"
        " -e 'return \"\"'"
        " -e 'end try'",
        defName);

    if (!PopenReadTrimmedPath(script, outPath, outSize))
    {
        if (outSize) outPath[0] = '\0';
        return false;
    }
    return true;
}

} // namespace FileDialog

// =============================================================================
// Linux
// =============================================================================
#elif defined(__linux__)

namespace FileDialog
{

/// 内部辅助：执行 shell 命令，读取第一行输出并去掉行尾换行符。
/// 与 macOS 版本的区别：不检查 pclose 返回值（zenity 在用户取消时返回非零）。
static bool PopenReadTrimmedZenity(const char* command, char* outPath, size_t outSize)
{
    if (!outSize) return false;
    outPath[0] = '\0';
    if (!command) return false;

    FILE* pipe = std::popen(command, "r");
    if (!pipe) return false;

    if (!std::fgets(outPath, static_cast<int>(outSize), pipe))
    {
        std::pclose(pipe);
        return false;
    }
    (void)std::pclose(pipe);

    // 去掉行尾 \n \r
    for (char* p = outPath; *p; ++p)
    {
        if (*p == '\n' || *p == '\r')
        {
            *p = '\0';
            break;
        }
    }
    return outPath[0] != '\0';
}

bool ShowOpenFileDialog(const char* /*title*/, const char* /*filter*/,
                        char* outPath, size_t outSize)
{
    if (!outPath || !outSize) return false;
    outPath[0] = '\0';

    // zenity 优先，kdialog 兜底
    if (PopenReadTrimmedZenity("zenity --file-selection 2>/dev/null", outPath, outSize))
        return true;
    return PopenReadTrimmedZenity("kdialog --getopenfilename $HOME 2>/dev/null", outPath, outSize);
}

bool ShowSaveFileDialog(const char* /*title*/, const char* /*filter*/, const char* /*defExt*/,
                        char* outPath, size_t outSize)
{
    if (!outPath || !outSize) return false;
    outPath[0] = '\0';

    if (PopenReadTrimmedZenity("zenity --file-selection --save --confirm-overwrite 2>/dev/null",
                               outPath, outSize))
        return true;
    return PopenReadTrimmedZenity("kdialog --getsavefilename $HOME 2>/dev/null", outPath, outSize);
}

} // namespace FileDialog

// =============================================================================
// Fallback (unsupported platform)
// =============================================================================
#else

namespace FileDialog
{

bool ShowOpenFileDialog(const char*, const char*, char* outPath, size_t outSize)
{
    if (outSize) outPath[0] = '\0';
    return false;
}

bool ShowSaveFileDialog(const char*, const char*, const char*, char* outPath, size_t outSize)
{
    if (outSize) outPath[0] = '\0';
    return false;
}

} // namespace FileDialog

#endif // platform

// =============================================================================
// EllipsizePath — platform-independent
// =============================================================================

namespace FileDialog
{

std::string EllipsizePath(const char* path, size_t maxLen)
{
    if (!path || !*path) return "(none)";
    std::string s(path);
    if (s.size() <= maxLen) return s;
    return std::string("...") + s.substr(s.size() - (maxLen - 3));
}

} // namespace FileDialog

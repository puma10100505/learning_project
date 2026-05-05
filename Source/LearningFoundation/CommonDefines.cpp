#include "CommonDefines.h"

#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#if defined(__APPLE__)
#  include <mach-o/dyld.h>
#  include <climits>
#elif defined(__linux__)
#  include <climits>
#  include <unistd.h>
#elif defined(_WIN32)
#  include <windows.h>
#endif

namespace {
namespace fs = std::filesystem;

static std::string with_trailing_slash(const fs::path& p) {
    if (p.empty()) {
        return std::string("./");
    }
    std::string s = p.generic_string();
    if (!s.empty() && s.back() != '/') {
        s += '/';
    }
    return s;
}

static std::string get_executable_path_raw() {
#if defined(__APPLE__)
    char pathbuf[PATH_MAX];
    std::uint32_t size = static_cast<std::uint32_t>(sizeof(pathbuf));
    if (_NSGetExecutablePath(pathbuf, &size) == 0) {
        if (char* r = realpath(pathbuf, nullptr)) {
            std::string out = r;
            std::free(r);
            return out;
        }
        return std::string(pathbuf);
    }
    if (size > static_cast<std::uint32_t>(sizeof(pathbuf))) {
        std::vector<char> buf(size);
        if (_NSGetExecutablePath(buf.data(), &size) == 0) {
            if (char* r = realpath(buf.data(), nullptr)) {
                std::string out = r;
                std::free(r);
                return out;
            }
            return std::string(buf.data());
        }
    }
    return {};
#elif defined(__linux__)
    char path[4096];
    const ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len < 0) {
        return {};
    }
    path[len] = '\0';
    return std::string(path);
#elif defined(_WIN32)
    char path[MAX_PATH];
    const DWORD n = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) {
        return {};
    }
    return std::string(path);
#else
    return {};
#endif
}

static std::string find_dir_with_assets(fs::path start) {
    try {
        if (fs::is_regular_file(start)) {
            start = start.parent_path();
        } else {
            start = fs::absolute(start);
        }
        for (int depth = 0; depth < 64 && !start.empty(); ++depth) {
            if (fs::is_directory(start / "Assets")) {
                return with_trailing_slash(start);
            }
            const fs::path parent = start.parent_path();
            if (parent == start) {
                break;
            }
            start = parent;
        }
    } catch (const fs::filesystem_error&) {
    }
    return {};
}

static std::string resolve_solution_base_path() {
    const std::string exe = get_executable_path_raw();
    if (!exe.empty()) {
        const std::string s = find_dir_with_assets(exe);
        if (!s.empty()) {
            return s;
        }
    }
    try {
        const std::string s = find_dir_with_assets(fs::current_path());
        if (!s.empty()) {
            return s;
        }
    } catch (const fs::filesystem_error&) {
    }
    try {
        return with_trailing_slash(fs::current_path());
    } catch (const fs::filesystem_error&) {
        return std::string("./");
    }
}

static std::wstring narrow_to_wide(const std::string& s) {
#if defined(_WIN32)
    if (s.empty()) {
        return std::wstring();
    }
    return fs::path(s).lexically_normal().wstring();
#else
    std::wstring w;
    w.reserve(s.size());
    for (unsigned char c : s) {
        w += static_cast<wchar_t>(c);
    }
    return w;
#endif
}
} // namespace

const std::string& GetSolutionBasePath() {
    static const std::string k = resolve_solution_base_path();
    return k;
}

const std::wstring& GetWideSolutionBasePath() {
    static const std::wstring w = [] {
        const std::string& n = GetSolutionBasePath();
        return narrow_to_wide(n);
    }();
    return w;
}

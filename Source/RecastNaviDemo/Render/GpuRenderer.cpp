/*
 * Render/GpuRenderer.cpp
 * ----------------------
 * GPU/Shader 实现：FBO 离屏 + GLSL 330 core + (位置, 颜色, 法线, lit 标志) 顶点。
 *
 * 着色：
 *   - 三角形：CPU 端逐面计算 face normal（背面剔除 + 平面光照），lit=1。
 *   - 线段：展开成 view-space 细四边形，lit=0（不参与光照，颜色直显）。
 *   - 片元：vCol.rgb * mix(1.0, ambient + (1-ambient)*max(N·L,0), vLit)
 *
 * MSAA：
 *   - samples > 1：渲到多采样 FBO（颜色 + 深度都是 multisample renderbuffer），
 *     End() 时 glBlitFramebuffer 解析到 colorTex 供 ImGui 采样；
 *   - samples = 1：直接渲到 colorTex（depth 用单采样 renderbuffer）。
 * 合成：主 pass 关闭 GL_BLEND、片元 a=1，避免半透明与深度写入叠加导致遮挡观感错误。
 *
 * 平台：!_WIN32 走 OpenGL 3.3 Core；_WIN32 走 D3D12（详见 GpuRendererD3D12.inl）。
 *
 * Shader 源码：从外部文件加载，文件位于 Render/Shaders/ 目录下：
 *   - recast_demo.vs  (顶点着色器)
 *   - recast_demo.fs  (片段着色器)
 *   路径解析：先以 __FILE__ 同目录的 Shaders/ 为优先（开发期，与源码同步无视 cwd），
 *           失败再依次尝试若干 cwd 相对路径（部署期常见目录）。两轮都找不到
 *           会返回 false，GpuShader 模式回退（UI 层应感知 Available()）。
 */

#include "GpuRenderer.h"

#include <algorithm>
#include <cmath>

#if defined(_WIN32)

#include "GpuRendererD3D12.inl"

#else // !_WIN32

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <glad/glad.h>

namespace GpuRenderer
{
namespace
{

// -----------------------------------------------------------------------------
// 着色器文件加载
//   Shader 源码以独立文件存放在 Render/Shaders/ 下（recast_demo.vs / .fs），
//   方便不重新编译 C++ 即可调整光照公式。下面的 LoadShaderSource 负责按一组
//   候选路径找到文件并整文件读出。
// -----------------------------------------------------------------------------
constexpr const char* kVsBasename       = "recast_demo.vs";
constexpr const char* kFsBasename       = "recast_demo.fs";
constexpr const char* kShadowVsBasename = "recast_shadow.vs";
constexpr const char* kShadowFsBasename = "recast_shadow.fs";

std::string ReadWholeFile(const std::filesystem::path& path)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return {};
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

// 候选搜索路径优先级（依序尝试，第一个 exists() 为准）：
//   1. <__FILE__-dir>/Shaders/<base>
//      —— 开发期最稳：编译期就把源文件路径烧进二进制，与 cwd 无关。
//   2. cwd/Source/RecastNaviDemo/Render/Shaders/<base>
//      —— 从工程根启动 (cmake --build && ./build_*/.../RecastNaviDemo) 时命中。
//   3. cwd/Render/Shaders/<base>     —— 从 Source/RecastNaviDemo 启动时命中。
//   4. cwd/Shaders/<base>            —— 部署后 Shaders/ 直接放在 exe 同级时命中。
std::filesystem::path FindShaderFile(const char* basename)
{
    namespace fs = std::filesystem;
    std::error_code ec;

    const fs::path here = fs::path(__FILE__).parent_path();
    const fs::path candidates[] = {
        here / "Shaders" / basename,
        fs::current_path(ec) / "Source" / "RecastNaviDemo" / "Render" / "Shaders" / basename,
        fs::current_path(ec) / "Render" / "Shaders" / basename,
        fs::current_path(ec) / "Shaders" / basename,
    };

    for (const auto& p : candidates)
    {
        if (fs::exists(p, ec) && fs::is_regular_file(p, ec))
            return p;
    }
    return {};
}

// 读取一支 shader 文件的源码到 outSrc。返回 true 成功；失败时记 stderr。
bool LoadShaderSource(const char* basename, std::string& outSrc, std::string& outResolvedPath)
{
    const std::filesystem::path resolved = FindShaderFile(basename);
    if (resolved.empty())
    {
        std::fprintf(stderr,
                     "[GpuRenderer] shader file not found: %s\n"
                     "  searched: <__FILE__-dir>/Shaders/, cwd/Source/RecastNaviDemo/Render/Shaders/, cwd/Render/Shaders/, cwd/Shaders/\n",
                     basename);
        outResolvedPath.clear();
        return false;
    }

    std::string content = ReadWholeFile(resolved);
    if (content.empty())
    {
        std::fprintf(stderr, "[GpuRenderer] shader file empty or unreadable: %s\n",
                     resolved.string().c_str());
        outResolvedPath = resolved.string();
        return false;
    }

    outSrc          = std::move(content);
    outResolvedPath = resolved.string();
    return true;
}

// -----------------------------------------------------------------------------
// 顶点结构 & 全局状态
// -----------------------------------------------------------------------------
struct Vertex
{
    float x, y, z;     // 0
    float r, g, b, a;  // 12
    float nx, ny, nz;  // 28
    float lit;         // 40
};

struct State
{
    bool   on        = false;
    bool   shaderOk  = false;

    int    rw        = 0;
    int    rh        = 0;
    int    fboW      = 0;
    int    fboH      = 0;
    int    samples   = 1;   // 当前 FBO 配置的 MSAA 采样数（实际生效值）

    GLuint fboMs     = 0;   // 多采样目标 FBO（仅 samples>1）
    GLuint colorMsRb = 0;
    GLuint depthMsRb = 0;
    GLuint fboResolve = 0;  // 单采样解析 FBO（绑定 colorTex；samples=1 也用作渲染目标）
    GLuint colorTex  = 0;   // 提供给 ImGui 采样
    GLuint depthRb   = 0;   // 仅 samples=1 时给 fboResolve 用

    GLuint vao       = 0;
    GLuint vbo       = 0;
    GLuint program   = 0;
    GLint  uVPLoc           = -1;
    GLint  uLightTypeLoc    = -1;
    GLint  uLightVecLoc     = -1;
    GLint  uLightColorLoc   = -1;
    GLint  uLightIntenLoc   = -1;
    GLint  uAmbientLoc      = -1;
    // 主 program 中的阴影相关 uniform
    GLint  uMainLightVPLoc    = -1;
    GLint  uMainShadowMapLoc  = -1;
    GLint  uMainShadowsOnLoc  = -1;
    GLint  uMainShadowStrLoc  = -1;
    GLint  uMainShadowBiasLoc = -1;
    GLint  uMainShadowPcfLoc  = -1;

    // Shadow pass 资源
    GLuint shadowProgram      = 0;
    GLint  uShadowLightVPLoc  = -1;
    GLuint shadowFbo          = 0;
    GLuint shadowDepthTex     = 0;
    int    shadowMapSize      = 0;     // 当前已分配的尺寸（边长）
    Mat4   lightVP{};                  // Begin 中根据光方向 + sceneCenter 计算

    Mat4        vp{};
    float       fovy      = 0.0f;
    Vec3        eye{};
    float       worldDiag = 1.0f;
    LightParams light{};

    std::vector<Vertex> verts;

    // Begin 时保存的 GL 状态
    GLint  savedFbo      = 0;
    GLint  savedViewport[4]    {};
    GLint  savedScissorBox[4]  {};
    GLboolean savedScissor   = GL_FALSE;
    GLboolean savedDepthTest = GL_FALSE;
    GLboolean savedBlend     = GL_FALSE;
    GLboolean savedCull      = GL_FALSE;
    GLint  savedActiveTex    = 0;
    GLint  savedTexBind2D    = 0;     // unit 0
    GLint  savedTexBind2D_u1 = 0;     // unit 1（shadow map 用）
    GLint  savedProgram      = 0;
    GLint  savedVao          = 0;
    GLint  savedArrayBuf     = 0;
    GLfloat savedClearColor[4] {};
};

State g_S;

// -----------------------------------------------------------------------------
// Shader / Program
// -----------------------------------------------------------------------------
GLuint CompileStage(GLenum type, const char* src)
{
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint ok = GL_FALSE;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[2048] = {};
        GLsizei len = 0;
        glGetShaderInfoLog(sh, sizeof(log) - 1, &len, log);
        std::fprintf(stderr, "[GpuRenderer] shader compile failed (%s):\n%s\n",
                     type == GL_VERTEX_SHADER ? "VS" : "FS", log);
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

// 通用：加载 vs/fs → 编译 → 绑定属性 → 链接，成功返回 program id，失败返回 0
GLuint BuildProgram(const char* vsBase, const char* fsBase)
{
    std::string vsSrc, fsSrc, vsPath, fsPath;
    if (!LoadShaderSource(vsBase, vsSrc, vsPath)) return 0;
    if (!LoadShaderSource(fsBase, fsSrc, fsPath)) return 0;
    std::fprintf(stderr,
                 "[GpuRenderer] program loaded:\n  VS: %s\n  FS: %s\n",
                 vsPath.c_str(), fsPath.c_str());

    GLuint vs = CompileStage(GL_VERTEX_SHADER,   vsSrc.c_str());
    GLuint fs = CompileStage(GL_FRAGMENT_SHADER, fsSrc.c_str());
    if (!vs || !fs) { if (vs) glDeleteShader(vs); if (fs) glDeleteShader(fs); return 0; }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    // 主 / shadow 程序共用同一个 VBO 布局，因此都按这套属性绑定即可。
    // shadow 着色器只引用 aPos + aLit，多余的绑定不影响。
    glBindAttribLocation(prog, 0, "aPos");
    glBindAttribLocation(prog, 1, "aCol");
    glBindAttribLocation(prog, 2, "aNormal");
    glBindAttribLocation(prog, 3, "aLit");
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[2048] = {};
        GLsizei len = 0;
        glGetProgramInfoLog(prog, sizeof(log) - 1, &len, log);
        std::fprintf(stderr, "[GpuRenderer] program link failed (vs=%s fs=%s):\n%s\n",
                     vsBase, fsBase, log);
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

bool LinkProgram()
{
    // ---- 主着色程序 ----
    if (!g_S.program)
    {
        GLuint prog = BuildProgram(kVsBasename, kFsBasename);
        if (!prog) return false;
        g_S.program            = prog;
        g_S.uVPLoc             = glGetUniformLocation(prog, "uVP");
        g_S.uLightTypeLoc      = glGetUniformLocation(prog, "uLightType");
        g_S.uLightVecLoc       = glGetUniformLocation(prog, "uLightVec");
        g_S.uLightColorLoc     = glGetUniformLocation(prog, "uLightColor");
        g_S.uLightIntenLoc     = glGetUniformLocation(prog, "uLightIntensity");
        g_S.uAmbientLoc        = glGetUniformLocation(prog, "uAmbient");
        g_S.uMainLightVPLoc    = glGetUniformLocation(prog, "uLightVP");
        g_S.uMainShadowMapLoc  = glGetUniformLocation(prog, "uShadowMap");
        g_S.uMainShadowsOnLoc  = glGetUniformLocation(prog, "uShadowsOn");
        g_S.uMainShadowStrLoc  = glGetUniformLocation(prog, "uShadowStrength");
        g_S.uMainShadowBiasLoc = glGetUniformLocation(prog, "uShadowBias");
        g_S.uMainShadowPcfLoc  = glGetUniformLocation(prog, "uShadowPcf");
    }

    // ---- Shadow 深度 pass 程序 ----
    if (!g_S.shadowProgram)
    {
        GLuint prog = BuildProgram(kShadowVsBasename, kShadowFsBasename);
        if (!prog) return false;
        g_S.shadowProgram     = prog;
        g_S.uShadowLightVPLoc = glGetUniformLocation(prog, "uLightVP");
    }

    return true;
}

void EnsureBuffers()
{
    if (g_S.vao) return;
    glGenVertexArrays(1, &g_S.vao);
    glGenBuffers(1, &g_S.vbo);
    glBindVertexArray(g_S.vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_S.vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, r));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, nx));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, lit));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// -----------------------------------------------------------------------------
// FBO / RBO 管理
// -----------------------------------------------------------------------------
void DestroyFbo()
{
    if (g_S.fboMs)      { glDeleteFramebuffers (1, &g_S.fboMs);      g_S.fboMs      = 0; }
    if (g_S.colorMsRb)  { glDeleteRenderbuffers(1, &g_S.colorMsRb);  g_S.colorMsRb  = 0; }
    if (g_S.depthMsRb)  { glDeleteRenderbuffers(1, &g_S.depthMsRb);  g_S.depthMsRb  = 0; }
    if (g_S.fboResolve) { glDeleteFramebuffers (1, &g_S.fboResolve); g_S.fboResolve = 0; }
    if (g_S.colorTex)   { glDeleteTextures     (1, &g_S.colorTex);   g_S.colorTex   = 0; }
    if (g_S.depthRb)    { glDeleteRenderbuffers(1, &g_S.depthRb);    g_S.depthRb    = 0; }
}

void EnsureFbo(int rw, int rh, int samples)
{
    if (g_S.fboW == rw && g_S.fboH == rh && g_S.samples == samples
        && g_S.fboResolve != 0)
        return;

    DestroyFbo();
    g_S.fboW    = rw;
    g_S.fboH    = rh;
    g_S.samples = samples;

    // 单采样颜色纹理（永远存在；ImGui 通过它采样最终图）
    glGenTextures(1, &g_S.colorTex);
    glBindTexture(GL_TEXTURE_2D, g_S.colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rw, rh, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (samples > 1)
    {
        // ---- 多采样：渲染目标 FBO 用 multisample renderbuffer ----
        glGenRenderbuffers(1, &g_S.colorMsRb);
        glBindRenderbuffer(GL_RENDERBUFFER, g_S.colorMsRb);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, rw, rh);

        glGenRenderbuffers(1, &g_S.depthMsRb);
        glBindRenderbuffer(GL_RENDERBUFFER, g_S.depthMsRb);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT24, rw, rh);

        glGenFramebuffers(1, &g_S.fboMs);
        glBindFramebuffer(GL_FRAMEBUFFER, g_S.fboMs);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                  GL_RENDERBUFFER, g_S.colorMsRb);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, g_S.depthMsRb);

        // 解析 FBO（仅作 blit 目标，附 colorTex）
        glGenFramebuffers(1, &g_S.fboResolve);
        glBindFramebuffer(GL_FRAMEBUFFER, g_S.fboResolve);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, g_S.colorTex, 0);
    }
    else
    {
        // ---- 单采样：直接渲到 colorTex；depth 用单采样 RBO ----
        glGenRenderbuffers(1, &g_S.depthRb);
        glBindRenderbuffer(GL_RENDERBUFFER, g_S.depthRb);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, rw, rh);

        glGenFramebuffers(1, &g_S.fboResolve);
        glBindFramebuffer(GL_FRAMEBUFFER, g_S.fboResolve);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, g_S.colorTex, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, g_S.depthRb);
    }

    GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (st != GL_FRAMEBUFFER_COMPLETE)
        std::fprintf(stderr, "[GpuRenderer] FBO incomplete (samples=%d): 0x%x\n",
                     samples, st);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// -----------------------------------------------------------------------------
// Shadow FBO / 深度纹理
//   - 单张正方形深度纹理 (GL_DEPTH_COMPONENT24)，clamp_to_border + 白色边
//     → 出 light frustum 的采样自动按 1.0（最远，恒不被遮挡）处理。
//   - 没有 color attachment：glDrawBuffer/ReadBuffer = NONE。
// -----------------------------------------------------------------------------
void DestroyShadowFbo()
{
    if (g_S.shadowFbo)      { glDeleteFramebuffers(1, &g_S.shadowFbo);      g_S.shadowFbo      = 0; }
    if (g_S.shadowDepthTex) { glDeleteTextures   (1, &g_S.shadowDepthTex);  g_S.shadowDepthTex = 0; }
    g_S.shadowMapSize = 0;
}

void EnsureShadowFbo(int size)
{
    if (g_S.shadowMapSize == size && g_S.shadowFbo != 0 && g_S.shadowDepthTex != 0) return;

    DestroyShadowFbo();

    glGenTextures(1, &g_S.shadowDepthTex);
    glBindTexture(GL_TEXTURE_2D, g_S.shadowDepthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
                 size, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float border[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

    glGenFramebuffers(1, &g_S.shadowFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, g_S.shadowFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_2D, g_S.shadowDepthTex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (st != GL_FRAMEBUFFER_COMPLETE)
        std::fprintf(stderr, "[GpuRenderer] shadow FBO incomplete (size=%d): 0x%x\n", size, st);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    g_S.shadowMapSize = size;
}

// -----------------------------------------------------------------------------
// 光源 VP 矩阵：右手系，光看向 -light_dir，正交投影框 ≈ ±sceneRadius²
// -----------------------------------------------------------------------------
Mat4 ComputeDirectionalLightVP(Vec3 lightDirWorld, Vec3 sceneCenter, float sceneRadius)
{
    Vec3 dir = Normalize(lightDirWorld);
    if (Length(dir) < 1e-6f) dir = V3(0, 1, 0);

    const float r        = std::max(sceneRadius, 1.0f);
    const Vec3  lightPos = sceneCenter + dir * (r * 1.5f);
    // 选一个"几乎不与 dir 平行"的 up
    Vec3 up = (std::fabs(dir.y) > 0.95f) ? V3(0, 0, 1) : V3(0, 1, 0);

    const Mat4 view = MakeLookAtRH(lightPos, sceneCenter, up);
    // 正交框：平面 ±r·1.2，深度 [0.1, 3·r]，覆盖整个场景
    const float ext  = r * 1.2f;
    const Mat4  proj = MakeOrthoRH(-ext, ext, -ext, ext, 0.1f, 3.0f * r);
    return MatMul(proj, view);
}

// -----------------------------------------------------------------------------
// 颜色 / 顶点构造
// -----------------------------------------------------------------------------
inline void UnpackCol(ImU32 col, float& r, float& g, float& b, float& a)
{
    r = static_cast<float>((col >> IM_COL32_R_SHIFT) & 0xFF) * (1.0f / 255.0f);
    g = static_cast<float>((col >> IM_COL32_G_SHIFT) & 0xFF) * (1.0f / 255.0f);
    b = static_cast<float>((col >> IM_COL32_B_SHIFT) & 0xFF) * (1.0f / 255.0f);
    a = static_cast<float>((col >> IM_COL32_A_SHIFT) & 0xFF) * (1.0f / 255.0f);
}

inline void EmitVert(Vec3 p, float r, float g, float b, float a,
                     Vec3 n, float lit)
{
    g_S.verts.push_back({ p.x, p.y, p.z, r, g, b, a, n.x, n.y, n.z, lit });
}

inline void EmitTri(Vec3 a, Vec3 b, Vec3 c, ImU32 col, Vec3 n, float lit)
{
    float r, g, bl, al;
    UnpackCol(col, r, g, bl, al);
    EmitVert(a, r, g, bl, al, n, lit);
    EmitVert(b, r, g, bl, al, n, lit);
    EmitVert(c, r, g, bl, al, n, lit);
}

} // namespace

// -----------------------------------------------------------------------------
// 公共接口
// -----------------------------------------------------------------------------
bool Available() { return true; }
bool IsActive()  { return g_S.on; }
int  BufferWidth()  { return g_S.fboW; }
int  BufferHeight() { return g_S.fboH; }
ImTextureID TextureId() { return (ImTextureID)(intptr_t)g_S.colorTex; }

void Begin(int rw, int rh, ImU32 clearColor, const Mat4& vp, float fovyRad,
           const Vec3& eyeWorld, float worldBoundsDiagonal, int samples,
           const LightParams& light)
{
    rw = std::max(1, rw);
    rh = std::max(1, rh);
    constexpr int kMaxDim = 4096;
    rw = std::min(rw, kMaxDim);
    rh = std::min(rh, kMaxDim);

    if (!g_S.shaderOk)
        g_S.shaderOk = LinkProgram();
    EnsureBuffers();

    // 把 samples 夹到驱动支持的范围
    GLint maxSamples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    if (maxSamples < 1) maxSamples = 1;
    samples = std::clamp(samples, 1, maxSamples);

    EnsureFbo(rw, rh, samples);

    g_S.rw        = rw;
    g_S.rh        = rh;
    g_S.vp        = vp;
    g_S.fovy      = fovyRad;
    g_S.eye       = eyeWorld;
    g_S.worldDiag = std::max(worldBoundsDiagonal, 1.0f);
    g_S.light     = light;
    g_S.verts.clear();
    g_S.on        = true;

    // ---- 阴影资源准备：仅 Directional + 开关启用时才走 shadow map ----
    const bool shadowsActive =
        light.shadowsOn && light.type == 0 && g_S.shaderOk && g_S.shadowProgram != 0;
    if (shadowsActive)
    {
        const int shadowSize = std::clamp(light.shadowMapSize, 64, 4096);
        EnsureShadowFbo(shadowSize);
        g_S.lightVP = ComputeDirectionalLightVP(light.vec, light.sceneCenter, light.sceneRadius);
    }
    else
    {
        // 主 shader 仍引用 uLightVP，给个单位阵兜底（uShadowsOn=0 时不会真去采样）
        g_S.lightVP = MakeIdentity();
    }

    // ---- 保存 GL 状态 ----
    glGetIntegerv (GL_FRAMEBUFFER_BINDING,    &g_S.savedFbo);
    glGetIntegerv (GL_VIEWPORT,                g_S.savedViewport);
    glGetIntegerv (GL_SCISSOR_BOX,             g_S.savedScissorBox);
    g_S.savedScissor   = glIsEnabled(GL_SCISSOR_TEST);
    g_S.savedDepthTest = glIsEnabled(GL_DEPTH_TEST);
    g_S.savedBlend     = glIsEnabled(GL_BLEND);
    g_S.savedCull      = glIsEnabled(GL_CULL_FACE);
    glGetIntegerv (GL_ACTIVE_TEXTURE,         &g_S.savedActiveTex);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv (GL_TEXTURE_BINDING_2D,     &g_S.savedTexBind2D);
    glActiveTexture(GL_TEXTURE1);
    glGetIntegerv (GL_TEXTURE_BINDING_2D,     &g_S.savedTexBind2D_u1);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv (GL_CURRENT_PROGRAM,        &g_S.savedProgram);
    glGetIntegerv (GL_VERTEX_ARRAY_BINDING,   &g_S.savedVao);
    glGetIntegerv (GL_ARRAY_BUFFER_BINDING,   &g_S.savedArrayBuf);
    glGetFloatv   (GL_COLOR_CLEAR_VALUE,       g_S.savedClearColor);

    // ---- 切到目标 FBO + 清屏 ----
    const GLuint target = (samples > 1) ? g_S.fboMs : g_S.fboResolve;
    glBindFramebuffer(GL_FRAMEBUFFER, target);
    glViewport(0, 0, rw, rh);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    // 关闭混合：碰撞 tint / 半透明 ImU32 若与 SRC_ALPHA 混合，会与深度写入叠加产生错误的“前后层”观感。
    // 离屏 RGBA 按不透明输出（见 recast_demo.fs 中 a=1），由深度测试唯一决定遮挡。
    glDisable(GL_BLEND);
    if (samples > 1) glEnable(GL_MULTISAMPLE);

    float cr, cg, cb, ca;
    UnpackCol(clearColor, cr, cg, cb, ca);
    glClearColor(cr, cg, cb, ca);
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void End()
{
    if (!g_S.on) return;
    g_S.on = false;

    const bool hasGeom        = !g_S.verts.empty() && g_S.shaderOk;
    const bool shadowsActive  = g_S.light.shadowsOn && g_S.light.type == 0
                                && g_S.shadowProgram != 0 && g_S.shadowFbo != 0;

    // ---- 把当帧顶点上传到 VBO（两遍 pass 共用同一份数据） ----
    if (hasGeom)
    {
        glBindVertexArray(g_S.vao);
        glBindBuffer(GL_ARRAY_BUFFER, g_S.vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(g_S.verts.size() * sizeof(Vertex)),
                     g_S.verts.data(), GL_STREAM_DRAW);
    }

    // ---- Pass 1: shadow map 深度 pass ----
    if (hasGeom && shadowsActive)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, g_S.shadowFbo);
        glViewport(0, 0, g_S.shadowMapSize, g_S.shadowMapSize);
        glEnable (GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        // 深度 pass 关掉颜色写入；FBO 本身也无 color attachment
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glClearDepth(1.0);
        glClear(GL_DEPTH_BUFFER_BIT);

        glUseProgram(g_S.shadowProgram);
        if (g_S.uShadowLightVPLoc >= 0)
            glUniformMatrix4fv(g_S.uShadowLightVPLoc, 1, GL_TRUE, &g_S.lightVP.m[0][0]);

        glBindVertexArray(g_S.vao);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(g_S.verts.size()));

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

    // ---- Pass 2: 主彩色 pass ----
    {
        const GLuint target = (g_S.samples > 1) ? g_S.fboMs : g_S.fboResolve;
        glBindFramebuffer(GL_FRAMEBUFFER, target);
        glViewport(0, 0, g_S.fboW, g_S.fboH);
        glEnable (GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
    }

    if (hasGeom)
    {
        glUseProgram(g_S.program);
        // Mat4 行主序 → glUniformMatrix4fv(GL_TRUE) 让 GL 转列主序给 GLSL mat4
        glUniformMatrix4fv(g_S.uVPLoc, 1, GL_TRUE, &g_S.vp.m[0][0]);
        if (g_S.uMainLightVPLoc >= 0)
            glUniformMatrix4fv(g_S.uMainLightVPLoc, 1, GL_TRUE, &g_S.lightVP.m[0][0]);

        // ---- 光照 uniform 上传 ----
        const LightParams& L = g_S.light;
        if (g_S.uLightTypeLoc  >= 0) glUniform1i (g_S.uLightTypeLoc,  L.type);
        if (g_S.uLightVecLoc   >= 0) glUniform3f (g_S.uLightVecLoc,   L.vec.x,   L.vec.y,   L.vec.z);
        if (g_S.uLightColorLoc >= 0) glUniform3f (g_S.uLightColorLoc, L.color.x, L.color.y, L.color.z);
        if (g_S.uLightIntenLoc >= 0) glUniform1f (g_S.uLightIntenLoc, L.intensity);
        if (g_S.uAmbientLoc    >= 0) glUniform1f (g_S.uAmbientLoc,    L.ambient);

        // ---- Shadow uniform 上传 + sampler 绑定 ----
        const int shadowsOn = shadowsActive ? 1 : 0;
        if (g_S.uMainShadowsOnLoc  >= 0) glUniform1i(g_S.uMainShadowsOnLoc,  shadowsOn);
        if (g_S.uMainShadowStrLoc  >= 0) glUniform1f(g_S.uMainShadowStrLoc,  L.shadowStrength);
        if (g_S.uMainShadowBiasLoc >= 0) glUniform1f(g_S.uMainShadowBiasLoc, L.shadowBias);
        if (g_S.uMainShadowPcfLoc  >= 0) glUniform1i(g_S.uMainShadowPcfLoc,  L.shadowPcf);
        // sampler2D uShadowMap 绑到 TEXTURE1（TEXTURE0 留给 ImGui 默认采样路径）
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, shadowsActive ? g_S.shadowDepthTex : 0);
        if (g_S.uMainShadowMapLoc >= 0) glUniform1i(g_S.uMainShadowMapLoc, 1);
        glActiveTexture(GL_TEXTURE0);

        glBindVertexArray(g_S.vao);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(g_S.verts.size()));
    }

    // ---- MSAA 解析：fboMs → fboResolve(colorTex) ----
    if (g_S.samples > 1 && g_S.fboMs && g_S.fboResolve)
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, g_S.fboMs);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_S.fboResolve);
        glBlitFramebuffer(0, 0, g_S.fboW, g_S.fboH,
                          0, 0, g_S.fboW, g_S.fboH,
                          GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }

    // ---- 恢复 GL 状态 ----
    glBindBuffer (GL_ARRAY_BUFFER,            (GLuint)g_S.savedArrayBuf);
    glBindVertexArray((GLuint)g_S.savedVao);
    glUseProgram ((GLuint)g_S.savedProgram);
    // 还原两个 texture unit
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, (GLuint)g_S.savedTexBind2D_u1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, (GLuint)g_S.savedTexBind2D);
    glActiveTexture(g_S.savedActiveTex);
    if (g_S.savedCull)      glEnable(GL_CULL_FACE);     else glDisable(GL_CULL_FACE);
    if (g_S.savedBlend)     glEnable(GL_BLEND);         else glDisable(GL_BLEND);
    if (!g_S.savedDepthTest) glDisable(GL_DEPTH_TEST); // ImGui 默认 disable
    if (g_S.savedScissor)   glEnable(GL_SCISSOR_TEST);  else glDisable(GL_SCISSOR_TEST);
    glScissor (g_S.savedScissorBox[0], g_S.savedScissorBox[1],
               g_S.savedScissorBox[2], g_S.savedScissorBox[3]);
    glViewport(g_S.savedViewport[0],  g_S.savedViewport[1],
               g_S.savedViewport[2],  g_S.savedViewport[3]);
    glClearColor(g_S.savedClearColor[0], g_S.savedClearColor[1],
                 g_S.savedClearColor[2], g_S.savedClearColor[3]);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)g_S.savedFbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)g_S.savedFbo);
    glBindFramebuffer(GL_FRAMEBUFFER,      (GLuint)g_S.savedFbo);
}

void DrawTriangleWorld(const Mat4& /*vp*/, Vec3 a, Vec3 b, Vec3 c, ImU32 col,
                       bool cullBackface)
{
    if (!g_S.on) return;

    // 计算 face normal（cross 出来法线由顶点序决定）
    const Vec3  e1 = b - a;
    const Vec3  e2 = c - a;
    Vec3        n  = Cross(e1, e2);
    const float nl = Length(n);
    if (nl < 1e-8f) return;
    n = n * (1.0f / nl);

    if (cullBackface)
    {
        const Vec3 ctr   = (a + b + c) * (1.0f / 3.0f);
        const Vec3 toEye = Normalize(g_S.eye - ctr);
        if (Dot(n, toEye) <= 0.0f) return;
    }

    EmitTri(a, b, c, col, n, 1.0f); // 三角全部走光照
}

void DrawLineWorld(const Mat4& /*vp*/, Vec3 a, Vec3 b, ImU32 col, float thicknessPixels)
{
    if (!g_S.on) return;
    const Vec3  ab  = b - a;
    const float len = Length(ab);
    if (len < 1e-6f) return;

    // 每端各算 halfWidth → 梯形 quad，屏幕宽度与线长解耦（恒定 thicknessPixels）。
    const float distA = std::max(Length(a - g_S.eye), 0.1f);
    const float distB = std::max(Length(b - g_S.eye), 0.1f);
    const float halfH = static_cast<float>(g_S.rh);
    const float k     =
        (2.0f * std::tan(g_S.fovy * 0.5f)) / std::max(halfH, 1.0f);
    const float floorW   = g_S.worldDiag * 1e-4f;
    const float halfWidA = std::max(thicknessPixels * k * 0.5f * distA, floorW);
    const float halfWidB = std::max(thicknessPixels * k * 0.5f * distB, floorW);

    const Vec3 eab   = ab * (1.0f / len);
    const Vec3 mid   = (a + b) * 0.5f;
    Vec3       toEye = g_S.eye - mid;
    if (Length(toEye) < 1e-4f) toEye = V3(0, 1, 0);
    toEye = Normalize(toEye);
    Vec3 side = Cross(eab, toEye);
    if (Length(side) < 1e-4f) side = Cross(eab, V3(0, 1, 0));
    if (Length(side) < 1e-4f) return;
    side = Normalize(side);
    const Vec3 sA = side * halfWidA;
    const Vec3 sB = side * halfWidB;

    const Vec3 p0 = a + sA;
    const Vec3 p1 = a - sA;
    const Vec3 p2 = b - sB;
    const Vec3 p3 = b + sB;

    // 线段不参与光照（lit=0），任意 normal 都行
    const Vec3 nDummy = V3(0, 1, 0);
    EmitTri(p0, p1, p2, col, nDummy, 0.0f);
    EmitTri(p0, p2, p3, col, nDummy, 0.0f);
}

} // namespace GpuRenderer

#endif // !_WIN32

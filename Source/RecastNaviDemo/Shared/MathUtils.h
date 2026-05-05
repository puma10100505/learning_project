#pragma once
/*
 * Shared/MathUtils.h
 * ------------------
 * 轻量数学库（无 glm 依赖）：Vec3、Mat4 及相关操作函数。
 *
 * 设计原则：
 *   - 仅头文件，所有函数均为 inline
 *   - 不依赖任何平台头文件，可在 Nav/、Render/、UI/ 等所有模块中引用
 */

#include <cmath>

// =============================================================================
// Vec3
// =============================================================================
struct Vec3 { float x, y, z; };

inline Vec3  V3(float x, float y, float z)             { return { x, y, z }; }
inline Vec3  operator+(Vec3 a, Vec3 b)                  { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline Vec3  operator-(Vec3 a, Vec3 b)                  { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline Vec3  operator*(Vec3 a, float s)                 { return { a.x * s, a.y * s, a.z * s }; }
inline float Dot(Vec3 a, Vec3 b)                        { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3  Cross(Vec3 a, Vec3 b)                      { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
inline float Length(Vec3 a)                             { return std::sqrt(Dot(a, a)); }
inline Vec3  Normalize(Vec3 a)
{
    float l = Length(a);
    return l > 1e-6f ? a * (1.0f / l) : V3(0, 0, 0);
}

// =============================================================================
// Mat4 — 4×4 矩阵，行主存储 (m[row][col])
// =============================================================================
struct Mat4 { float m[4][4]; };

inline Mat4 MakeIdentity()
{
    Mat4 r{};
    for (int i = 0; i < 4; ++i) r.m[i][i] = 1.0f;
    return r;
}

/// 右手坐标系 Look-At 视图矩阵
inline Mat4 MakeLookAtRH(Vec3 eye, Vec3 target, Vec3 up)
{
    Vec3 f = Normalize(target - eye);
    Vec3 s = Normalize(Cross(f, up));
    Vec3 u = Cross(s, f);
    Mat4 r{};
    r.m[0][0] =  s.x; r.m[0][1] =  s.y; r.m[0][2] =  s.z; r.m[0][3] = -Dot(s, eye);
    r.m[1][0] =  u.x; r.m[1][1] =  u.y; r.m[1][2] =  u.z; r.m[1][3] = -Dot(u, eye);
    r.m[2][0] = -f.x; r.m[2][1] = -f.y; r.m[2][2] = -f.z; r.m[2][3] =  Dot(f, eye);
    r.m[3][3] =  1.0f;
    return r;
}

/// 右手坐标系透视矩阵，NDC z 范围 [-1, 1]
inline Mat4 MakePerspectiveRH(float fovyRad, float aspect, float znear, float zfar)
{
    Mat4 r{};
    const float t  = std::tan(fovyRad * 0.5f);
    r.m[0][0] = 1.0f / (aspect * t);
    r.m[1][1] = 1.0f / t;
    r.m[2][2] = -(zfar + znear) / (zfar - znear);
    r.m[2][3] = -(2.0f * zfar * znear) / (zfar - znear);
    r.m[3][2] = -1.0f;
    return r;
}

/// 右手坐标系正交投影矩阵，NDC z 范围 [-1, 1]（与 GL 默认一致）。
/// 用于方向光 shadow map：光源固定看向 -Z（光空间），把 [l,r]×[b,t]×[-far,-near] 映射到 [-1,1]³。
inline Mat4 MakeOrthoRH(float l, float r, float b, float t, float znear, float zfar)
{
    Mat4 m{};
    m.m[0][0] =  2.0f / (r - l);
    m.m[1][1] =  2.0f / (t - b);
    m.m[2][2] = -2.0f / (zfar - znear);
    m.m[0][3] = -(r + l) / (r - l);
    m.m[1][3] = -(t + b) / (t - b);
    m.m[2][3] = -(zfar + znear) / (zfar - znear);
    m.m[3][3] =  1.0f;
    return m;
}

inline Mat4 MatMul(const Mat4& a, const Mat4& b)
{
    Mat4 r{};
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            r.m[i][j] = a.m[i][0]*b.m[0][j] + a.m[i][1]*b.m[1][j]
                      + a.m[i][2]*b.m[2][j] + a.m[i][3]*b.m[3][j];
    return r;
}

/// 把 (x,y,z,1) 变换到裁剪空间，结果写入 out[4]
inline void TransformPoint(const Mat4& m, Vec3 p, float out[4])
{
    out[0] = m.m[0][0]*p.x + m.m[0][1]*p.y + m.m[0][2]*p.z + m.m[0][3];
    out[1] = m.m[1][0]*p.x + m.m[1][1]*p.y + m.m[1][2]*p.z + m.m[1][3];
    out[2] = m.m[2][0]*p.x + m.m[2][1]*p.y + m.m[2][2]*p.z + m.m[2][3];
    out[3] = m.m[3][0]*p.x + m.m[3][1]*p.y + m.m[3][2]*p.z + m.m[3][3];
}

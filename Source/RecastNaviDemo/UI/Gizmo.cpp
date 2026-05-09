/*
 * UI/Gizmo.cpp
 * ------------
 * ImGuizmo 集成实现。详见 Gizmo.h。
 */

#include "Gizmo.h"

#include "../Nav/NavTypes.h"
#include "../Render/RenderTypes.h"

#include "ImGuizmo.h"

#include <algorithm>
#include <cmath>

namespace Gizmo
{

namespace
{

constexpr float kPi = 3.14159265358979323846f;

// -------------------------------------------------------------------------
// Mat4(row-major, m[row][col]) → ImGuizmo float[16](column-major OpenGL style)
// -------------------------------------------------------------------------
inline void RowToColMajor(const Mat4& m, float out[16])
{
    for (int col = 0; col < 4; ++col)
        for (int row = 0; row < 4; ++row)
            out[col * 4 + row] = m.m[row][col];
}

// -------------------------------------------------------------------------
// 由 Obstacle 合成模型矩阵 (列优先 float[16])。
// 模型空间中，单位 Box 半边 (1,1,1)：
//   - 原点 = obstacle 中心 (CX, BaseY + Height/2, CZ)
//   - 缩放 (SX, Height/2, SZ)（Box）或 (Radius, Height/2, Radius)（Cylinder）
//   - 旋转：绕 Y 轴 YawDeg
// -------------------------------------------------------------------------
void BuildObstacleMatrix(const Obstacle& o, float out[16])
{
    // 缩放
    float sx = 1.0f, sy = std::max(0.001f, o.Height * 0.5f), sz = 1.0f;
    if (o.Shape == ObstacleShape::Box)
    {
        sx = std::max(0.001f, o.SX);
        sz = std::max(0.001f, o.SZ);
    }
    else
    {
        sx = sz = std::max(0.001f, o.Radius);
    }

    const float rad = o.YawDeg * kPi / 180.0f;
    const float c = std::cos(rad);
    const float s = std::sin(rad);

    // 旋转 * 缩放（列优先存储）
    // R = | c 0 s |   S = diag(sx, sy, sz)
    //     | 0 1 0 |
    //     |-s 0 c |
    // M = R * S：列 0 = (c*sx, 0, -s*sx)，列 1 = (0, sy, 0)，列 2 = (s*sz, 0, c*sz)，列 3 = (tx, ty, tz)
    const float tx = o.CX;
    const float ty = o.BaseY + o.Height * 0.5f;
    const float tz = o.CZ;

    out[ 0] =  c * sx; out[ 1] = 0.0f;   out[ 2] = -s * sx; out[ 3] = 0.0f;
    out[ 4] = 0.0f;    out[ 5] = sy;     out[ 6] = 0.0f;    out[ 7] = 0.0f;
    out[ 8] =  s * sz; out[ 9] = 0.0f;   out[10] =  c * sz; out[11] = 0.0f;
    out[12] = tx;      out[13] = ty;     out[14] = tz;      out[15] = 1.0f;
}

// -------------------------------------------------------------------------
// 从 ImGuizmo 列优先 float[16] 中读出 translation / 缩放 / Yaw。
// 假设矩阵形如 R_y(theta) * diag(sx, sy, sz) * T。
// -------------------------------------------------------------------------
void DecomposeYawScaleTrans(const float m[16],
                            float& outYawDeg,
                            float& outSX, float& outSY, float& outSZ,
                            float& outTX, float& outTY, float& outTZ)
{
    const float cx = m[ 0], cy = m[ 1], cz = m[ 2];      // col 0
    const float ux = m[ 4], uy = m[ 5], uz = m[ 6];      // col 1
    const float fx = m[ 8], fy = m[ 9], fz = m[10];      // col 2

    outSX = std::sqrt(cx * cx + cy * cy + cz * cz);
    outSY = std::sqrt(ux * ux + uy * uy + uz * uz);
    outSZ = std::sqrt(fx * fx + fy * fy + fz * fz);

    // 归一化 X 与 Z 列以提取 yaw（绕 Y 轴）
    const float nxx = (outSX > 1e-6f) ? cx / outSX : 1.0f;
    const float nxz = (outSX > 1e-6f) ? cz / outSX : 0.0f;
    // 反推：col0 = (cos, 0, -sin) → yaw = atan2(-nxz, nxx)
    outYawDeg = std::atan2(-nxz, nxx) * 180.0f / kPi;

    outTX = m[12];
    outTY = m[13];
    outTZ = m[14];
}

ImGuizmo::OPERATION ToOp(int v)
{
    switch (v)
    {
        case 1: return ImGuizmo::TRANSLATE;
        case 2: return ImGuizmo::ROTATE_Y;
        case 3: return ImGuizmo::SCALE;
        default: return static_cast<ImGuizmo::OPERATION>(0);
    }
}

} // anonymous namespace

bool IsOverGizmo()  { return ImGuizmo::IsOver(); }
bool IsUsingGizmo() { return ImGuizmo::IsUsing(); }

void OnFrameBegin()
{
    ImGuizmo::BeginFrame();
}

void HandleHotkeys(AppState& app)
{
    // 仅在没有 ImGui 输入控件聚焦 & 有选中障碍 & 处于 3D 视图时才接管 W/E/R/Q
    if (ImGui::IsAnyItemActive() || ImGui::GetIO().WantTextInput) return;
    if (app.CurrentViewMode != ViewMode::Orbit3D) return;
    if (app.SelectedObstacle < 0) return;
    if (!app.bGizmoEnabled) return;

    // Note: ImGui 1.85 没有 ImGuiKey 命名键；使用 ASCII 常量与 IsKeyPressed(int)
    if (ImGui::IsKeyPressed('W', false)) app.GizmoOp = 1;
    if (ImGui::IsKeyPressed('E', false)) app.GizmoOp = 2;
    if (ImGui::IsKeyPressed('R', false)) app.GizmoOp = 3;
    if (ImGui::IsKeyPressed('Q', false)) app.GizmoOp = 0;
}

void Draw(AppState& app, const ImVec2& canvasMin, const ImVec2& canvasSize)
{
    if (!app.bGizmoEnabled) return;
    if (app.GizmoOp <= 0)   return;
    if (app.CurrentViewMode != ViewMode::Orbit3D) return;
    if (!app.LastMap3D.bValid) return;
    if (app.SelectedObstacle < 0 ||
        app.SelectedObstacle >= static_cast<int>(app.Geom.Obstacles.size())) return;

    Obstacle& o = app.Geom.Obstacles[app.SelectedObstacle];

    // ImGuizmo 上下文：把绘制目标限制到画布矩形
    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
    ImGuizmo::SetRect(canvasMin.x, canvasMin.y, canvasSize.x, canvasSize.y);
    ImGuizmo::SetOrthographic(false);

    // 视图 / 投影矩阵：从 LastMap3D 读出（行优先），转 ImGuizmo 期望的列优先
    float view[16], proj[16];
    RowToColMajor(app.LastMap3D.View, view);
    RowToColMajor(app.LastMap3D.Proj, proj);

    // 模型矩阵：根据 obstacle 字段构建
    float model[16];
    BuildObstacleMatrix(o, model);

    const ImGuizmo::OPERATION op   = ToOp(app.GizmoOp);
    const ImGuizmo::MODE      mode = (op == ImGuizmo::ROTATE_Y || app.bGizmoLocal)
                                     ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

    // 吸附数组
    float snapVec[3] = { 0, 0, 0 };
    if (app.bGizmoSnap)
    {
        switch (op)
        {
            case ImGuizmo::TRANSLATE: snapVec[0] = snapVec[1] = snapVec[2] = app.GizmoSnapMove;  break;
            case ImGuizmo::ROTATE_Y:  snapVec[0] = app.GizmoSnapRot;                              break;
            case ImGuizmo::SCALE:     snapVec[0] = snapVec[1] = snapVec[2] = app.GizmoSnapScale; break;
            default: break;
        }
    }
    const float* snapPtr = app.bGizmoSnap ? snapVec : nullptr;

    const bool used = ImGuizmo::Manipulate(view, proj, op, mode, model,
                                           /*delta*/nullptr, snapPtr);

    if (used)
    {
        float yawDeg, sx, sy, sz, tx, ty, tz;
        DecomposeYawScaleTrans(model, yawDeg, sx, sy, sz, tx, ty, tz);

        // ---- 应用回 Obstacle ----
        // 平移 / Y 高度
        o.CX     = tx;
        o.CZ     = tz;
        o.Height = std::max(0.05f, sy * 2.0f);          // sy 是半高
        o.BaseY  = ty - o.Height * 0.5f;

        // 缩放
        if (o.Shape == ObstacleShape::Box)
        {
            o.SX = std::max(0.05f, sx);
            o.SZ = std::max(0.05f, sz);
        }
        else
        {
            // Cylinder：用 X/Z 缩放的平均值（最少 0.05）
            o.Radius = std::max(0.05f, 0.5f * (sx + sz));
        }

        // 旋转：仅 Y 轴
        if (op == ImGuizmo::ROTATE_Y)
        {
            // 把 yaw 归一到 [-180, 180]
            while (yawDeg >  180.0f) yawDeg -= 360.0f;
            while (yawDeg < -180.0f) yawDeg += 360.0f;
            o.YawDeg = yawDeg;
        }

        app.bGeomDirty = true;
    }
}

} // namespace Gizmo

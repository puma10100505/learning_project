/*
 * UI/Interaction.cpp
 * ------------------
 * 画布交互层实现。
 * 详细说明见 Interaction.h。
 */

#include "Interaction.h"

#include "../Nav/NavTypes.h"    // PointInsideObstacle
#include "../Phys/PhysWorld.h"

#include <cmath>
#include <limits>
#include <algorithm>

// Platform-conditional key codes
#if defined(_WIN32)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <Windows.h>
#endif
#if !defined(_WIN32)
#  include <GLFW/glfw3.h>
#endif

namespace Interaction
{

namespace
{

bool BuildOrbit3DViewRay(const AppState& app, const ImVec2& mp, Vec3& outEye, Vec3& outDir)
{
    if (!app.LastMap3D.bValid) return false;
    const float ndcX = ((mp.x - app.LastMap3D.ViewportMin.x) / app.LastMap3D.ViewportSize.x) * 2.0f - 1.0f;
    const float ndcY = 1.0f - ((mp.y - app.LastMap3D.ViewportMin.y) / app.LastMap3D.ViewportSize.y) * 2.0f;

    const Vec3 eye    = app.LastMap3D.EyeWorld;
    const Vec3 target = app.Cam.Target;
    const Vec3 fwd    = Normalize(target - eye);
    const Vec3 right  = Normalize(Cross(fwd, V3(0, 1, 0)));
    const Vec3 up     = Cross(right, fwd);
    const float aspect = app.LastMap3D.ViewportSize.x / app.LastMap3D.ViewportSize.y;
    const float th     = std::tan(app.Cam.Fovy * 0.5f);
    outEye             = eye;
    outDir             = Normalize(fwd + right * (ndcX * th * aspect) + up * (ndcY * th));
    return true;
}

} // namespace

// =============================================================================
// 低层射线拣取
// =============================================================================

bool RayTriangle(const Vec3& orig, const Vec3& dir,
                 const Vec3& v0,   const Vec3& v1, const Vec3& v2,
                 float& outT)
{
    const Vec3  e1   = v1 - v0;
    const Vec3  e2   = v2 - v0;
    const Vec3  pvec = Cross(dir, e2);
    const float det  = Dot(e1, pvec);
    if (std::fabs(det) < 1e-7f) return false;
    const float invDet = 1.0f / det;
    const Vec3  tvec   = orig - v0;
    const float u = Dot(tvec, pvec) * invDet;
    if (u < 0.0f || u > 1.0f) return false;
    const Vec3  qvec = Cross(tvec, e1);
    const float v = Dot(dir, qvec) * invDet;
    if (v < 0.0f || u + v > 1.0f) return false;
    const float t = Dot(e2, qvec) * invDet;
    if (t <= 1e-4f) return false;
    outT = t;
    return true;
}

bool RaycastInputMesh(const AppState& app,
                      const Vec3& orig, const Vec3& dir,
                      float& outX, float& outY, float& outZ)
{
    const int triCount = static_cast<int>(app.Geom.Triangles.size() / 3);
    if (triCount == 0) return false;
    float bestT = std::numeric_limits<float>::infinity();
    bool  hit   = false;
    for (int t = 0; t < triCount; ++t)
    {
        const int*   tri = &app.Geom.Triangles[t * 3];
        const float* va  = &app.Geom.Vertices[tri[0] * 3];
        const float* vb  = &app.Geom.Vertices[tri[1] * 3];
        const float* vc  = &app.Geom.Vertices[tri[2] * 3];
        float ht;
        if (RayTriangle(orig, dir,
                        V3(va[0], va[1], va[2]),
                        V3(vb[0], vb[1], vb[2]),
                        V3(vc[0], vc[1], vc[2]),
                        ht) && ht < bestT)
        {
            bestT = ht;
            hit   = true;
        }
    }
    if (!hit) return false;
    outX = orig.x + dir.x * bestT;
    outY = orig.y + dir.y * bestT;
    outZ = orig.z + dir.z * bestT;
    return true;
}

// =============================================================================
// 屏幕 → 世界坐标
// =============================================================================

bool ScreenTo2DWorld(const AppState& app, const ImVec2& mp,
                     float& wx, float& wy, float& wz)
{
    if (!app.LastMap2D.bValid) return false;
    const float* bmin = app.LastMap2D.Bmin;
    const float* bmax = app.LastMap2D.Bmax;
    const float fx = (mp.x - app.LastMap2D.CanvasMin.x) / app.LastMap2D.CanvasSize.x;
    const float fy = (mp.y - app.LastMap2D.CanvasMin.y) / app.LastMap2D.CanvasSize.y;
    wx = bmin[0] + fx * (bmax[0] - bmin[0]);
    wz = bmin[2] + (1.0f - fy) * (bmax[2] - bmin[2]);
    wy = (bmin[1] + bmax[1]) * 0.5f;  // 顶视图无法获得真实 Y, 以包围盒中心 Y 兜底
    return true;
}

bool ScreenTo3DGroundWorld(const AppState& app, const ImVec2& mp,
                           float& wx, float& wy, float& wz)
{
    Vec3 eye, dir;
    if (!BuildOrbit3DViewRay(app, mp, eye, dir)) return false;

    // PhysX：与导航输入网格（及障碍体）求最近命中
    if (PhysWorld::IsActive())
    {
        int         obsIdx = -1;
        const float o[3]   = { eye.x, eye.y, eye.z };
        const float d[3]   = { dir.x, dir.y, dir.z };
        if (PhysWorld::RaycastClosest(o, d, 1.0e6f, wx, wy, wz, obsIdx)) return true;
    }

    // OBJ: 真三角形 raycast, 拿到模型表面点 (含 Y)
    if (app.Geom.Source == GeomSource::ObjFile && !app.Geom.Triangles.empty())
    {
        if (RaycastInputMesh(app, eye, dir, wx, wy, wz)) return true;
    }

    // Procedural / fallback: 射线交 y = bmid 平面
    const float ymid = (app.Geom.Bounds[1] + app.Geom.Bounds[4]) * 0.5f;
    if (std::fabs(dir.y) < 1e-5f) return false;
    const float t = (ymid - eye.y) / dir.y;
    if (t <= 0.0f) return false;
    wx = eye.x + dir.x * t;
    wy = ymid;
    wz = eye.z + dir.z * t;
    return true;
}

bool PickGroundWorld(const AppState& app, const ImVec2& mp,
                     float& wx, float& wy, float& wz)
{
    if (app.CurrentViewMode == ViewMode::TopDown2D)
    {
        if (!ScreenTo2DWorld(app, mp, wx, wy, wz)) return false;
        if (PhysWorld::IsActive())
        {
            int   obsIgnored = -1;
            float hitY       = wy;
            if (PhysWorld::RaycastVerticalTopDown(wx, wz, app.Geom.Bounds, hitY, obsIgnored)) wy = hitY;
        }
        return true;
    }
    return ScreenTo3DGroundWorld(app, mp, wx, wy, wz);
}

int PickObstacleIndex(const AppState& app, const ImVec2& mp)
{
    if (!PhysWorld::IsActive())
    {
        float wx = 0.0f, wy = 0.0f, wz = 0.0f;
        if (!PickGroundWorld(app, mp, wx, wy, wz)) return -1;
        return FindObstacleAt(app, wx, wz);
    }

    if (app.CurrentViewMode == ViewMode::Orbit3D)
    {
        Vec3 eye, dir;
        if (!BuildOrbit3DViewRay(app, mp, eye, dir)) return -1;
        float hx = 0.f, hy = 0.f, hz = 0.f;
        int   obs = -1;
        const float o[3] = { eye.x, eye.y, eye.z };
        const float d[3] = { dir.x, dir.y, dir.z };
        if (PhysWorld::RaycastClosest(o, d, 1.0e6f, hx, hy, hz, obs)) return obs;
        return -1;
    }

    float wx = 0.f, wy = 0.f, wz = 0.f;
    if (!ScreenTo2DWorld(app, mp, wx, wy, wz)) return -1;
    int   obs = -1;
    float hitY = wy;
    if (PhysWorld::RaycastVerticalTopDown(wx, wz, app.Geom.Bounds, hitY, obs)) return obs;
    return FindObstacleAt(app, wx, wz);
}

// =============================================================================
// 障碍命中
// =============================================================================

int FindObstacleAt(const AppState& app, float wx, float wz)
{
    for (int i = static_cast<int>(app.Geom.Obstacles.size()) - 1; i >= 0; --i)
    {
        if (PointInsideObstacle(wx, wz, app.Geom.Obstacles[i])) return i;
    }
    return -1;
}

// =============================================================================
// 画布交互
// =============================================================================

void HandleCanvasInteraction(AppState& app,
                             const InteractionCallbacks& cb,
                             const ImVec2& /*canvasItemMin*/,
                             const ImVec2& /*canvasItemSize*/)
{
    // ImGui::InvisibleButton 已在外部布好, 这里只读取 hover/active 状态
    const bool       hovered = ImGui::IsItemHovered();
    const bool       active  = ImGui::IsItemActive();
    const ImGuiIO&   io      = ImGui::GetIO();
    const ImVec2     mp      = ImGui::GetMousePos();

    // 3D 相机操作 (右键拖拽旋转, 滚轮缩放) — 优先处理, 不被编辑模式拦截
    if (app.CurrentViewMode == ViewMode::Orbit3D)
    {
        if (hovered && io.MouseWheel != 0.0f)
        {
            app.Cam.Distance *= std::pow(1.1f, -io.MouseWheel);
            app.Cam.Distance = std::max(2.0f, std::min(800.0f, app.Cam.Distance));
        }
        if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f))
        {
            const ImVec2 d = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right, 0.0f);
            app.Cam.Yaw   += d.x * 0.005f;
            app.Cam.Pitch += d.y * 0.005f;
            app.Cam.Pitch  = std::max(-1.5f, std::min(1.5f, app.Cam.Pitch));
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
        }
    }

    if (!hovered && !active) return;

    // 当前世界坐标 (左键操作总是 ground pick)
    float wx = 0.0f, wy = 0.0f, wz = 0.0f;
    const bool havePick = PickGroundWorld(app, mp, wx, wy, wz);

    // -------- PlaceStart / PlaceEnd --------
    if (app.CurrentEditMode == EditMode::PlaceStart || app.CurrentEditMode == EditMode::PlaceEnd)
    {
        if (hovered && havePick && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            if (app.CurrentEditMode == EditMode::PlaceStart)
            { app.Start[0] = wx; app.Start[1] = wy; app.Start[2] = wz; }
            else
            { app.End[0]   = wx; app.End[1]   = wy; app.End[2]   = wz; }
            if (cb.TryAutoReplan) cb.TryAutoReplan();
        }
        return;
    }

    // -------- PlaceOffMeshStart / PlaceOffMeshEnd --------
    if (app.CurrentEditMode == EditMode::PlaceOffMeshStart)
    {
        if (hovered && havePick && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            // 记录起点，进入等待终点状态
            app.PendingOffMeshStart[0] = wx;
            app.PendingOffMeshStart[1] = wy;
            app.PendingOffMeshStart[2] = wz;
            app.CurrentEditMode = EditMode::PlaceOffMeshEnd;
        }
        return;
    }

    if (app.CurrentEditMode == EditMode::PlaceOffMeshEnd)
    {
        if (hovered && havePick && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            // 终点与起点有足够距离则创建 Link
            const float dx = wx - app.PendingOffMeshStart[0];
            const float dz = wz - app.PendingOffMeshStart[2];
            if (dx * dx + dz * dz > 0.25f)  // 最小 0.5 m
            {
                OffMeshLink lk;
                lk.Start[0] = app.PendingOffMeshStart[0];
                lk.Start[1] = app.PendingOffMeshStart[1];
                lk.Start[2] = app.PendingOffMeshStart[2];
                lk.End[0]   = wx;
                lk.End[1]   = wy;
                lk.End[2]   = wz;
                app.Geom.OffMeshLinks.push_back(lk);
                app.bGeomDirty = true;
                if (cb.TryAutoRebuild) cb.TryAutoRebuild();
            }
            app.CurrentEditMode = EditMode::None;
        }
        return;
    }

    // -------- CreateBox --------
    if (app.CurrentEditMode == EditMode::CreateBox)
    {
        if (app.Geom.Source != GeomSource::Procedural) return;
        if (hovered && havePick && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            app.CreateDraft.bActive = true;
            app.CreateDraft.StartX  = wx;
            app.CreateDraft.StartZ  = wz;
            app.CreateDraft.CurX    = wx;
            app.CreateDraft.CurZ    = wz;
        }
        else if (app.CreateDraft.bActive && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            if (havePick) { app.CreateDraft.CurX = wx; app.CreateDraft.CurZ = wz; }
        }
        else if (app.CreateDraft.bActive && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            Obstacle ob;
            ob.Height = app.DefaultObstacleHeight;
            if (app.DefaultObstacleShape == ObstacleShape::Box)
            {
                const float minX = std::min(app.CreateDraft.StartX, app.CreateDraft.CurX);
                const float maxX = std::max(app.CreateDraft.StartX, app.CreateDraft.CurX);
                const float minZ = std::min(app.CreateDraft.StartZ, app.CreateDraft.CurZ);
                const float maxZ = std::max(app.CreateDraft.StartZ, app.CreateDraft.CurZ);
                if ((maxX - minX) > 0.2f && (maxZ - minZ) > 0.2f)
                {
                    ob.Shape = ObstacleShape::Box;
                    ob.CX = (minX + maxX) * 0.5f;
                    ob.CZ = (minZ + maxZ) * 0.5f;
                    ob.SX = (maxX - minX) * 0.5f;
                    ob.SZ = (maxZ - minZ) * 0.5f;
                    app.Geom.Obstacles.push_back(ob);
                    if (cb.RebuildProceduralInputGeometry) cb.RebuildProceduralInputGeometry();
                    app.bGeomDirty = true;
                    if (cb.TryAutoRebuild) cb.TryAutoRebuild();
                }
            }
            else
            {
                const float dx = app.CreateDraft.CurX - app.CreateDraft.StartX;
                const float dz = app.CreateDraft.CurZ - app.CreateDraft.StartZ;
                const float r  = std::sqrt(dx * dx + dz * dz);
                if (r > 0.2f)
                {
                    ob.Shape  = ObstacleShape::Cylinder;
                    ob.CX     = app.CreateDraft.StartX;
                    ob.CZ     = app.CreateDraft.StartZ;
                    ob.Radius = r;
                    app.Geom.Obstacles.push_back(ob);
                    app.SelectedObstacle = static_cast<int>(app.Geom.Obstacles.size()) - 1;
                    if (cb.RebuildProceduralInputGeometry) cb.RebuildProceduralInputGeometry();
                    app.bGeomDirty = true;
                    if (cb.TryAutoRebuild) cb.TryAutoRebuild();
                }
            }
            if (app.DefaultObstacleShape == ObstacleShape::Box && !app.Geom.Obstacles.empty())
                app.SelectedObstacle = static_cast<int>(app.Geom.Obstacles.size()) - 1;
            app.CreateDraft.bActive = false;
        }
        return;
    }

    // -------- Select & Move --------
    if (app.CurrentEditMode == EditMode::MoveBox)
    {
        if (app.Geom.Source != GeomSource::Procedural) return;
        if (hovered && havePick && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            const int idx        = PickObstacleIndex(app, mp);
            app.SelectedObstacle = idx;
            if (idx >= 0)
            {
                app.MoveState.Index   = idx;
                app.MoveState.OffsetX = wx - app.Geom.Obstacles[idx].CX;
                app.MoveState.OffsetZ = wz - app.Geom.Obstacles[idx].CZ;
            }
        }
        else if (app.MoveState.Index >= 0 && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            if (havePick && app.MoveState.Index < (int)app.Geom.Obstacles.size())
            {
                Obstacle& o = app.Geom.Obstacles[app.MoveState.Index];
                o.CX = wx - app.MoveState.OffsetX;
                o.CZ = wz - app.MoveState.OffsetZ;
            }
        }
        else if (app.MoveState.Index >= 0 && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            if (cb.RebuildProceduralInputGeometry) cb.RebuildProceduralInputGeometry();
            app.bGeomDirty = true;
            app.MoveState.Index = -1;
            if (cb.TryAutoRebuild) cb.TryAutoRebuild();
        }
        return;
    }

    // -------- DeleteBox --------
    if (app.CurrentEditMode == EditMode::DeleteBox)
    {
        if (app.Geom.Source != GeomSource::Procedural) return;
        if (hovered && havePick && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            const int idx = PickObstacleIndex(app, mp);
            if (idx >= 0)
            {
                app.Geom.Obstacles.erase(app.Geom.Obstacles.begin() + idx);
                if (app.SelectedObstacle == idx) app.SelectedObstacle = -1;
                else if (app.SelectedObstacle > idx) --app.SelectedObstacle;
                if (cb.RebuildProceduralInputGeometry) cb.RebuildProceduralInputGeometry();
                app.bGeomDirty = true;
                if (cb.TryAutoRebuild) cb.TryAutoRebuild();
            }
        }
        return;
    }

    // -------- 任意模式: 左键单击障碍 = 选中, 空白处 = 取消选择 --------
    if (hovered && havePick && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
        && app.Geom.Source == GeomSource::Procedural)
    {
        app.SelectedObstacle = PickObstacleIndex(app, mp);
    }
}

// =============================================================================
// 快捷键
// =============================================================================

void HandleHotkeys(AppState& app, const InteractionCallbacks& cb)
{
    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput) return;

    auto Pressed = [](int k) { return ImGui::IsKeyPressed(k, false); };

    // ---- WASD/QE 连续平移 (仅 3D 视角) ----
    if (app.CurrentViewMode == ViewMode::Orbit3D)
    {
        const float dt = std::max(0.0f, io.DeltaTime);
        if (dt > 0.0f)
        {
            const float cyaw = std::cos(app.Cam.Yaw);
            const float syaw = std::sin(app.Cam.Yaw);
            const Vec3  fwd  = V3(-cyaw, 0.0f, -syaw);
            const Vec3  right = V3(syaw, 0.0f, -cyaw);

            float speed = std::max(2.0f, app.Cam.Distance * 1.2f);
            if (io.KeyShift) speed *= 3.0f;
            if (io.KeyCtrl)  speed *= 0.3f;

            Vec3 move{ 0.0f, 0.0f, 0.0f };
#if defined(_WIN32)
            if (ImGui::IsKeyDown('W')) move = move + fwd;
            if (ImGui::IsKeyDown('S')) move = move - fwd;
            if (ImGui::IsKeyDown('D')) move = move + right;
            if (ImGui::IsKeyDown('A')) move = move - right;
            if (ImGui::IsKeyDown('E')) move.y += 1.0f;
            if (ImGui::IsKeyDown('Q')) move.y -= 1.0f;
#else
            if (ImGui::IsKeyDown(GLFW_KEY_W)) move = move + fwd;
            if (ImGui::IsKeyDown(GLFW_KEY_S)) move = move - fwd;
            if (ImGui::IsKeyDown(GLFW_KEY_D)) move = move + right;
            if (ImGui::IsKeyDown(GLFW_KEY_A)) move = move - right;
            if (ImGui::IsKeyDown(GLFW_KEY_E)) move.y += 1.0f;
            if (ImGui::IsKeyDown(GLFW_KEY_Q)) move.y -= 1.0f;
#endif

            app.Cam.Target.x += move.x * speed * dt;
            app.Cam.Target.y += move.y * speed * dt;
            app.Cam.Target.z += move.z * speed * dt;
        }
    }

#if defined(_WIN32)
    if (Pressed(VK_ESCAPE))
    {
        app.CurrentEditMode  = EditMode::None;
        app.SelectedObstacle = -1;
    }
    if (Pressed('1')) app.CurrentEditMode = EditMode::PlaceStart;
    if (Pressed('2')) app.CurrentEditMode = EditMode::PlaceEnd;
    if (Pressed('T')) app.CurrentEditMode = EditMode::MoveBox;
    if (Pressed('B'))
    {
        app.CurrentEditMode        = EditMode::CreateBox;
        app.DefaultObstacleShape   = ObstacleShape::Box;
    }
    if (Pressed('C'))
    {
        app.CurrentEditMode        = EditMode::CreateBox;
        app.DefaultObstacleShape   = ObstacleShape::Cylinder;
    }
    if (Pressed(VK_DELETE) || Pressed('X'))
#else
    if (Pressed(GLFW_KEY_ESCAPE))
    {
        app.CurrentEditMode  = EditMode::None;
        app.SelectedObstacle = -1;
    }
    if (Pressed(GLFW_KEY_1)) app.CurrentEditMode = EditMode::PlaceStart;
    if (Pressed(GLFW_KEY_2)) app.CurrentEditMode = EditMode::PlaceEnd;
    if (Pressed(GLFW_KEY_T)) app.CurrentEditMode = EditMode::MoveBox;
    if (Pressed(GLFW_KEY_B))
    {
        app.CurrentEditMode        = EditMode::CreateBox;
        app.DefaultObstacleShape   = ObstacleShape::Box;
    }
    if (Pressed(GLFW_KEY_C))
    {
        app.CurrentEditMode        = EditMode::CreateBox;
        app.DefaultObstacleShape   = ObstacleShape::Cylinder;
    }
    if (Pressed(GLFW_KEY_DELETE) || Pressed(GLFW_KEY_X))
#endif
    {
        if (app.Geom.Source == GeomSource::Procedural &&
            app.SelectedObstacle >= 0 &&
            app.SelectedObstacle < static_cast<int>(app.Geom.Obstacles.size()))
        {
            app.Geom.Obstacles.erase(app.Geom.Obstacles.begin() + app.SelectedObstacle);
            app.SelectedObstacle = -1;
            app.MoveState.Index  = -1;
            if (cb.RebuildProceduralInputGeometry) cb.RebuildProceduralInputGeometry();
            app.bGeomDirty = true;
            if (cb.TryAutoRebuild) cb.TryAutoRebuild();
        }
    }
#if defined(_WIN32)
    if (Pressed('R'))
    {
        if (cb.BuildNavMesh) cb.BuildNavMesh();
        if (cb.TryAutoReplan) cb.TryAutoReplan();
    }
    if (Pressed('F'))
    {
        if (cb.FindPath) cb.FindPath();
    }
    if (Pressed('V'))
    {
        app.CurrentViewMode = (app.CurrentViewMode == ViewMode::TopDown2D)
                            ? ViewMode::Orbit3D : ViewMode::TopDown2D;
    }
    if (Pressed(VK_HOME))
    {
        if (cb.FrameCameraToBounds) cb.FrameCameraToBounds();
    }
#else
    if (Pressed(GLFW_KEY_R))
    {
        if (cb.BuildNavMesh) cb.BuildNavMesh();
        if (cb.TryAutoReplan) cb.TryAutoReplan();
    }
    if (Pressed(GLFW_KEY_F))
    {
        if (cb.FindPath) cb.FindPath();
    }
    if (Pressed(GLFW_KEY_V))
    {
        app.CurrentViewMode = (app.CurrentViewMode == ViewMode::TopDown2D)
                            ? ViewMode::Orbit3D : ViewMode::TopDown2D;
    }
    if (Pressed(GLFW_KEY_HOME))
    {
        if (cb.FrameCameraToBounds) cb.FrameCameraToBounds();
    }
#endif
}

} // namespace Interaction

#pragma once
/*
 * Render/NavStepDebug.h
 * ---------------------
 * NavStepBuilder 各阶段中间数据的 3D 调试可视化叠绘。
 *
 * 在 DrawCanvas3D 完成后调用，作为屏幕空间叠加层（无深度测试）：
 *   - Step 1 (Config)  : 绘制 BV/几何包围盒 + 体素网格在 XZ 平面的轮廓
 *   - Step 2 (Rasterize): rcHeightfield spans 的顶面四边形（按 area 染色）
 *   - Step 3 (Filter)  : 同上，但仅可走 spans
 *   - Step 4 (CompactHF): rcCompactHeightfield 各 cell 顶面（按 area 染色）
 *   - Step 5 (Erode)   : 同 Step4 + dist 字段灰阶
 *   - Step 6 (Regions) : 按 reg id 哈希着色
 *   - Step 7 (Contours): rcContour 简化顶点折线
 *   - Step 8/9/10      : 已由 Renderer3D 主流程绘制（不重复）
 *
 * 设计原则：
 *   - 仅依赖 Render3D::Line3D / TriFilled3D 与 Map3D
 *   - 不接管像素深度，只绘制 ImDrawList 折线/填充三角
 *   - 大场景 cell 数过多时自动 stride 抽样，避免 ImDrawList 顶点爆炸
 */

#include "RenderTypes.h"
#include "../Nav/NavStepBuilder.h"
#include "imgui.h"

namespace NavStepDebug
{

/// 调试叠绘可视化参数（每帧由调用方填充）
struct DrawParams
{
    /// 当前帧 3D 视口与矩阵（来自 Renderer3D::DrawCanvas3D 返回值）
    const Map3D* Map = nullptr;
    /// 已切到 LastCompleted 步骤的构建器
    const NavStepBuilder::StepBuilder* Step = nullptr;
    /// 主开关（关闭时不绘制任何东西）
    bool bEnabled = true;
    /// 单帧最大体素 / 紧凑单元数。超过自动 stride 抽样。
    int  MaxCellsToDraw = 60000;
};

/// 在 [vMin, vMin+vSize] 区域内绘制当前步骤对应的调试可视化层。
/// 必须在 Renderer3D::DrawCanvas3D 之后、ImGui::EndChild 之前调用。
void Draw3D(ImDrawList* dl, ImVec2 vMin, ImVec2 vSize, const DrawParams& p);

} // namespace NavStepDebug

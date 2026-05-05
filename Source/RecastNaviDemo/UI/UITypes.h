#pragma once
/*
 * UI/UITypes.h
 * ------------
 * UI 层所需的枚举和状态结构体：
 *   - ViewMode      ：2D 俯视 / 3D 轨道
 *   - EditMode      ：当前编辑操作（无 / 放置起点 / 放置终点 / 创建障碍 / 移动 / 删除）
 *   - ViewSettings  ：渲染可见性开关 + 自动重新规划/重构标志
 *   - CreateDraftBox：鼠标拖拽创建 Box 障碍的临时状态
 *   - MoveBoxState  ：鼠标拖拽移动 Box 障碍的临时状态
 *
 * 依赖：imgui.h（ViewSettings 中的 ImVec4 颜色）
 */

#include "imgui.h"

// =============================================================================
// 视图模式
// =============================================================================
enum class ViewMode
{
    TopDown2D,  ///< 2D 俯视图（ImDrawList 软件绘制）
    Orbit3D,    ///< 3D 轨道视角（软件投影到 ImDrawList）
};

// =============================================================================
// 编辑模式
// =============================================================================
enum class EditMode
{
    None,              ///< 仅浏览，不编辑
    PlaceStart,        ///< 点击画布放置寻路起点
    PlaceEnd,          ///< 点击画布放置寻路终点
    CreateBox,         ///< 鼠标拖拽创建 Box 障碍
    MoveBox,           ///< 鼠标拖拽移动已有障碍
    DeleteBox,         ///< 点击画布删除障碍
    PlaceOffMeshStart, ///< 点击画布设定 OffMeshLink 起点
    PlaceOffMeshEnd,   ///< 点击画布设定 OffMeshLink 终点
};

// =============================================================================
// 3D 画布合成方式（仅 Orbit3D）
// =============================================================================
enum class View3DDepthMode : uint8_t
{
    None        = 0, ///< 直接 ImDrawList，无全局深度/排序
    CpuZBuffer  = 1, ///< CPU 深度缓冲 + OpenGL 纹理
    PainterSort = 2, ///< 按三角重心距相机排序后绘制（画家算法）
    GpuShader   = 3, ///< GPU 着色器（FBO + GLSL 330）+ 硬件深度（仅 GL 平台可用）
};

// =============================================================================
// 光照参数（仅 GpuShader 模式生效）
// -----------------------------------------------------------------------------
//   - Directional：把场景当作太阳照射，Direction 为光指向（fragment 看到的 L =
//     normalize(Direction)），距离 / 衰减无效。
//   - Point：把光当作放在 normalize(Direction) * Distance 的世界坐标点光源，
//     片元 L = normalize(lightPos - fragPos)，距离衰减按 d²/(d² + Distance²)。
// =============================================================================
struct LightSettings
{
    int   Type        = 0;        ///< 0 = Directional（太阳），1 = Point（点光源）
    /// 方向（Directional：光行进方向；Point：从场景原点指向灯位的方向）。
    /// 不必预先归一化，shader 内会 normalize；但建议保持非零向量。
    float DirX        = 0.45f;
    float DirY        = 0.85f;
    float DirZ        = 0.30f;
    /// Point 模式下灯到场景原点的距离（米）。lightPos = normalize(Dir) * Distance。
    /// Directional 模式下忽略。
    float Distance    = 30.0f;
    /// 灯光颜色（RGB，线性，0-1）；alpha 不参与计算。
    float Color[3]    = { 1.0f, 1.0f, 1.0f };
    /// 漫反射强度倍数（0 = 无方向光；1 = 默认；>1 过曝增强）。
    float Intensity   = 1.0f;
    /// 环境光强度（0 = 全黑阴影；1 = 平涂无方向感）；与原 fragment shader 的常量等价。
    float Ambient     = 0.38f;

    // -------- 阴影 (Shadow Mapping)。仅 Directional + GpuShader 模式下生效 --------
    /// 是否启用阴影（Directional 模式额外渲一遍深度图，主 pass 采样比较）
    bool  bShadowsOn      = false;
    /// 阴影贴图分辨率（正方形）。1024 默认；512 模糊但快；2048/4096 锐利但占显存。
    int   ShadowMapSize   = 1024;
    /// 阴影颜色强度：0 = 完全无阴影（仅环境光照亮），1 = 受光面与背光面亮度差最大。
    float ShadowStrength  = 0.7f;
    /// 深度比较 bias，防止 shadow acne（自阴影条纹）。典型 0.0005 ~ 0.005。
    /// 太小 → 出现条纹噪声；太大 → 出现 peter-panning（阴影脱离物体根部）。
    float ShadowBias      = 0.0015f;
    /// 0 = 单点采样（硬阴影边）；1 = 3x3 PCF（软边，9 次贴图采样开销）
    int   ShadowPcf       = 1;
};

// =============================================================================
// 视图渲染可见性设置
// =============================================================================
struct ViewSettings
{
    bool bShowGrid         = true;   ///< 显示网格辅助线
    /// 与 PhysX/Recast 输入一致的碰撞体着色：三角网地面 + 障碍棱柱
    bool          bShowCollisionTint     = true;
    ImVec4        GroundCollisionTint     = { 0.28f, 0.52f, 0.40f, 0.48f };
    ImVec4        ObstacleTopTint         = { 0.86f, 0.38f, 0.32f, 0.72f };   ///< 顶面（及 2D 俯视 footprint）
    ImVec4        ObstacleSideTint        = { 0.62f, 0.28f, 0.42f, 0.88f };  ///< 侧面朝向相机
    ImVec4        ObstacleSideBackTint    = { 0.38f, 0.16f, 0.24f, 0.88f };  ///< 侧面背向相机（与正面同不透明显示；无 Z 缓冲时远侧面已剔除）
    ImVec4        ObstacleCollisionEdge   = { 1.00f, 0.55f, 0.42f, 0.95f };
    bool bShowObstacles    = true;   ///< 显示障碍物轮廓
    /// 默认 None：ImDrawList 直接走 GPU，1800 三角的默认场景轻松 60 FPS。
    /// CpuZBuffer 是软件光栅化，大场景下会到 10 FPS 量级（仅作演示/对比用，请按需开启）。
    /// Windows D3D12 版未接 Z-Buffer 纹理上传，CpuZBuffer 也不会显示。
    View3DDepthMode DepthMode3D = View3DDepthMode::None;
    /// CpuZBuffer 模式的内部缓冲降采样系数：rw = panel.x / N, rh = panel.y / N。
    /// N=1 原生分辨率（最清晰，最慢）；N=2 半分辨率（默认，质量/速度平衡）；
    /// N=3、4 进一步降采样以换 FPS。仅 CpuZBuffer 生效，None / PainterSort 走 GPU 不受影响。
    int             CpuZBufferDownscale = 2;
    /// GpuShader 模式的 MSAA 采样数：1 关闭、2、4、8（超过 GL_MAX_SAMPLES 自动夹紧）。
    /// 仅 GpuShader 生效；其它模式忽略。
    int             GpuMsaaSamples      = 4;
    /// GpuShader 模式的灯光参数（仅 fragment 着色阶段读取）；其它模式不参与计算但仍可调。
    LightSettings   Light;
    bool bShowInputMesh    = true;   ///< 显示输入几何线框（地面/OBJ）
    bool bFillPolygons     = true;   ///< 填充 NavMesh 多边形
    bool bRegionColors     = true;   ///< 使用区域哈希色区分多边形
    bool bShowDetailMesh   = false;  ///< 显示细节网格
    bool bShowPolyEdges    = true;   ///< 显示多边形边线
    bool bShowPathCorridor = true;   ///< 显示寻路走廊高亮
    bool bAutoReplan       = true;   ///< 每次操作后自动重新寻路
    bool bAutoRebuild      = false;  ///< 几何变化后自动重新构建 NavMesh
    bool bShowBuildVolume  = true;   ///< 显示自定义生成区域线框盒
    bool bShowAutoNavLinks = true;   ///< 显示自动生成的 NavLink
    bool bSmoothPath       = false;  ///< 显示平滑路径（代替原始折线）
    int  SmoothMethod      = 0;      ///< 0=StringPull, 1=Bezier
    int  SmoothSubdivisions = 3;     ///< Bezier 细分轮数（1-6）
};

// =============================================================================
// 创建 Box 障碍的草稿状态（鼠标按下→拖拽→释放）
// =============================================================================
struct CreateDraftBox
{
    bool  bActive = false;  ///< 是否正在拖拽创建中
    float StartX  = 0.0f;  ///< 鼠标按下时的世界 X
    float StartZ  = 0.0f;  ///< 鼠标按下时的世界 Z
    float CurX    = 0.0f;  ///< 鼠标当前位置的世界 X
    float CurZ    = 0.0f;  ///< 鼠标当前位置的世界 Z
};

// =============================================================================
// 移动障碍的临时状态（鼠标拖拽）
// =============================================================================
struct MoveBoxState
{
    int   Index   = -1;    ///< 被拖拽的障碍索引（-1 = 无）
    float OffsetX = 0.0f;  ///< 鼠标按下时相对障碍中心 CX 的 X 偏移
    float OffsetZ = 0.0f;  ///< 鼠标按下时相对障碍中心 CZ 的 Z 偏移
};

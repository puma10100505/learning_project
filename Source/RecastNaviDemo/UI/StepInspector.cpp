/*
 * UI/StepInspector.cpp
 * --------------------
 * "Step Inspector" 浮动窗口实现。详细说明见 StepInspector.h。
 */

#include "StepInspector.h"

#include <climits>
#include <cstdio>

#include "Recast.h"
#include "DetourNavMesh.h"

namespace StepInspector
{

namespace
{

// =============================================================================
// 各步骤的状态徽章
// =============================================================================
enum class StepStatus
{
    Pending,    ///< 尚未执行（颜色：灰）
    Current,    ///< 最近完成的一步（颜色：金）
    Done,       ///< 已完成且不是最近一步（颜色：绿）
    Failed,     ///< 当前 / 下一步失败（颜色：红）
};

StepStatus GetStepStatus(const NavStepBuilder::StepBuilder& sb,
                         NavStepBuilder::Step              s)
{
    using NavStepBuilder::Step;
    const int last = static_cast<int>(sb.LastCompleted);
    const int idx  = static_cast<int>(s);
    if (sb.bFailed && idx == last + 1) return StepStatus::Failed;
    if (idx <  last) return StepStatus::Done;
    if (idx == last && idx > 0) return StepStatus::Current;
    return StepStatus::Pending;
}

void DrawStatusBadge(StepStatus st)
{
    ImVec4 color;
    const char* label = "PENDING";
    switch (st)
    {
        case StepStatus::Done:    color = ImVec4(0.45f, 0.85f, 0.45f, 1.0f); label = "DONE";    break;
        case StepStatus::Current: color = ImVec4(1.00f, 0.85f, 0.30f, 1.0f); label = "CURRENT"; break;
        case StepStatus::Failed:  color = ImVec4(1.00f, 0.40f, 0.40f, 1.0f); label = "FAILED";  break;
        case StepStatus::Pending: color = ImVec4(0.55f, 0.55f, 0.60f, 1.0f); label = "PENDING"; break;
    }
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
}

// =============================================================================
// 各步骤的算法说明（多段落静态文本）
// =============================================================================
const char* GetAlgoText(NavStepBuilder::Step s)
{
    using NavStepBuilder::Step;
    switch (s)
    {
        case Step::Config:
            return
                "把 NavBuildConfig (米制 + agent 描述) 翻译成 Recast 体素世界的 rcConfig。\n"
                "核心换算 (cellHeight=ch, cellSize=cs):\n"
                "  walkableHeight = ceil(AgentHeight  / ch)  [体素列]\n"
                "  walkableClimb  = floor(AgentMaxClimb/ ch) [体素行]\n"
                "  walkableRadius = ceil(AgentRadius  / cs)  [体素半径]\n"
                "  maxEdgeLen     = round(EdgeMaxLen / cs)\n"
                "  minRegionArea  = RegionMinSize^2  (cell^2)\n"
                "  detailSampleDist = (>=0.9? cs * DetailSampleDist : 0)\n"
                "包围盒 bmin/bmax 取 BuildVolume (若激活) 或几何自带 Bounds。\n"
                "rcCalcGridSize() 根据 cs/ch 把世界 AABB 分成 (width × height) 列。\n"
                "本步骤只填字段，不分配任何 Recast 资源。";

        case Step::Rasterize:
            return
                "1) rcAllocHeightfield + rcCreateHeightfield()\n"
                "   分配 width×height 列；每列是一条按 y 排序的 rcSpan 链表 (spans[x+z*w])。\n"
                "2) rcClearUnwalkableTriangles(walkableSlopeAngle)\n"
                "   计算每个三角形法线与 +Y 的夹角 -> 若 > AgentMaxSlope 标记 RC_NULL_AREA;\n"
                "   注意: 该函数只把 walkable 改为 NULL，不会反向。\n"
                "3) rcRasterizeTriangles()\n"
                "   每个三角形 -> XZ 投影到 cell 网格；按 cell 列与三角片求交得到 (smin, smax)。\n"
                "   同列高度重叠的 spans 会合并；垂直距离 ≤ walkableClimb 的相邻 spans\n"
                "   按 area 优先级合并 (walkable 覆盖 null)。\n"
                "结果: rcHeightfield 含若干 rcSpan，每个携带 (smin, smax, area)。";

        case Step::Filter:
            return
                "在 rcHeightfield 上做三遍纯就地修改的过滤，目的是把 rasterize 出来的体素\n"
                "结果修正为更接近真实可走表面：\n"
                "1) rcFilterLowHangingWalkableObstacles(walkableClimb)\n"
                "   若一个 NULL_AREA span 的下方有 WALKABLE，且垂直差 ≤ walkableClimb,\n"
                "   则把它改成 WALKABLE。用于让台阶顶端 / 楼梯角对齐。\n"
                "2) rcFilterLedgeSpans(walkableHeight, walkableClimb)\n"
                "   检查 span 与四个邻居 spans 的高度差; 若任一差 > walkableClimb 标 NULL_AREA。\n"
                "   防止边缘悬崖处生成可走表面。\n"
                "3) rcFilterWalkableLowHeightSpans(walkableHeight)\n"
                "   检查 span 上方净空; 若 < walkableHeight 标 NULL_AREA (头顶过低无法行走)。\n"
                "结果: 仍是 rcHeightfield，但 area 标记被精修。";

        case Step::CompactHF:
            return
                "1) rcAllocCompactHeightfield + rcBuildCompactHeightfield(walkableHeight, walkableClimb)\n"
                "   把链表式 spans 压缩为索引数组：\n"
                "     cells[w*h]  : index/count 指向首 span\n"
                "     spans[]     : (y, h净空, con 4邻居连接, reg)\n"
                "     areas[]     : 每紧凑 span 一字节 area type\n"
                "   只保留每列最高一个可走的 span (walkable + 上方足够净空)。\n"
                "   con 字段编码 4 个轴向(±X/±Z)邻居在邻列 spans 数组中的相对索引（6 bit/方向）。\n"
                "2) 释放 rcHeightfield (后续步骤都基于 chf)。\n"
                "结果: 数据更紧凑、随机访问 O(1)、可在 chf 上建距离场 / region / contour。";

        case Step::Erode:
            return
                "Step 5a: rcErodeWalkableArea(walkableRadius)\n"
                "   广度优先扫描: 把所有距离 RC_NULL_AREA span 在 cell 单位上 ≤ walkableRadius\n"
                "   的 walkable spans 标 NULL_AREA。语义上等价于把障碍 / 不可走面在 XZ 平面\n"
                "   膨胀 walkableRadius 个 cell, 给 agent 留半径净空。\n"
                "\n"
                "Step 5b: rcBuildDistanceField()\n"
                "   两次扫描 (forward + backward chamfer) 对每个 walkable span 计算到\n"
                "   最近 NULL span 的近似距离, 写入 chf.dist[]，单位是 cell。\n"
                "   maxDistance = max(dist[i])。\n"
                "结果: chf 携带距离场, 用于 Step6 watershed 区域分割。";

        case Step::Regions:
            return
                "rcBuildRegions(borderSize=0, minRegionArea, mergeRegionArea)：\n"
                "Watershed 分水岭算法（按距离场水平面收缩）：\n"
                "1) 从 maxDistance 向 0 逐层下降；\n"
                "2) 每层做两件事：\n"
                "   ① Expand：已有区域吃下相邻的、未分配的、距离 ≤ 当前层级的 spans;\n"
                "   ② Flood: 在该层剩下的孤岛上创建新区域 id;\n"
                "3) 最后清理:\n"
                "   - 面积 < minRegionArea 的孤立区域被吞并/丢弃;\n"
                "   - 面积 < mergeRegionArea 的相邻区域合并为更大的区域;\n"
                "   - 跨越 borderSize 的边界区域被视为外部环境忽略。\n"
                "结果: chf.spans[i].reg 写入区域 id; chf.maxRegions = 区域总数。";

        case Step::Contours:
            return
                "rcBuildContours(maxSimplificationError, maxEdgeLen)：\n"
                "1) 遍历 chf 中每个 walkable span, 检测它与上下左右邻居 spans 是否属于不同 region —\n"
                "   如果是则该方向是区域边界。\n"
                "2) 沿一个 region 的边界顺时针绕一圈，得到原始 raw contour 折线 (rverts)。\n"
                "3) Douglas-Peucker 简化:\n"
                "   把折线递归切分, 当所有原始点到当前简化段的最大垂直距离 ≤\n"
                "   maxSimplificationError 时停止细分。\n"
                "4) 长边切分: 任何长度超过 maxEdgeLen 的简化边再被等距插入中间点,\n"
                "   保证 NavMesh 多边形边长有上限 (利于后续 BV-tree 与导航)。\n"
                "5) 写入 rcContourSet.conts[] (每条 contour 含 verts/rverts/reg/area)。\n"
                "结果: 每个 region 对应一条闭合 contour, 即 region 的多边形外轮廓。";

        case Step::PolyMesh:
            return
                "rcBuildPolyMesh(maxVertsPerPoly)：\n"
                "1) 对每条 contour 做耳切法 (Ear-Clipping) 三角剖分;\n"
                "2) 合并相邻三角形为最大不超过 maxVertsPerPoly 顶点的凸多边形\n"
                "   (Recast 默认 6, Detour 上限 12);\n"
                "3) 顶点全局去重, 建立邻接关系 polys[i*nvp*2 + nvp..2nvp-1] 存储每条边的对侧 poly;\n"
                "4) regs[] 拷贝 contour.reg, areas[] 拷贝 contour.area, flags[] 默认 0;\n"
                "5) 该步骤同时建立 NavMesh 的连通图 (后续 Detour 寻路图基础)。\n"
                "结果: rcPolyMesh 携带 npolys 个凸多边形 + 顶点表 + 邻接 + 区域/area/flag。";

        case Step::DetailMesh:
            return
                "rcBuildPolyMeshDetail(detailSampleDist, detailSampleMaxError, chf)：\n"
                "1) 对每个 PolyMesh 凸多边形, 在原 chf 高度场上以 detailSampleDist 网格采样高度;\n"
                "2) 加上多边形顶点本身, 做 Delaunay 三角剖分得到子三角集合;\n"
                "3) 简化: 当一个采样点到 (其邻接三角面) 的高度差 < detailSampleMaxError 时\n"
                "   将其去除, 减少三角数;\n"
                "4) 写入 rcPolyMeshDetail.meshes[i*4] = (vbase, nverts, tbase, ntris) 索引该 poly 的子集。\n"
                "5) 之后释放 chf (不再需要)。\n"
                "用途: 把扁平的 PolyMesh 高度恢复到接近原地形, 让 agent 沿斜坡 / 起伏地形移动\n"
                "时的 Y 坐标查询更准确。";

        case Step::DetourNav:
            return
                "完成 Recast → Detour 的转换:\n"
                "1) 把 PolyMesh 中所有 walkable polys 标 flags |= 0x01 (默认通行掩码);\n"
                "2) 准备 dtNavMeshCreateParams: PolyMesh + DetailMesh + OffMeshLinks + cs/ch/agent;\n"
                "3) dtCreateNavMeshData() 序列化为单 tile 二进制 (含 BV tree 加速结构);\n"
                "4) dtAllocNavMesh + init(navData, DT_TILE_FREE_DATA): 创建 NavMesh, 接管二进制内存;\n"
                "5) dtAllocNavMeshQuery + init(2048 nodes pool): 寻路查询对象。\n"
                "至此整个流水线完成, 用户可以调用 Find Path / Place Start/End 进行寻路。";

        case Step::None:
        default:
            return "";
    }
}

// =============================================================================
// 各步骤的"输出数据"快照（基于 StepBuilder 当前内存状态生成）
// =============================================================================
void DrawOutputData(const NavStepBuilder::StepBuilder& sb,
                    NavStepBuilder::Step              s)
{
    using NavStepBuilder::Step;
    const bool reached = static_cast<int>(s) <= static_cast<int>(sb.LastCompleted);

    if (!reached)
    {
        ImGui::TextDisabled("(尚未执行 - 数据不可用)");
        return;
    }

    char buf[256];
    auto Row = [&](const char* k, const char* v)
    {
        ImGui::Bullet(); ImGui::Text("%-22s : %s", k, v);
    };
    auto RowN = [&](const char* k, long long v)
    {
        std::snprintf(buf, sizeof(buf), "%lld", v);
        Row(k, buf);
    };
    auto RowF = [&](const char* k, double v, const char* unit = "")
    {
        std::snprintf(buf, sizeof(buf), "%.3f%s", v, unit);
        Row(k, buf);
    };

    switch (s)
    {
        case Step::Config:
        {
            const rcConfig& c = sb.Cfg;
            std::snprintf(buf, sizeof(buf), "%.3f / %.3f m", c.cs, c.ch);
            Row("cell size / height", buf);
            std::snprintf(buf, sizeof(buf), "%d × %d cells", c.width, c.height);
            Row("grid (XZ)", buf);
            std::snprintf(buf, sizeof(buf), "(%.2f, %.2f, %.2f) → (%.2f, %.2f, %.2f)",
                          c.bmin[0], c.bmin[1], c.bmin[2],
                          c.bmax[0], c.bmax[1], c.bmax[2]);
            Row("bbox (m)", buf);
            RowN("walkableHeight (cells)", c.walkableHeight);
            RowN("walkableClimb  (cells)", c.walkableClimb);
            RowN("walkableRadius (cells)", c.walkableRadius);
            std::snprintf(buf, sizeof(buf), "%.2f° / %d", c.walkableSlopeAngle, c.maxEdgeLen);
            Row("walkableSlope° / maxEdge", buf);
            RowN("minRegionArea  (cells^2)", c.minRegionArea);
            RowN("mergeRegionArea(cells^2)", c.mergeRegionArea);
            std::snprintf(buf, sizeof(buf), "%.3f / %.3f", c.detailSampleDist, c.detailSampleMaxError);
            Row("detailSampleDist/Err", buf);
            break;
        }

        case Step::Rasterize:
        case Step::Filter:
        {
            if (!sb.Solid) { ImGui::TextDisabled("(rcHeightfield 已被释放)"); return; }
            const rcHeightfield& hf = *sb.Solid;
            std::snprintf(buf, sizeof(buf), "%d × %d", hf.width, hf.height);
            Row("rcHeightfield grid", buf);
            // 统计 spans 数与 walkable / null 比例
            int totalSpans = 0, walkSpans = 0, nullSpans = 0;
            for (int z = 0; z < hf.height; ++z)
                for (int x = 0; x < hf.width; ++x)
                    for (rcSpan* sp = hf.spans[x + z * hf.width]; sp; sp = sp->next)
                    {
                        ++totalSpans;
                        if (sp->area == RC_WALKABLE_AREA) ++walkSpans;
                        else if (sp->area == RC_NULL_AREA) ++nullSpans;
                    }
            RowN("total spans", totalSpans);
            RowN("walkable spans", walkSpans);
            RowN("null/blocked spans", nullSpans);
            std::snprintf(buf, sizeof(buf), "(%.2f, %.2f, %.2f) → (%.2f, %.2f, %.2f)",
                          hf.bmin[0], hf.bmin[1], hf.bmin[2],
                          hf.bmax[0], hf.bmax[1], hf.bmax[2]);
            Row("bbox (m)", buf);
            break;
        }

        case Step::CompactHF:
        case Step::Erode:
        case Step::Regions:
        {
            if (!sb.Chf) { ImGui::TextDisabled("(rcCompactHeightfield 已被释放)"); return; }
            const rcCompactHeightfield& chf = *sb.Chf;
            std::snprintf(buf, sizeof(buf), "%d × %d", chf.width, chf.height);
            Row("compact grid", buf);
            RowN("spanCount", chf.spanCount);
            RowN("cells", chf.width * chf.height);
            RowN("walkableHeight (cells)", chf.walkableHeight);
            RowN("walkableClimb  (cells)", chf.walkableClimb);
            // 按 area 统计
            int walk = 0, nul = 0;
            for (int i = 0; i < chf.spanCount; ++i)
            {
                if (chf.areas[i] == RC_WALKABLE_AREA) ++walk;
                else if (chf.areas[i] == RC_NULL_AREA) ++nul;
            }
            RowN("walkable spans", walk);
            RowN("null spans", nul);
            if (s == Step::Erode || s == Step::Regions)
                RowN("maxDistance (chamfer)", chf.maxDistance);
            if (s == Step::Regions)
                RowN("maxRegions (region id)", chf.maxRegions);
            break;
        }

        case Step::Contours:
        {
            if (!sb.Cset) { ImGui::TextDisabled("(rcContourSet 已被释放)"); return; }
            const rcContourSet& cs = *sb.Cset;
            RowN("nconts", cs.nconts);
            int totalSimp = 0, totalRaw = 0;
            int minV = INT_MAX, maxV = 0;
            for (int i = 0; i < cs.nconts; ++i)
            {
                totalSimp += cs.conts[i].nverts;
                totalRaw  += cs.conts[i].nrverts;
                if (cs.conts[i].nverts > 0)
                {
                    if (cs.conts[i].nverts < minV) minV = cs.conts[i].nverts;
                    if (cs.conts[i].nverts > maxV) maxV = cs.conts[i].nverts;
                }
            }
            RowN("total simplified verts", totalSimp);
            RowN("total raw verts",       totalRaw);
            std::snprintf(buf, sizeof(buf), "%d ~ %d",
                          (cs.nconts > 0 ? minV : 0), maxV);
            Row("verts/contour (min~max)", buf);
            std::snprintf(buf, sizeof(buf), "%.4f", cs.maxError);
            Row("maxSimplificationErr", buf);
            std::snprintf(buf, sizeof(buf), "%.3f / %.3f", cs.cs, cs.ch);
            Row("cs / ch", buf);
            break;
        }

        case Step::PolyMesh:
        {
            if (!sb.Pmesh) { ImGui::TextDisabled("(rcPolyMesh 已被释放/移交)"); return; }
            const rcPolyMesh& pm = *sb.Pmesh;
            RowN("npolys", pm.npolys);
            RowN("nverts", pm.nverts);
            RowN("nvp (max verts/poly)", pm.nvp);
            // 多边形顶点数分布
            int counts[16] = {0};
            for (int i = 0; i < pm.npolys; ++i)
            {
                const unsigned short* p = &pm.polys[i * pm.nvp * 2];
                int n = 0;
                for (; n < pm.nvp && p[n] != RC_MESH_NULL_IDX; ++n) {}
                if (n >= 0 && n < 16) counts[n]++;
            }
            for (int n = 3; n < 13; ++n)
            {
                if (counts[n] == 0) continue;
                std::snprintf(buf, sizeof(buf), "%d polys", counts[n]);
                char key[32];
                std::snprintf(key, sizeof(key), "  %d-gons", n);
                Row(key, buf);
            }
            std::snprintf(buf, sizeof(buf), "(%.2f, %.2f, %.2f) → (%.2f, %.2f, %.2f)",
                          pm.bmin[0], pm.bmin[1], pm.bmin[2],
                          pm.bmax[0], pm.bmax[1], pm.bmax[2]);
            Row("bbox (m)", buf);
            break;
        }

        case Step::DetailMesh:
        {
            if (!sb.Dmesh) { ImGui::TextDisabled("(rcPolyMeshDetail 已被释放/移交)"); return; }
            const rcPolyMeshDetail& dm = *sb.Dmesh;
            RowN("nmeshes (sub-mesh)", dm.nmeshes);
            RowN("nverts (total)", dm.nverts);
            RowN("ntris  (total)", dm.ntris);
            if (dm.nmeshes > 0)
            {
                std::snprintf(buf, sizeof(buf), "%.1f", static_cast<double>(dm.ntris) / dm.nmeshes);
                Row("avg tris / sub-mesh", buf);
            }
            break;
        }

        case Step::DetourNav:
        {
            const NavRuntime* rt = nullptr;
            // sb 自身的 NavMesh 已转交给 NavRuntime；此时只能从 AppState 拿，
            // 但本函数没有 AppState 引用 —— 给出 byte size 提示 (NavMeshData 转交后清空)
            // 改为提示用户切到 Build Config 面板查看 dtNavMesh 详情。
            (void)rt;
            ImGui::TextDisabled("dtNavMesh / dtNavMeshQuery 已转交给 NavRuntime。");
            ImGui::TextDisabled("详情请见 Build Config 面板的 PolyMesh / DetailMesh 行或 Stats 窗口。");
            break;
        }

        case Step::None:
        default:
            ImGui::TextDisabled("(no data)");
            break;
    }
}

// =============================================================================
// 单个步骤区块
// =============================================================================
void DrawStepSection(const NavStepBuilder::StepBuilder& sb,
                     NavStepBuilder::Step              s,
                     bool                              autoOpenIfCurrent)
{
    using NavStepBuilder::Step;
    const StepStatus st = GetStepStatus(sb, s);
    const int        idx = static_cast<int>(s);

    // 强制当前步骤默认展开
    if (autoOpenIfCurrent && (st == StepStatus::Current || st == StepStatus::Failed))
        ImGui::SetNextItemOpen(true, ImGuiCond_Always);
    else if (st == StepStatus::Done)
        ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);

    char header[96];
    std::snprintf(header, sizeof(header), "Step %d  %s",
                  idx, NavStepBuilder::GetStepName(s));

    ImGui::PushID(idx);
    if (ImGui::CollapsingHeader(header))
    {
        ImGui::Indent();

        // 状态徽章
        ImGui::TextDisabled("Status:"); ImGui::SameLine();
        DrawStatusBadge(st);
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        // 耗时（只在已完成的步骤上显示有意义的值）
        const PhaseTimings& T = sb.Timings;
        double ms = 0.0;
        switch (s)
        {
            case Step::Rasterize:  ms = T.RasterizeMs;    break;
            case Step::Filter:     ms = T.FilterMs;       break;
            case Step::CompactHF:  ms = T.CompactMs;      break;
            case Step::Erode:      ms = T.ErodeMs + T.DistFieldMs; break;
            case Step::Regions:    ms = T.RegionsMs;      break;
            case Step::Contours:   ms = T.ContoursMs;     break;
            case Step::PolyMesh:   ms = T.PolyMeshMs;     break;
            case Step::DetailMesh: ms = T.DetailMeshMs;   break;
            case Step::DetourNav:  ms = T.DetourCreateMs; break;
            default: break;
        }
        if (ms > 0.0)
            ImGui::Text("Time: %.3f ms", ms);
        else
            ImGui::TextDisabled("Time: -");

        // 简短说明（一行）
        ImGui::TextWrapped("摘要: %s", NavStepBuilder::GetStepDescription(s));

        // 失败信息
        if (st == StepStatus::Failed && !sb.FailMsg.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
            ImGui::TextWrapped("ERROR: %s", sb.FailMsg.c_str());
            ImGui::PopStyleColor();
        }

        // 算法说明（多段落）
        if (ImGui::TreeNodeEx("Algorithm / 算法说明", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextUnformatted(GetAlgoText(s));
            ImGui::PopTextWrapPos();
            ImGui::TreePop();
        }

        // 输出数据
        if (ImGui::TreeNodeEx("Output Data / 输出数据", ImGuiTreeNodeFlags_DefaultOpen))
        {
            DrawOutputData(sb, s);
            ImGui::TreePop();
        }

        ImGui::Unindent();
    }
    ImGui::PopID();
}

} // anonymous namespace

// =============================================================================
// 入口
// =============================================================================
void Draw(AppState& app)
{
    if (!app.bShowStepInspector) return;

    // 默认尺寸 / 位置（首次出现时；之后窗口位置/大小由 ImGui 自动持久化）
    ImGui::SetNextWindowSize(ImVec2(560.0f, 640.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos (ImVec2(80.0f,  80.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(360.0f, 240.0f), ImVec2(2400.0f, 1800.0f));

    constexpr ImGuiWindowFlags kFlags = ImGuiWindowFlags_NoCollapse;

    if (!ImGui::Begin("Step Inspector / 分步详情", &app.bShowStepInspector, kFlags))
    {
        ImGui::End();
        return;
    }

    const auto& sb       = app.StepBuild;
    const bool  active   = NavStepBuilder::IsActive(sb);
    const auto  lastDone = sb.LastCompleted;
    const auto  next     = NavStepBuilder::PeekNextStep(sb);

    // ---- 顶栏：当前会话状态 + 选项 ----
    ImGui::Text("Session: %s   |   Last completed: %s   |   Next: %s",
                active ? "ACTIVE" : "(idle)",
                NavStepBuilder::GetStepName(lastDone),
                next == NavStepBuilder::Step::None
                    ? "(pipeline complete)"
                    : NavStepBuilder::GetStepName(next));

    if (sb.bFailed)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
        ImGui::TextWrapped("ERROR (last attempt): %s", sb.FailMsg.c_str());
        ImGui::PopStyleColor();
    }

    static bool sAutoOpenCurrent = true;
    ImGui::Checkbox("Auto-expand current step / 当前步默认展开", &sAutoOpenCurrent);
    ImGui::SameLine(); ImGui::TextDisabled("(其它步骤展开状态记忆于 imgui.ini)");

    // ---- 流水线总进度条 ----
    {
        const float frac = active
            ? (static_cast<int>(lastDone) / float(NavStepBuilder::kStepCount))
            : 0.0f;
        char prog[64];
        std::snprintf(prog, sizeof(prog), "%d / %d", static_cast<int>(lastDone),
                      NavStepBuilder::kStepCount);
        ImGui::ProgressBar(frac, ImVec2(-FLT_MIN, 0.0f), prog);
    }

    ImGui::Separator();

    // ---- 各步骤详情区 ----
    ImGui::BeginChild("##stepscroll", ImVec2(0, 0), false,
                      ImGuiWindowFlags_HorizontalScrollbar);
    using NavStepBuilder::Step;
    for (int i = 1; i <= NavStepBuilder::kStepCount; ++i)
        DrawStepSection(sb, static_cast<Step>(i), sAutoOpenCurrent);
    ImGui::EndChild();

    ImGui::End();
}

} // namespace StepInspector

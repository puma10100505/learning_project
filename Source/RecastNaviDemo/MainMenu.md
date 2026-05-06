# RecastNaviDemo 左侧面板功能分类

左侧面板由 `MainLayout::OnGUI` 中 `##LeftPane` 子窗口绘制，自上而下依次为若干可折叠区块（见 `UI/MainLayout.cpp` + `UI/Panels.cpp`）。下表按**功能域**归类，而非严格按 UI 顺序。

---

## 1. 应用与几何上下文

| UI 位置 | 内容 |
|--------|------|
| 面板顶栏 | 标题「RecastNavigation Demo」及几何来源后缀：`[procedural]`（程序化地形）或 `[obj]`（OBJ 导入） |

---

## 2. 视图模式、合成与性能

**折叠区：View**

| 子类 | 配置项 / 行为 |
|------|----------------|
| 视图模式 | **2D Top** / **3D Orbit** 单选 |
| 3D 深度合成（仅 Orbit3D） | **3D composite**：None（仅 ImDrawList）、CPU Z-Buffer + GPU 纹理、Depth sort（画家算法）、GPU Shader（FBO；Windows 下为 D3D12 HLSL 离屏）。附平台说明与 Painter 穿插提示 |
| CPU Z-Buffer 质量 | **Z-Buffer resolution**：1x～4x 降采样（影响速度与清晰度） |
| GPU Shader 抗锯齿 | **MSAA samples**：1x / 2x / 4x / 8x（提示光照在 Lighting 面板） |
| 图层开关 | **Grid**、**Input mesh**、**Obstacles** |
| 网格细节（Grid 开启时） | **Grid auto-fit**、手动 **Grid half extent**、**Grid line thickness**（高 DPI 倍率） |
| 碰撞着色 | **Collision tint** 及地面、障碍顶/侧/边等 **RGBA 颜色** |
| NavMesh / 调试绘制 | **Fill polygons**、**Region colors**、**Poly edges**、**Detail mesh**、**Path corridor** |
| 自动化 | **Auto replan**（起终点等变化时自动寻路）、**Auto rebuild**（几何变化时自动构建） |
| 窗口 | **Stats Window**（底部性能趋势窗开关）、**Build Volume**（自定义构建体可视化） |
| 相机 | **Reset Camera (Home)**；3D 下 **Yaw / Pitch / Distance** 滑条 |

---

## 3. 路径与走廊外观

**折叠区：View（下半）**

| 配置项 | 说明 |
|--------|------|
| **Path Color** | 路径线颜色 |
| **Corridor Color (RGBA)** | 走廊半透明颜色 |

---

## 4. 3D 光照与阴影（GpuShader）

**折叠区：Lighting**

在非「3D + GpuShader」时仅作提示，参数由 **GpuShader** fragment shader 使用。

| 子类 | 配置项 |
|------|--------|
| 光源类型 | **Directional (sun)** / **Point (lamp)** |
| 方向 | **Direction**（滑条），显示归一化向量 |
| 距离 | **Distance**（点光源灯位与衰减；平行光下禁用） |
| 颜色与强度 | **Color**、**Intensity**、**Ambient** |
| 阴影（树节点 Shadows） | **Enable**（仅 Directional + GpuShader 生效）、**Map Size**（512～4096）、**Strength**、**Bias**、**3x3 PCF** |
| 预设 | **Reset**、**Noon**、**Sunset**、**Lamp** |

---

## 5. 程序化场景几何

**折叠区：Scene / 场景**

仅在 **GeomSource::Procedural** 时可编辑；OBJ 模式下仅显示说明并直接 return。

| 子类 | 配置项 |
|------|--------|
| 尺寸与细分 | **Half Size**、**Grid Cells**；（只读）三角形数、网格步长说明 |
| 预设 | **Tiny 10**、**Small 30**、**Medium 60**、**Large 120**、**Huge 300** |
| 障碍网格 | **Obstacle solid mesh**（顶面可走；会触发几何重建/自动重建） |

场景参数变更时会 **clamp 障碍与起终点**、重建程序化几何、可 **Frame Camera**、标记 dirty 并可 **TryAutoRebuild**。

---

## 6. 编辑模式、障碍与 OffMeshLink

**折叠区：Edit Mode**

| 子类 | 内容 |
|------|------|
| 模式 | 下拉：**None**、**Place Start**、**Place End**、**Create Obstacle**、**Select & Move**、**Delete Obstacle**（共 6 项；**OffMesh 起点/终点** 放置模式不在下拉中，由下方 **+ Add Link** 切入） |
| 帮助 | **Hotkeys / 快捷键** 树：3D 导航（WASD、QE、右键、滚轮、Home）、编辑（1/2、T、B/C、Del/X、R、F、V、Esc） |
| 约束提示 | OBJ 模式下障碍编辑不可用时彩色警告 |
| 选择状态 | 当前选中障碍索引、形状、中心、高度 |
| 新建障碍默认 | **New Shape**（Box / Cylinder）、**New Height** |
| 批量操作 | **Clear All Obstacles**（仅 Procedural）、**Reset Geometry**（销毁 Nav、恢复默认几何） |
| 障碍列表 | 可滚动列表：每项显示参数、**h** 高度滑条、**del**；（变更触发重建/自动重建） |
| OffMeshLink | 计数、**+ Add Link**（切到放置模式）、链接列表（起终点、半径 **r**、删除）、**Clear All Links** |

---

## 7. Recast / Detour 构建配置与运行时

**折叠区：Build Config**

### 7.1 经典 Recast 11 参数

| 参数 | 含义（与面板中文说明一致） |
|------|---------------------------|
| Cell Size / Cell Height | 体素水平/垂直尺寸（米） |
| Agent Height / Radius | 角色净空高度、侵蚀半径 |
| Agent Max Climb / Max Slope | 最大攀爬高度、最大行走坡度（度） |
| Region Min Size / Merge Size | 区域最小尺寸与合并阈值（cell²） |
| Edge Max Len / Edge Max Error | 轮廓边长上限与简化误差 |
| Verts Per Poly | 多边形最大顶点数（3～12） |

### 7.2 构建范围与动态障碍

| 配置项 | 说明 |
|--------|------|
| **Custom Build Volume** | 勾选后 **BV Min / Max** 世界坐标拖动编辑 |
| **Use TileCache** | 动态障碍运行时增删（需构建后激活）；显示 TileCache 是否 active |

### 7.3 自动 NavLink（边界）

| 配置项 | 说明 |
|--------|------|
| **Auto NavLinks** | 总开关；基于边界轮廓 |
| 阈值与搜索 | **Jump Up Height**、**Drop Down Max**、**Search Radius**、**Link Radius**、**Min Height Diff**、**Merge Radius** |
| 操作与展示 | **Generate NavLinks**、**Show Auto NavLinks**、生成结果统计（双向/单向下跳计数） |

### 7.4 构建动作与状态

| 内容 | 说明 |
|------|------|
| **(Re)Build NavMesh** | 全宽按钮 |
| 状态文本 | Built 与否、`BuildStatus`、PolyMesh/Detail 多边形与顶点数 |
| **Geometry dirty** | 几何需重建时的橙色提示 |

---

## 8. 寻路查询与路径平滑

**折叠区：Path Query**

| 子类 | 内容 |
|------|------|
| 起终点 | **Start** / **End** 三维 **DragFloat3** |
| 查询 | **Find Path** 按钮；**Status** 文本 |
| 结果统计 | Straight corners、Corridor polys、（可选）Smoothed corners |
| 平滑 | **Smooth Path**；方法 **StringPull** / **Bezier**；Bezier 下 **subdiv**（1～6）。变更时若已有路径会 **重新 Find Path** |

---

## 9. 资源导入与导出

**折叠区：Import / Export**

| 子类 | 内容 |
|------|------|
| OBJ | **Open OBJ…**、当前路径省略显示；成功后重置障碍/选择/相机、按包围盒布置起终点、可自动重建 |
| 几何源切换 | **Use Procedural Geometry**（销毁 Nav、默认程序化、相机、自动重建） |
| NavMesh 二进制 (HNV1) | **Save As…**、**Open NavMesh…**；路径显示；加载成功可 **TryAutoReplan** |

---

## 10. 构建耗时与上下文日志

**折叠区：Stats & Log**

| 内容 | 说明 |
|------|------|
| **Build phases (ms)** 表 | Rasterize、Filter、CompactHF、Erode、DistanceField、Regions、Contours、PolyMesh、DetailMesh、dtCreateNavMesh、Total |
| **rcContext log** | 行数、**Clear** 按钮、可滚动子窗口（自动滚到底） |

---

## 附：不属于左侧面板但相关的 UI

- **右侧画布顶栏**：当前 View / Edit 模式文字、图例颜色（Obstacle、NavMesh、Start、End、Path）。
- **底部浮动窗口「Performance Stats / 性能趋势」**：由 **Stats Window** 开关控制，含构建与寻路历史趋势（非左栏内嵌）。

---

*文档依据源码：`UI/MainLayout.cpp`（左栏结构与顺序）、`UI/Panels.cpp`（各面板控件）。*

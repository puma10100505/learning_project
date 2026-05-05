# TODO

> 用于跟踪当前未完成 / 跨平台后续工作。完成后请把对应条目移到末尾的 "Done" 区或直接删除。

---

## 🪟 RecastNaviDemo — Windows 端 GpuShader 后端

**状态**：未实现（Mac/Linux 已上线，Windows D3D12 后端走 stub fallback）。

**背景**：
- 目录 `Source/RecastNaviDemo/Render/` 现有 `GpuRenderer.h/.cpp`：
  - `!defined(_WIN32)`：完整实现 — OpenGL 3.3 Core + GLSL 330，FBO 离屏 + MSAA + Lambert + ambient 光照。
  - `defined(_WIN32)`：全 stub，`Available()` 返回 `false`。
- Windows 主入口走 `DirectX::CreateWindowInstance`（D3D12 + ImGui DX12 backend），没有可用的 GL 上下文，因此当前 Windows 端选 `GpuShader` 时会安静 fallback 到 None。

**目标**：
让 Windows 用户在 D3D12 后端下也能开 `GpuShader`，且功能、视觉与 Mac/Linux 等价（深度、光照、MSAA、线段加宽、ImGui 集成无缝）。

**实现要点（建议路线，按优先级排序）**

1. **改为运行时多后端，不与 GL 文件混编**：
   - 把 `GpuRenderer.cpp` 拆成 `GpuRendererGL.cpp` 与 `GpuRendererD3D12.cpp`（保持 `GpuRenderer.h` 接口不变），CMake 里用 `if (WIN32) ... else() ...` 选择编译哪个。
2. **D3D12 资源**：
   - 创建一张 RTT：`ID3D12Resource` (`DXGI_FORMAT_R8G8B8A8_UNORM`)，`D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET`。
   - 创建一张 DSV：`DXGI_FORMAT_D24_UNORM_S8_UINT`，`ALLOW_DEPTH_STENCIL`。
   - 双缓冲（避免 ImGui SRV 与渲染同帧 hazard）：`g_S.colorTex[2]`，每帧轮换。
3. **MSAA**：
   - 多采样目标 `D3D12_RESOURCE_DESC.SampleDesc.{Count,Quality}`；
   - 解析阶段用 `ID3D12GraphicsCommandList::ResolveSubresource(srcMS, dstSingle)`。
4. **着色器**：
   - 把 `kVertSrc / kFragSrc` 用相同语义改成 HLSL（vs_5_0 / ps_5_0），`fxc.exe` 离线编译或 `D3DCompile` 在线编译；
   - 顶点结构 `(pos vec3, color vec4, normal vec3, lit float)` 在 D3D12 用同样的输入布局；
   - Lambert 光照参数与 GLSL 端保持一致：`L = normalize(0.45, 0.85, 0.30)`，`ambient = 0.38`。
5. **Pipeline State Object**：
   - 一个 PSO：深度测试 LESS，blend = SRC_ALPHA / ONE_MINUS_SRC_ALPHA，cull none（CPU 端已剔背面），primitive = TRIANGLE_LIST，输出格式与 RTT/DSV 匹配；
   - 多采样数变化时重建 PSO（或多 PSO 缓存）。
6. **ImGui 接入**：
   - DX12 backend 接受 `ImTextureID = (ImTextureID)(intptr_t)(srvHandle.ptr)`；
   - 在 `DirectX12Window` 的 SRV 描述符堆里给 `colorTex` 申请一个 SRV slot，每帧 `dl->AddImage(handle, ...)`；
   - 注意 Mac/Linux GL 路径需要 UV `(0,1)→(1,0)` 翻转，D3D12 NDC y 朝下，**不需要翻转**，对应 `Renderer3D.cpp` 中要根据后端切 UV。
7. **资源生命周期**：
   - canvas 尺寸变化时：等当前帧 fence 完成 → 释放旧 RTT/DSV → 重建。
   - Shutdown 时跟随 `DirectX12Window` 的 Cleanup 一起释放。

**验证清单**：
- [ ] `View > 3D composite = GPU Shader` 在 Windows 下 FPS = 60（vsync），CPU 占用接近 0%。
- [ ] 深度遮挡正确（路径在地面上方、障碍 box 之间互相遮挡）。
- [ ] MSAA 4x / 8x 切换有视觉差异，无 GPU 错误。
- [ ] 光照：地面 / box 顶面亮，box 侧面较暗（与 Mac/Linux 一致）。
- [ ] 切换 None ↔ GpuShader 不漏资源、不闪屏。

**估时**：1–2 个工作日（含 PSO 调优、SRV 描述符堆集成、双缓冲资源管理）。

---

## ✅ Done

- 2026-05-05 RecastNaviDemo: GpuShader 模式（Mac/Linux）— FBO + GLSL 330 + Lambert + MSAA。
- 2026-05-05 RecastNaviDemo: 主场景 FPS HUD；CpuZBuffer 多线程光栅；地面 / 障碍颜色分离；CpuZBuffer 渲染分辨率下拉。
- 2026-05-05 工程：PhysX 头宏 `_DEBUG/NDEBUG` 冲突修复（`SharedDef.cmake` + `LearningFoundation/CMakeLists.txt`）。

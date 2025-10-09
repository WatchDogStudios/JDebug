# JDebug Vulkan Renderer Architecture

## Goals
- Replace the former `RendererCore` dependency with a modern, standalone Vulkan renderer tailored to JDebug.
- Support JVD scene visualization (PVD bodies) and editor features with a lean, explicit rendering API.
- Provide deterministic initialization/shutdown and clear ownership boundaries between engine subsystems.

## Guiding Principles
- **Explicit control:** Expose device, swap-chain, and frame graph primitives without hidden global state.
- **Modular layers:** Separate platform/window integration, Vulkan context management, resource lifetime tracking, and high-level rendering utilities.
- **Incremental migration:** Start with feature parity required by JDebug viewport (clear + simple mesh submission), then grow toward debug rendering, lighting, and post-processing.
- **Testability:** Enable headless validation of command recording and resource creation.

## Layered Design

```
+---------------------------------------------------------------+
|                   JDebug Application Layer                    |
|  (Viewport widget, PVD renderer, editor overlays, tests)      |
+--------------------------↑------------------------------------+
|                 Vulkan Render Front-End Layer                 |
| - `JdVkRenderer` orchestrates frames & render passes          |
| - `JdVkRenderScene` manages cameras, draw lists               |
| - `JdVkResourceManager` tracks GPU buffers & shaders          |
+--------------------------↑------------------------------------+
|            Vulkan Core Context & Platform Layer               |
| - `JdVkInstance`, `JdVkDevice`, `JdVkSwapChain`               |
| - `JdVkMemoryAllocator` (VMA integration)                     |
| - `JdVkCommandPool`, `JdVkQueueContext`                       |
+--------------------------↑------------------------------------+
|                 Platform / Window Abstraction                 |
| - Bridges Qt window handles into `VkSurfaceKHR`               |
| - Coordinates resize, vsync, and presentation                 |
+---------------------------------------------------------------+
```

## Core Modules

| Module | Responsibility |
| --- | --- |
| `VulkanCore` | Instance, device selection, swap-chain creation, lifetime management, debug utilities. |
| `VulkanResources` | Buffer, image, sampler abstractions, descriptor set layouts, shader loading (SPIR-V). |
| `VulkanRenderGraph` | Simple pass system producing frame command buffers; future home for graph-based scheduling. |
| `VulkanFrontend` | Interfaces consumed by gameplay/editor code (`JdVkRenderer`, `JdVkFrameContext`, `JdVkMesh`). |

Each module exposes C++ headers under `Code/VulkanRenderer/<Module>/...` and is grouped in a single `VulkanRenderer` CMake target for now, allowing subdivision later.

## Initialization Flow

1. **Window hookup**
   - Wrap native Win32 handle from `JDebugViewportWidget` into `VkSurfaceKHR` via WSI helper.
2. **Instance & validation layers**
   - Create `VkInstance` using Volk for loader management.
   - Enable standard validation layers when available (controlled via CVar).
   - Register debug messenger for logging.
3. **Physical/logical device selection**
   - Pick queue family supporting graphics + presentation.
   - Create `VkDevice` with required extensions (swap-chain, timeline semaphores).
4. **Memory allocator**
   - Integrate VMA (already shipped with Jolt/ThirdParty) for buffer/image allocation.
5. **Swap-chain**
   - Query surface format, present mode (mailbox preferred, fallback FIFO), extent from window.
   - Create color images + views and a depth attachment (D24S8 or D32S8).
6. **Command infrastructure**
   - Create command pool per frame-in-flight, allocate primary/secondary buffers.
   - Create synchronization primitives (timeline semaphore or F+S fences).
7. **Resource systems**
   - Upload static meshes, create descriptor set layouts (camera, object, material), and pipeline layouts.
   - Compile shaders with DXC → SPIR-V path already in repo (ShaderCompilerDXC); reuse existing `.nsShader` metadata by generating SPIR-V offline.
8. **Frame loop integration**
   - Acquire next image, begin recording, run render graph (clear → draw PVD meshes → UI), submit, present.

## Minimal Feature Milestone (MVP)

- Clear screen, render colored bounding boxes for PVD bodies, swap buffers.
- Support per-frame constant buffer updates (camera transforms) and a single indexed mesh draw path.
- Provide API surface akin to:
  ```cpp
  struct JdVkFrameContext {
    VkCommandBuffer cmd;
    VkExtent2D extent;
  };

  class JdVkRenderer {
  public:
    JdVkRenderer(const JdVkCreateInfo&);
    ~JdVkRenderer();

    JdVkFrameContext BeginFrame();
    void DrawMesh(const JdVkMesh&, const JdVkMaterial&, const JdVkDrawParams&);
    void EndFrame(JdVkFrameContext&);
  };
  ```

## Integration Strategy

1. **Scaffolding**
   - Add `VulkanRenderer` static library target providing instance/device/swap-chain helpers.
   - Expose initialization API consumed by `JDebugViewportWidget`.
2. **Viewport hookup**
   - Replace RenderCore calls with thin wrappers: `UpdateCamera` builds matrices, `QueueDebugGeometry` populates `JdVkRenderScene`.
3. **PVD Renderer Migration**
   - Rewrite `PvdBodyComponent` extraction to feed `JdVkMeshDraw` structures instead of RenderData.
   - Provide simple pipeline with instancing support for spheres/boxes.
4. **Post-processing**
   - Defer AO/bloom until core is stable. Provide optional passes for tonemapping using fullscreen shader once shader compilation flow is ported.
5. **Tooling & Tests**
   - Add smoke test to initialize renderer headlessly (no window) using off-screen surface (VK_KHR_surface + headless extensions). Use Catch2-based test verifying command submission.

## Open Questions
- Shader pipeline: reuse `.nsShader` metadata or adopt pure SPIR-V pipeline descriptions?
- GUI overlays: port existing ImGui extractor to Vulkan (likely via render graph pass).
- Asset pipeline: ensure ShaderCompilerDXC emits Vulkan-compatible SPIR-V permutations with same conventions as .nsShader.
- Platform abstraction: confirm macOS/Linux support (MoltenVK, X11/Wayland) as stretch goals.

## Next Steps
1. Implement `VulkanRenderer` CMake target with volk + Vulkan SDK linkage.
2. Create foundational classes (`JdVkInstance`, `JdVkDevice`, `JdVkSwapChain`).
3. Update `JDebugApp` to initialize the renderer and render a clear color.
4. Incrementally reintroduce mesh rendering and debug draw support.

This document serves as the blueprint for the new Vulkan renderer; subsequent commits should reference the milestones outlined above.

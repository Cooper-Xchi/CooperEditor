# Engine 目录结构

当前工程已经从单文件示例整理成适合继续扩展的最小自研引擎骨架。

## 目录

```text
Engine/
├── include/
│   └── engine/
│       ├── Application.hpp
│       └── vulkan/
│           └── VulkanContext.hpp
├── src/
│   └── engine/
│       ├── Application.cpp
│       └── vulkan/
│           └── VulkanContext.cpp
├── main.cpp
├── CMakeLists.txt
└── *.md
```

## 模块职责

### `main.cpp`

只保留程序入口。

职责：

- 创建 `engine::Application`
- 调用 `run()`
- 统一处理顶层异常

它不再关心 Vulkan 细节。

### `include/engine/Application.hpp`

应用层接口。

职责：

- 表达“应用启动流程”的概念
- 作为以后接窗口系统、主循环、输入系统的入口

### `src/engine/Application.cpp`

应用层实现。

职责：

- 驱动启动流程
- 调用 Vulkan 上下文初始化
- 组织当前阶段的控制台输出

它负责“流程编排”，不负责底层 Vulkan 对象创建细节。

### `include/engine/vulkan/VulkanContext.hpp`

Vulkan 模块公开接口。

职责：

- 定义 `DeviceInfo`
- 定义 `VulkanContext`
- 向上层暴露“已经初始化好的 Vulkan 运行时状态”

### `src/engine/vulkan/VulkanContext.cpp`

Vulkan 模块实现。

职责：

- 查询 Vulkan API 版本
- 创建 `VkInstance`
- 枚举 `VkPhysicalDevice`
- 查询 `Queue Family`
- 选择图形队列族
- 创建 `VkDevice`
- 获取 `VkQueue`
- 负责 Vulkan 生命周期销毁

它是当前阶段最核心的底层模块。

## 为什么这样拆

这次拆分的目标不是“做复杂”，而是先把责任边界分清。

当前边界是：

- `Application`：负责启动和组织流程
- `VulkanContext`：负责 Vulkan 初始化和资源生命周期
- `main.cpp`：只负责程序入口

这个结构适合后续继续长：

- `engine/window/`
- `engine/render/`
- `engine/core/`
- `engine/scene/`

## 下一阶段怎么扩

进入窗口和显示阶段后，建议继续按这个方向扩展：

### `engine/window`

负责：

- GLFW/SDL 初始化
- 创建窗口
- 窗口尺寸与事件

### `engine/vulkan`

继续负责：

- `Surface`
- `Swapchain`
- `RenderPass`
- `Pipeline`
- `CommandBuffer`

### `engine/render`

等渲染流程稳定后，再逐步从 `vulkan` 里抽出更高层渲染概念。

比如：

- Renderer
- FrameGraph
- Material
- Mesh

当前还没到这一步，不要过早抽象。

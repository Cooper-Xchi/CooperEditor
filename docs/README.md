# Vulkan 起步说明

这个仓库现在已经改成了一个最小 Vulkan 工程：

- `CMakeLists.txt` 会查找 Vulkan SDK
- `main.cpp` 会创建 `VkInstance`
- 程序启动后会打印 Vulkan API 版本和可见 GPU

## 1. 先理解 macOS 上的 Vulkan

macOS 没有原生 Vulkan 驱动。

你在 Mac 上跑 Vulkan，通常依赖这条链路：

- 你的程序调用 Vulkan API
- `Vulkan SDK` 提供头文件、loader、工具链
- `MoltenVK` 把 Vulkan 转成 Apple 的 Metal

所以你的目标不是“只装一个库”，而是把 `Vulkan SDK + MoltenVK` 装完整。

## 2. 安装步骤

最稳妥的方法是安装 LunarG 的 macOS Vulkan SDK。

### 方法 A：官方 SDK（推荐新手）

1. 打开 LunarG 官网，下载 macOS 版 Vulkan SDK
2. 安装完成后，确认类似目录存在：

```text
/Users/你的用户名/VulkanSDK/<版本号>/macOS
```

3. 把下面几行加到你的 shell 配置里（`~/.zshrc`）：

```bash
export VULKAN_SDK="$HOME/VulkanSDK/<版本号>/macOS"
export PATH="$VULKAN_SDK/bin:$PATH"
export DYLD_LIBRARY_PATH="$VULKAN_SDK/lib:$DYLD_LIBRARY_PATH"
export VK_LAYER_PATH="$VULKAN_SDK/share/vulkan/explicit_layer.d"
```

4. 重新打开终端后检查：

```bash
echo $VULKAN_SDK
which vulkaninfo
vulkaninfo | head
```

如果 `vulkaninfo` 能运行，说明 SDK 基本装好了。

## 3. 用 CLion 打开这个仓库时要做什么

如果你用 CLion：

1. `File -> Open` 打开这个仓库
2. 确保 CLion 使用的是你的 `zsh` 环境
3. 如果 CMake 报找不到 Vulkan，先重启 CLion
4. 再执行一次 `Reload CMake Project`

关键点是：CLion 必须能读到 `VULKAN_SDK` 环境变量。

## 4. 这个项目现在在做什么

`main.cpp` 做了三件很基础但很关键的事：

1. 查询 Vulkan loader 支持的 API 版本
2. 创建 `VkInstance`
3. 枚举物理设备（GPU）

如果这一步都能跑通，说明你的 Vulkan 环境已经通了，后面才能继续学：

- 队列族
- 逻辑设备
- Swapchain
- Render Pass
- Pipeline
- Command Buffer

## 5. 为什么 macOS 代码里多了一点特殊处理

在 Apple 平台上，这段代码会额外启用：

- `VK_KHR_portability_enumeration`
- `VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR`

这是因为 MoltenVK 通过 portability 路径暴露设备；少这两项时，很多新手会遇到“实例创建成功，但枚举不到 GPU”。

## 6. 你下一步应该怎么学

建议顺序：

1. 先把当前项目跑通，看到 GPU 名字
2. 再学创建 `VkDevice`
3. 再接 GLFW，创建窗口和 Surface
4. 然后学 Swapchain

不要一开始就直接抄完整渲染器；Vulkan 概念很多，新手最容易在“环境没通”和“代码太大”之间一起卡死。

## 7. 当前仓库状态

你原来的仓库只是 CLion 默认的 Hello World，还不算 Vulkan 项目。

现在它至少已经具备了 Vulkan 工程的最小骨架；如果你把 SDK 装好，下一步就可以继续往窗口、设备、交换链推进。

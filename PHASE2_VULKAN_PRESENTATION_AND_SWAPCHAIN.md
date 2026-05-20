# 第二阶段完整文档：窗口显示、Swapchain 与每帧执行

本文档专门对应当前工程第二阶段的代码，也就是：

- 创建 GLFW 窗口
- 创建 Vulkan `Surface`
- 查询 `present` 支持
- 查询并选择 `Swapchain` 参数
- 创建 `Swapchain`
- 获取 `Swapchain Images`
- 创建 `Image View`
- 创建 `Render Pass`
- 创建 `Framebuffer`
- 创建 `Command Pool`
- 分配并录制 `Command Buffer`
- 创建同步对象
- 执行 `Acquire -> Submit -> Present`
- 处理 `Swapchain Recreate`

这份文档针对你当前仓库的模块化架构，而不是旧的单文件示例。

对应文件：

- [src/engine/Application.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/Application.cpp:1)
- [include/engine/window/Window.hpp](/Users/chenhongchi/Desktop/Engine/include/engine/window/Window.hpp:1)
- [src/engine/window/Window.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/window/Window.cpp:1)
- [include/engine/vulkan/VulkanContext.hpp](/Users/chenhongchi/Desktop/Engine/include/engine/vulkan/VulkanContext.hpp:1)
- [include/engine/vulkan/VulkanTypes.hpp](/Users/chenhongchi/Desktop/Engine/include/engine/vulkan/VulkanTypes.hpp:1)
- [src/engine/vulkan/VulkanBootstrap.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanBootstrap.cpp:1)
- [src/engine/vulkan/VulkanDevice.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanDevice.cpp:1)
- [src/engine/vulkan/VulkanSwapchain.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanSwapchain.cpp:1)
- [src/engine/vulkan/VulkanRender.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanRender.cpp:1)
- [src/engine/vulkan/VulkanReporter.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanReporter.cpp:1)

---

## 1. 第二阶段到底解决了什么

第一阶段解决的是：

`程序怎么获得 GPU 工作能力`

第二阶段解决的是：

`GPU 的结果怎么稳定地显示到窗口上`

也就是说，你现在已经不只是“会创建 `VkDevice`”，而是已经走通了完整显示链：

```text
Window
  -> Surface
      -> Swapchain
          -> Swapchain Images
              -> Image Views
                  -> Render Pass
                      -> Framebuffer
                          -> Command Buffer
                              -> Acquire
                                  -> Submit
                                      -> Present
```

这一阶段的结果是：

`一个最小但结构完整的 Vulkan 清屏程序`

它已经具备：

- 持续主循环
- 双帧 in flight
- 独立 `graphicsQueue / presentQueue`
- `OUT_OF_DATE / SUBOPTIMAL` 下的重建能力
- framebuffer resize 触发的重建路径

---

## 2. 当前架构里每个模块在做什么

### `Application`

`Application` 现在只负责顶层流程：

1. 读取并校验配置
2. 创建窗口
3. 初始化 `VulkanContext`
4. 打印当前阶段总结
5. 进入主循环
6. 处理 resize 事件
7. 每帧调用 `drawFrame()`

它不直接操作 Vulkan 细节。

### `Window`

`Window` 负责 GLFW 窗口层：

- `glfwInit`
- 创建窗口
- 事件轮询
- 判断窗口是否关闭
- framebuffer resize 回调
- 查询当前 framebuffer 尺寸

你可以把它理解成：

`平台窗口系统的最小包装层`

### `VulkanBootstrap`

这一层负责：

- 实例扩展准备
- `VkInstance` 创建
- `Surface` 创建

它解决的是：

`先把 Vulkan 和窗口系统真正接上`

### `VulkanDevice`

这一层负责：

- 枚举物理设备
- 查询 queue family
- 查询 `present` 支持
- 选择主设备
- 创建逻辑设备
- 获取 `graphicsQueue / presentQueue`

它解决的是：

`选哪块 GPU，以及通过哪些队列做渲染和呈现`

### `VulkanSwapchain`

这一层负责：

- 查询 `SwapchainSupportDetails`
- 选择 `SwapchainSelection`
- 创建 `VkSwapchainKHR`
- 获取 `Swapchain Images`
- 创建 `Image Views`

它解决的是：

`显示链路的图像资源怎么组织`

### `VulkanRender`

这一层负责：

- `Render Pass`
- `Framebuffer`
- `Command Pool`
- `Command Buffer`
- 同步对象
- 每帧 `Acquire -> Submit -> Present`

它解决的是：

`一帧命令如何真正被 GPU 执行并显示出来`

### `VulkanContext`

`VulkanContext` 是第二阶段的总调度者。

它现在不再把所有底层逻辑堆在一个文件里，而是负责：

- 总生命周期
- `drawFrame()`
- `swapchain recreate`
- 主设备的持有与驱动

---

## 3. 第二阶段最重要的几个新概念

### 3.1 Window

窗口不是 Vulkan 创建的。

当前工程里窗口由 GLFW 创建。

也就是说：

- `Window`：平台层对象
- `Surface`：Vulkan 中连接这个窗口的桥梁对象

### 3.2 Surface

`Surface` 是：

`Vulkan 和窗口系统之间的显示桥梁`

没有它，Vulkan 就不知道图像要显示到哪个窗口。

### 3.3 Swapchain

`Swapchain` 是：

`一组轮流用于显示的图像`

它不是渲染命令容器，也不是单张 RenderTexture。

更准确地说，它像是：

`一组和窗口系统绑定、用于最终上屏的图像缓冲`

### 3.4 Image View

`VkImage` 是图像资源本体。  
`VkImageView` 是：

`告诉 Vulkan：我要怎么把这张图拿来用`

在当前阶段，这些 swapchain image view 会作为颜色附件挂进 framebuffer。

### 3.5 Render Pass

`Render Pass` 是：

`渲染规则说明书`

它描述：

- 这一帧要写什么附件
- 这一帧开始前附件处于什么布局
- 这一帧结束后附件要转成什么布局

### 3.6 Framebuffer

`Framebuffer` 是：

`某一次渲染真正对应的目标实例`

在当前阶段：

- 一个 `RenderPass`
- 多个 `Framebuffer`
- 每个 framebuffer 对应一张 swapchain image view

### 3.7 Command Buffer

`CommandBuffer` 是：

`真正记录 GPU 指令的容器`

当前阶段录进去的是最小命令：

- begin command buffer
- begin render pass
- clear color
- end render pass
- end command buffer

### 3.8 Synchronization

同步对象不是“多余配件”，而是 Vulkan 每帧执行链里最重要的安全机制之一。

当前阶段你已经用到了：

- `Semaphore`
- `Fence`
- 双帧 `in flight`
- `imagesInFlight`

它们共同解决：

- 哪张图这帧能用
- GPU 什么时候能开始写
- 哪张图是不是还被上一帧占着

---

## 4. 第二阶段完整执行链

当前工程一帧的大致流程是：

```text
窗口事件循环
  -> 如果 framebuffer resize，触发 swapchain recreate
  -> 等当前 frame slot 的 fence
  -> acquire 一张 swapchain image
  -> 如果这张 image 还在飞，等它自己的 fence
  -> 提交 command buffer
  -> GPU 写入这张 swapchain image
  -> present 到窗口
  -> currentFrameIndex 在双帧之间轮换
```

这条链就是第二阶段真正意义上的“最小渲染循环”。

---

## 5. 第二阶段新增 Vulkan API 总表

这一阶段你新增接触到的 Vulkan 官方 API 主要有：

1. `vkDestroySurfaceKHR`
2. `vkGetPhysicalDeviceSurfaceSupportKHR`
3. `vkGetPhysicalDeviceSurfaceCapabilitiesKHR`
4. `vkGetPhysicalDeviceSurfaceFormatsKHR`
5. `vkGetPhysicalDeviceSurfacePresentModesKHR`
6. `vkCreateSwapchainKHR`
7. `vkDestroySwapchainKHR`
8. `vkGetSwapchainImagesKHR`
9. `vkCreateImageView`
10. `vkDestroyImageView`
11. `vkCreateRenderPass`
12. `vkDestroyRenderPass`
13. `vkCreateFramebuffer`
14. `vkDestroyFramebuffer`
15. `vkCreateCommandPool`
16. `vkDestroyCommandPool`
17. `vkAllocateCommandBuffers`
18. `vkBeginCommandBuffer`
19. `vkEndCommandBuffer`
20. `vkCmdBeginRenderPass`
21. `vkCmdEndRenderPass`
22. `vkCreateSemaphore`
23. `vkDestroySemaphore`
24. `vkCreateFence`
25. `vkDestroyFence`
26. `vkWaitForFences`
27. `vkResetFences`
28. `vkAcquireNextImageKHR`
29. `vkQueueSubmit`
30. `vkQueuePresentKHR`
31. `vkDeviceWaitIdle`

下面逐个解释。

---

## 6. `vkGetPhysicalDeviceSurfaceSupportKHR`

### 作用

查询：

`某块物理设备的某个 queue family，能不能对当前 surface 做 present`

### 为什么要它

支持 `Graphics` 不等于一定支持 `Present`。

所以必须单独检查：

`这个 queue family 能不能把图像显示到这个窗口对应的 surface 上`

### 当前代码中怎么用

在 `queryPresentSupport(...)` 里，遍历每个 queue family 调这个 API。

最终得到一个：

```cpp
std::vector<bool> presentSupport
```

每个下标对应一个 queue family。

---

## 7. `vkGetPhysicalDeviceSurfaceCapabilitiesKHR`

### 作用

获取：

`当前 surface 对 swapchain 的基础限制`

### 它提供什么信息

最重要的包括：

- `minImageCount`
- `maxImageCount`
- `currentExtent`
- `minImageExtent`
- `maxImageExtent`

### 当前阶段为什么重要

因为创建 swapchain 之前，必须先知道：

- 至少几张 image
- 最多几张 image
- 当前窗口需要什么尺寸

---

## 8. `vkGetPhysicalDeviceSurfaceFormatsKHR`

### 作用

获取：

`当前 surface 支持哪些上屏图像格式`

### 结果是什么

返回的是一组：

```cpp
VkSurfaceFormatKHR
```

每一项包含：

- `format`
- `colorSpace`

### 当前阶段怎么选

当前工程优先选择：

- `VK_FORMAT_B8G8R8A8_SRGB`
- `VK_COLOR_SPACE_SRGB_NONLINEAR_KHR`

找不到就退回第一个支持项。

---

## 9. `vkGetPhysicalDeviceSurfacePresentModesKHR`

### 作用

获取：

`当前 surface 支持哪些 present mode`

### 当前阶段怎么选

当前工程的策略是：

1. 如果关闭了 `preferVSync`，优先尝试 `IMMEDIATE`
2. 如果开启了 `preferMailboxPresentMode`，优先尝试 `MAILBOX`
3. 否则退回 `FIFO`

这就是：

`present mode 的偏好策略`

---

## 10. `vkCreateSwapchainKHR`

### 作用

真正创建：

`VkSwapchainKHR`

### 为什么不能直接创建

因为它依赖前面选好的参数：

- image 数量
- image format
- color space
- extent
- present mode
- sharing mode

### 当前阶段你传进去的关键信息

- `minImageCount`
- `imageFormat`
- `imageColorSpace`
- `imageExtent`
- `imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT`
- `presentMode`
- `imageSharingMode`

### `imageSharingMode` 为什么重要

如果 `graphics family` 和 `present family` 不同，就要考虑两者如何共享 swapchain image。

当前工程里：

- family 相同：`EXCLUSIVE`
- family 不同：`CONCURRENT`

---

## 11. `vkGetSwapchainImagesKHR`

### 作用

从 swapchain 里取出：

`真正的 VkImage 列表`

### 为什么它重要

`VkSwapchainKHR` 本身不是图像，而是图像链管理对象。

真正用于显示的图像，要通过这个 API 取出来。

### 调用模式

它和很多 Vulkan 枚举 API 一样，也是两次调用模式：

1. 第一次拿数量
2. 第二次拿列表

---

## 12. `vkCreateImageView`

### 作用

给每一张 swapchain image 创建一个 `VkImageView`

### 为什么要 image view

因为后面的 framebuffer、render pass attachment 等使用的通常不是裸 `VkImage`，而是 `VkImageView`。

你可以先理解成：

- `VkImage`：资源本体
- `VkImageView`：使用方式

---

## 13. `vkCreateRenderPass`

### 作用

创建一个：

`描述颜色附件渲染规则的 Render Pass`

### 当前阶段的关键配置

当前工程里颜色附件是：

- `loadOp = CLEAR`
- `storeOp = STORE`
- `initialLayout = UNDEFINED`
- `finalLayout = PRESENT_SRC_KHR`

### 这几项怎么理解

- `CLEAR`
  开始渲染前清空颜色附件

- `STORE`
  渲染结束后保留结果

- `UNDEFINED`
  开始前不关心旧内容

- `PRESENT_SRC_KHR`
  结束后把图像布局转成可 present 的状态

---

## 14. `vkCreateFramebuffer`

### 作用

创建：

`Render Pass 的具体渲染目标`

### 当前阶段的关系

- 一个 render pass
- 多个 framebuffer
- 每个 framebuffer 对应一张 swapchain image view

所以 framebuffer 是“这一帧到底画到哪张图上”的具体目标实例。

---

## 15. `vkCreateCommandPool`

### 作用

创建：

`CommandBuffer 的分配来源`

### 为什么 command pool 和 queue family 有关

因为 command buffer 最终是提交给某个 queue family 使用的。

当前阶段 command pool 绑定的是：

`graphicsQueueFamilyIndex`

---

## 16. `vkAllocateCommandBuffers`

### 作用

从 command pool 里分配：

`真正可录命令的 VkCommandBuffer`

### 当前阶段怎么分配

当前工程是：

`每个 framebuffer 分配一个 primary command buffer`

这样一帧拿到哪张 image，就能直接用对应下标的 command buffer。

---

## 17. `vkBeginCommandBuffer`

### 作用

开始录制一个 command buffer。

没有 begin，就不能往里面写命令。

### 当前阶段的意义

它表示：

`这张命令清单现在开始写内容`

---

## 18. `vkCmdBeginRenderPass`

### 作用

在 command buffer 里发出一条命令：

`开始一次 Render Pass`

### 当前阶段做了什么

开始 render pass 时，会：

- 指定要用哪个 render pass
- 指定要画到哪个 framebuffer
- 指定渲染区域
- 指定 clear color

所以它是“真正进入本帧颜色附件渲染阶段”的入口命令。

---

## 19. `vkCmdEndRenderPass`

### 作用

结束当前 render pass。

它和 `vkCmdBeginRenderPass` 成对出现。

当前阶段虽然还没有 pipeline 和 draw call，但已经形成了一个合法的最小 render pass 包围结构。

---

## 20. `vkEndCommandBuffer`

### 作用

结束命令录制。

只有成功结束后，这个 command buffer 才能真正提交给 queue 执行。

---

## 21. `vkCreateSemaphore`

### 作用

创建 GPU-GPU 同步用的信号量。

### 当前阶段怎么用

每个 frame slot 里都有两类 semaphore：

- `imageAvailableSemaphore`
- `renderFinishedSemaphore`

### 它们各自表示什么

- `imageAvailableSemaphore`
  acquire 到的图像现在可以开始渲染了

- `renderFinishedSemaphore`
  这帧渲染已经完成，可以拿去 present 了

---

## 22. `vkCreateFence`

### 作用

创建 CPU-GPU 同步用的 fence。

### 当前阶段怎么用

每个 frame slot 有一个：

- `inFlightFence`

它表示：

`这个 frame slot 上一次提交的 GPU 工作是否已经结束`

---

## 23. `vkWaitForFences`

### 作用

等待 fence 被 GPU 置为 signaled。

### 当前阶段怎么用

当前阶段有两种等待：

1. 等当前 frame slot 的 fence
2. 如果当前 acquire 到的 swapchain image 还在飞，再等它对应的 fence

这就是双帧 in flight 和 image-in-flight 跟踪的关键。

---

## 24. `vkResetFences`

### 作用

把 fence 重置回未触发状态。

### 为什么要它

当前 frame slot 下一次提交前，得先把上次已经 signaled 的 fence 重置掉，才能继续复用。

---

## 25. `vkAcquireNextImageKHR`

### 作用

从 swapchain 里获取：

`当前这一帧可以使用的图像下标`

### 结果是什么

它返回：

- `imageIndex`

表示：

`这帧该画第几张 swapchain image`

### 为什么它很关键

因为：

`不是你自己随便选哪张图，而是系统告诉你现在哪张图能用`

### 当前阶段还处理了什么

如果它返回：

- `VK_ERROR_OUT_OF_DATE_KHR`
- `VK_SUBOPTIMAL_KHR`

当前工程会走 `swapchain recreate`

---

## 26. `vkQueueSubmit`

### 作用

把 command buffer 提交给图形队列执行。

### 当前阶段的关键关系

提交时会携带：

- 等待 `imageAvailableSemaphore`
- 执行对应 `commandBuffer`
- 完成后 signal `renderFinishedSemaphore`
- 绑定当前 frame slot 的 `inFlightFence`

所以它解决的是：

`这一帧的图形命令何时开始、执行什么、结束后发什么信号`

---

## 27. `vkQueuePresentKHR`

### 作用

把渲染好的 swapchain image 交给显示系统去呈现。

### 为什么它不是自动发生的

因为 Vulkan 里“画完”不等于“自动上屏”。

你必须显式 present。

### 当前阶段怎么用

present 会等待：

- `renderFinishedSemaphore`

然后把：

- swapchain
- imageIndex

交给显示系统。

---

## 28. `vkDeviceWaitIdle`

### 作用

等待整个逻辑设备上的工作全部完成。

### 当前阶段在哪些地方用

主要在：

- `shutdown()`
- `recreateSwapchain()`

### 为什么这里可以接受

它比较重，但在：

- 程序退出
- swapchain 重建

这种低频操作里完全合理。

不能把它放进每帧主路径，这一点你前面已经优化掉了。

---

## 29. 第二阶段新增的 Destroy API 为什么重要

你这一阶段还用到了很多 destroy API：

- `vkDestroySurfaceKHR`
- `vkDestroySwapchainKHR`
- `vkDestroyImageView`
- `vkDestroyRenderPass`
- `vkDestroyFramebuffer`
- `vkDestroyCommandPool`
- `vkDestroySemaphore`
- `vkDestroyFence`

这些 API 的共同特点是：

`创建出来的 Vulkan 资源，通常都要你自己按依赖顺序显式销毁`

第二阶段比第一阶段更能体现 Vulkan 的资源管理风格：

`不是只会创建，而是必须能正确回收和重建`

---

## 30. 第二阶段最重要的 C++ / GLFW API

这份文档主要讲 Vulkan，但第二阶段还新增了两个非常重要的非 Vulkan API：

### `glfwCreateWindowSurface`

作用：

`把 GLFW 窗口接成 Vulkan 可识别的 Surface`

它不是 Vulkan 官方函数，但它是第二阶段里窗口和 Vulkan 接起来的关键桥梁。

### `glfwSetFramebufferSizeCallback`

作用：

当 framebuffer 尺寸变化时通知程序。

它让你后面能够触发：

`swapchain recreate`

---

## 31. 当前阶段的数据结构到底在保存什么

当前最关键的是 [VulkanTypes.hpp](/Users/chenhongchi/Desktop/Engine/include/engine/vulkan/VulkanTypes.hpp:1)

你现在应该重点认识这几个结构。

### `SwapchainSupportDetails`

保存：

- capabilities
- formats
- present modes

它是：

`创建 swapchain 之前查到的支持范围`

### `SwapchainSelection`

保存最终选择出来的：

- imageCount
- surfaceFormat
- presentMode
- extent

它是：

`真正准备喂给 vkCreateSwapchainKHR 的核心参数`

### `FrameSyncObjects`

保存每个 frame slot 的：

- image available semaphore
- render finished semaphore
- in-flight fence

它是：

`一帧同步资源的打包结构`

### `DeviceInfo`

这是当前第二阶段最重的结构。

它保存：

- 物理设备信息
- queue family 信息
- graphics/present queue
- swapchain 资源
- render pass / framebuffer
- command pool / command buffer
- frames in flight
- images in flight
- 是否已经提交过帧

你可以先把它理解成：

`当前主设备从“硬件发现”到“每帧呈现”的完整状态快照`

---

## 32. 第二阶段源码主线是怎么跑起来的

### 32.1 `Application::run()`

主流程是：

1. 读配置
2. 校验配置
3. 创建窗口
4. 初始化 VulkanContext
5. 打印总结
6. 进入主循环
7. 处理 resize
8. 调用 `drawFrame()`

### 32.2 `VulkanContext::initialize()`

当前顺序是：

1. 清理旧状态
2. 查询 runtime API version
3. 创建实例
4. 创建 surface
5. 发现设备并初始化主设备资源

### 32.3 `discoverDevices()`

这一步做了两类工作：

1. 枚举所有物理设备，并保留它们的基础信息
2. 选择主设备，并只对主设备继续创建显示链资源

这是当前架构里非常重要的改进点：

`不是每块枚举到的设备都完整初始化，而是先选主设备，再把真正的初始化成本集中在主设备上`

### 32.4 `createSwapchainResources()`

这是第二阶段最关键的“可重建资源创建入口”。

它会：

1. 重新查询 swapchain support
2. 重新选择 swapchain 参数
3. 创建 swapchain
4. 获取 swapchain images
5. 创建 image views
6. 创建 render pass
7. 创建 framebuffers
8. 创建 command pool
9. 分配 command buffers
10. 录制 command buffers
11. 创建双帧同步对象

你可以把它理解成：

`第二阶段显示链资源总装函数`

### 32.5 `drawFrame()`

当前 `drawFrame()` 只驱动主设备。

它会：

1. 检查主设备的 swapchain 和同步资源是否有效
2. 调用 `submitSingleFrame(...)`
3. 如果需要 recreate，就走 `recreateSwapchain(...)`
4. 否则标记这一帧提交成功

### 32.6 `recreateSwapchain()`

这一步会：

1. 等待 framebuffer 尺寸恢复有效
2. `vkDeviceWaitIdle`
3. 销毁旧 swapchain 相关资源
4. 重新创建 swapchain 相关资源

这是第二阶段后半段最重要的工程能力之一。

---

## 33. 第二阶段为什么现在还不能画三角形

虽然你已经有了：

- render pass
- framebuffer
- command buffer
- submit
- present

但你现在还没有：

- shader
- pipeline layout
- graphics pipeline
- vertex input
- draw call

当前 command buffer 录进去的只是：

`清屏命令`

所以现在的程序本质上是：

`最小清屏呈现程序`

不是三角形程序。

这正是第三阶段要解决的问题。

---

## 34. 第二阶段最重要的结论

如果把这一阶段压缩成最值得记住的 8 句话，就是：

1. `Window` 是平台窗口，`Surface` 是 Vulkan 里的显示桥梁。
2. `Swapchain` 是一组和窗口绑定、轮流上屏的图像。
3. 创建 swapchain 前必须先查 capabilities / formats / present modes。
4. `ImageView` 决定怎么使用 swapchain image。
5. `RenderPass + Framebuffer` 定义了“往哪张图按什么规则渲染”。
6. `CommandBuffer` 记录 GPU 真正要执行的命令。
7. 每一帧都要走 `Acquire -> Submit -> Present`。
8. `Swapchain` 不是一次性对象，必须能在 `OUT_OF_DATE / resize` 下重建。

---

## 35. 这一阶段完成后，你已经具备了什么

到第二阶段结束时，你已经真正拥有了：

- 一个 GLFW 窗口
- 一套 Vulkan 显示链
- 一条每帧执行链
- 一套双帧同步模型
- 一条 swapchain 重建链
- 一份配置和路径系统支撑的基础工程骨架

这已经足够作为第三阶段的起点。

换句话说：

`你现在不是“刚会创建 Vulkan 设备”，而是已经拥有了一个能稳定显示清屏结果的最小 Vulkan 渲染框架。`

---

## 36. 第二阶段源码逐函数拆解

这一节专门按当前代码里的真实函数来讲。

目标不是再重复概念，而是把：

- 这个函数输入什么
- 它内部做了什么
- 它输出什么状态
- 它在整条链里处于哪一层

全部串起来。

---

## 37. `Window::initialize()`

文件：
[Window.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/window/Window.cpp:25)

### 作用

初始化 GLFW，并创建一个 Vulkan 用窗口。

### 它内部做了什么

1. 先 `shutdown()` 清理旧窗口状态
2. 读取窗口配置
3. 调用 `glfwInit()`
4. 设置 `GLFW_CLIENT_API = GLFW_NO_API`
5. 设置 `GLFW_RESIZABLE`
6. 调用 `glfwCreateWindow(...)`
7. 绑定 `Window` 自身到 GLFW user pointer
8. 注册 framebuffer resize 回调

### 为什么 `GLFW_NO_API` 很重要

因为这里明确告诉 GLFW：

`这个窗口不是给 OpenGL 上下文用的，后面由 Vulkan 接管`

### 这一步之后你得到了什么

你得到了一个：

- GLFWwindow

但这还不是 Vulkan 能直接使用的对象。

下一步还要通过 `glfwCreateWindowSurface(...)` 变成 `Surface`。

---

## 38. `Window::framebufferResizeCallback(...)`

文件：
[Window.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/window/Window.cpp:11)

### 作用

当窗口 framebuffer 尺寸变化时，设置一个标记。

### 为什么不直接在回调里重建 swapchain

因为 GLFW 回调只是平台事件通知点。

当前工程选择更稳的方式：

- 回调里只记录“发生过 resize”
- 真正的 Vulkan 重建放到主循环里做

这能避免把大量 Vulkan 操作塞进平台回调。

### 它改了哪个状态

```cpp
framebufferResized_ = true;
```

所以它只是一个事件标记器。

---

## 39. `Window::consumeFramebufferResized()`

文件：
[Window.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/window/Window.cpp:74)

### 作用

读取并清空 resize 标记。

### 为什么叫 consume

因为它不是“只看一眼”，而是：

- 读出当前值
- 然后把标记清回 `false`

这是一种非常典型的“事件消费”写法。

---

## 40. `Window::framebufferSize()`

文件：
[Window.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/window/Window.cpp:76)

### 作用

获取当前 framebuffer 的真实尺寸。

### 为什么这个函数重要

因为 swapchain recreate 时不能只看“窗口发生过 resize”，还要看：

`当前 framebuffer 尺寸是不是合法`

有些平台在窗口最小化时可能返回：

```text
0 x 0
```

这时不能直接创建 swapchain。

---

## 41. `Application::run()`

文件：
[Application.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/Application.cpp:13)

### 作用

这是第二阶段应用层总入口。

### 主流程拆解

1. 读取配置
2. 校验配置
3. 创建窗口
4. 初始化 VulkanContext
5. 打印阶段总结
6. 进入主循环
7. 每帧轮询事件
8. 如果窗口 resized，就通知 VulkanContext 重建 swapchain
9. 调用 `drawFrame()`
10. 第一次成功 present 后打印首帧成功信息

### 这一层为什么重要

因为它把第二阶段所有东西真正串成了：

`初始化 -> 主循环 -> 事件 -> 渲染 -> 呈现`

这已经是一个最小引擎主循环的形状了。

---

## 42. `VulkanContext::initialize(...)`

文件：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:139)

### 作用

启动整个 Vulkan 子系统。

### 内部顺序

1. `shutdown()` 清空旧状态
2. 保存窗口引用到 `window_`
3. 查询 runtime API version
4. 创建实例
5. 创建 surface
6. 发现设备并初始化主设备

### 为什么保存 `window_`

因为第二阶段后半段已经需要：

- 查询 framebuffer 尺寸
- 等待窗口恢复有效尺寸

所以 `VulkanContext` 后面在 `swapchain recreate` 时需要访问窗口对象。

---

## 43. `VulkanContext::createInstance()`

文件：
[VulkanBootstrap.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanBootstrap.cpp:55)

### 作用

创建 `VkInstance`

### 第二阶段和第一阶段相比有什么不同

第一阶段创建实例时，主要是为了进入 Vulkan 世界。

第二阶段这里还多了一个新要求：

`要把窗口系统所需的实例扩展也启用进来`

这就是为什么会调用：

```cpp
glfwGetRequiredInstanceExtensions(...)
```

### 这一步之后得到了什么

程序正式拥有一个 Vulkan instance，并且这个 instance 已经具备创建 surface 的能力。

---

## 44. `VulkanContext::createSurface(...)`

文件：
[VulkanBootstrap.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanBootstrap.cpp:82)

### 作用

把 GLFW 窗口转成 Vulkan `Surface`

### 核心 API

```cpp
glfwCreateWindowSurface(...)
```

### 为什么这一步关键

它就是：

`Window -> Surface`

这条链第一次落地的地方。

没有它，后面所有：

- present support 查询
- swapchain support 查询
- swapchain 创建

都没法做。

---

## 45. `VulkanContext::discoverDevices()`

文件：
[VulkanDevice.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanDevice.cpp:89)

### 作用

枚举所有物理设备，并从中选择主设备。

### 它内部做了什么

对每块物理设备依次做：

1. 读取物理设备属性
2. 查询 queue family
3. 查询 `presentSupport`
4. 查询 `swapchainSupport`
5. 选择 graphics family
6. 选择 present family
7. 如果设备满足第二阶段需要的条件，就可以成为主设备候选

### 当前工程的关键设计

不是所有设备都完整初始化。

而是：

- 所有设备都保留枚举信息
- 只有第一个满足条件的设备被标记成 `isPrimaryDevice`
- 只有主设备继续创建：
  - `VkDevice`
  - queues
  - swapchain 资源

### 为什么这是优化

因为如果你枚举到 2 块或更多物理设备，而每块都完整建一套 swapchain / render pass / command pool / sync，这会浪费很多初始化成本，而且语义也不对。

第二阶段真正需要的是：

`选一个主设备来驱动窗口渲染`

---

## 46. `detail::queryPresentSupport(...)`

文件：
[VulkanSwapchain.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanSwapchain.cpp:7)

### 作用

查询每个 queue family 对当前 surface 是否支持 present。

### 为什么返回 `std::vector<bool>`

因为当前工程想保留“每个队列族的 present 能力表”。

这样后面：

- 打印总结时能显示 `[Present]`
- 选 present family 时也能直接遍历这张表

---

## 47. `detail::querySwapchainSupport(...)`

文件：
[VulkanSwapchain.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanSwapchain.cpp:25)

### 作用

一次性把创建 swapchain 前需要的三类信息全查出来：

- capabilities
- formats
- present modes

### 为什么它是第二阶段核心函数之一

因为第二阶段后面几乎所有“参数选择”都建立在它返回的数据上。

你可以把它理解成：

`显示链路配置空间查询入口`

---

## 48. `detail::chooseSwapchainSelection(...)`

文件：
[VulkanSwapchain.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanSwapchain.cpp:122)

### 作用

从支持列表里选出一组真正要用的 swapchain 参数。

### 它内部依赖哪些小函数

- `chooseImageCount(...)`
- `chooseSurfaceFormat(...)`
- `choosePresentMode(...)`
- `chooseExtent(...)`

### 输出是什么

输出一个：

```cpp
SwapchainSelection
```

也就是：

`真正准备拿去创建 swapchain 的参数集合`

---

## 49. `detail::createLogicalDevice(...)`

文件：
[VulkanDevice.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanDevice.cpp:35)

### 作用

基于主设备的 queue family 需求创建 `VkDevice`

### 第二阶段这里的关键变化

这一步不再只创建 graphics queue 对应的设备队列请求。

现在会根据：

- `graphicsQueueFamilyIndex`
- `presentQueueFamilyIndex`

决定申请：

- 一套队列
或
- 两套队列

### 为什么这比第一阶段更完整

因为第二阶段已经涉及：

- 图形工作
- 呈现工作

虽然你当前机器上两个 family 可能是同一个，但架构上已经为“不同 family”准备好了。

---

## 50. `detail::acquireQueue(...)`

文件：
[VulkanDevice.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanDevice.cpp:79)

### 作用

从 `VkDevice` 里取出某个 family 对应的 queue

### 为什么这里不再叫 `acquireGraphicsQueue`

因为第二阶段已经不仅有 graphics queue，还可能有 present queue。

所以这里被抽成更通用的：

`按 queue family 取队列`

---

## 51. `VulkanContext::createSwapchainResources(...)`

文件：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:69)

### 作用

创建所有“跟 swapchain 生命周期绑定”的资源。

### 它内部顺序

1. 重新查询 `swapchainSupport`
2. 重新查询 `presentSupport`
3. 重新选择 `presentQueueFamilyIndex`
4. 检查 formats / present modes / queue family 是否都有效
5. 重新选择 `swapchainSelection`
6. 创建 `swapchain`
7. 获取 `swapchainImages`
8. 创建 `swapchainImageViews`
9. 创建 `renderPass`
10. 创建 `framebuffers`
11. 创建 `commandPool`
12. 分配 `commandBuffers`
13. 录制 `commandBuffers`
14. 创建双帧同步对象
15. 初始化 `imagesInFlight`

### 为什么它非常重要

它是当前第二阶段“最值钱”的一个整理结果。

因为现在：

- 第一次初始化用它
- `swapchain recreate` 也用它

所以显示链资源不再散在多个地方重复写。

---

## 52. `VulkanContext::destroySwapchainResources(...)`

文件：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:31)

### 作用

销毁所有和 swapchain 生命周期绑定的资源。

### 当前会销毁哪些东西

- frame sync objects
- command pool
- command buffers
- framebuffers
- render pass
- image views
- swapchain

### 为什么它和 `shutdown()` 不一样

`shutdown()` 是全局停机。  
`destroySwapchainResources()` 是：

`只拆掉可重建的显示链资源，但保留逻辑设备和队列`

这正是第二阶段后半段“可重建架构”的关键。

---

## 53. `VulkanContext::recreateSwapchain(...)`

文件：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:128)

### 作用

在 surface 失效或尺寸变化后，重建显示链资源。

### 当前顺序

1. 等待 framebuffer 尺寸恢复有效
2. `vkDeviceWaitIdle`
3. 销毁旧 swapchain 资源
4. 重新创建 swapchain 资源

### 为什么要先等有效尺寸

因为有些平台下窗口最小化时 framebuffer 可能是：

```text
0 x 0
```

这时如果硬创建 swapchain，通常会失败。

所以现在工程里专门加入了：

- `framebufferSize()`
- `waitEvents()`
- `waitForValidFramebufferSize()`

这是第二阶段后半段很重要的一项健壮性改进。

---

## 54. `VulkanContext::waitForValidFramebufferSize()`

文件：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:7)

### 作用

等待窗口 framebuffer 尺寸恢复成合法值。

### 内部逻辑

1. 从 `window_` 读取当前 framebuffer 尺寸
2. 如果是 `0x0`
   - 先看窗口是否要关闭
   - 否则 `waitEvents()`
   - 再重新查询尺寸
3. 直到尺寸不是 0

### 这解决了什么坑

它解决的是：

`resize/recreate 链在窗口最小化时误创建 swapchain`

---

## 55. `detail::recordCommandBuffers(...)`

文件：
[VulkanRender.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanRender.cpp:117)

### 作用

把当前阶段的最小渲染命令录进每个 command buffer。

### 当前录了什么

1. begin command buffer
2. 设置 clear color
3. begin render pass
4. end render pass
5. end command buffer

### 为什么这里还没 draw call

因为你现在还没有：

- shader
- pipeline
- vertex data

所以当前阶段是：

`清屏渲染命令`

不是三角形命令。

---

## 56. `detail::createFrameSyncObjects(...)`

文件：
[VulkanRender.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanRender.cpp:178)

### 作用

创建：

`多帧 in flight 所需的一组同步对象`

### 当前阶段为什么不再只保留一套 sync

因为第二阶段后半段已经升级成了：

`双帧 in flight`

所以每个 frame slot 都需要：

- image available semaphore
- render finished semaphore
- in-flight fence

这就是：

```cpp
std::vector<FrameSyncObjects> framesInFlight;
```

---

## 57. `detail::submitSingleFrame(...)`

文件：
[VulkanRender.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanRender.cpp:188)

### 作用

执行第二阶段真正的一帧：

`Acquire -> Submit -> Present`

### 详细拆解

#### 第一步：选择当前 frame slot

```cpp
auto& currentFrame = framesInFlight[currentFrameIndex];
```

表示：

`当前轮到第几个 in-flight frame 槽`

#### 第二步：等当前 frame slot 完成

```cpp
vkWaitForFences(...)
vkResetFences(...)
```

表示：

`这一个 frame slot 上次提交的 GPU 工作必须先结束，才能复用`

#### 第三步：Acquire 一张 swapchain image

```cpp
vkAcquireNextImageKHR(...)
```

返回：

- `imageIndex`

当前还会处理：

- `VK_ERROR_OUT_OF_DATE_KHR`
- `VK_SUBOPTIMAL_KHR`

如果需要，就返回：

```cpp
FrameSubmitStatus::RecreateSwapchain
```

#### 第四步：如果这张 image 还在飞，先等它

```cpp
if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
    vkWaitForFences(...)
}
```

然后：

```cpp
imagesInFlight[imageIndex] = currentFrame.inFlightFence;
```

这表示：

`这张 swapchain image 现在归当前 frame slot 管`

#### 第五步：Submit

提交时会：

- 等 `imageAvailableSemaphore`
- 执行 `commandBuffers[imageIndex]`
- 完成后 signal `renderFinishedSemaphore`
- 把当前 frame 的 fence 绑到这次提交

#### 第六步：Present

present 时会等待：

- `renderFinishedSemaphore`

然后把：

- `swapchain`
- `imageIndex`

交给显示系统。

#### 第七步：frame slot 轮换

```cpp
currentFrameIndex =
    (currentFrameIndex + 1) % framesInFlight.size();
```

这就是双帧 in flight 的轮换。

### 为什么它是第二阶段最关键的函数

因为它第一次把你前面学过的这些概念都压到一帧里了：

- swapchain image
- frame sync
- command buffer
- queue submit
- queue present
- image ownership tracking
- recreate condition

这就是第二阶段从“概念学习”变成“真实渲染循环”的转折点。

---

## 58. `VulkanContext::drawFrame()`

文件：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:175)

### 作用

驱动主设备执行一帧。

### 当前阶段的重要改进

它不再每帧遍历所有设备，而是：

- 先看 `primaryDeviceIndex_` 是否存在
- 只驱动主设备

### 这为什么比以前更合理

因为第二阶段真正要做的是：

`选一个主设备负责窗口渲染`

而不是：

`每帧试试看谁能跑`

这也是你前面补掉的一个工程坑。

---

## 59. `VulkanContext::handleFramebufferResize()`

文件：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:203)

### 作用

当窗口层告诉 Vulkan：

`framebuffer 已经变化`

时，主动触发主设备的 swapchain 重建。

### 当前结构为什么合理

它没有在 GLFW 回调里直接重建。

而是：

- 回调只打标记
- 主循环消费标记
- 再调用 `handleFramebufferResize()`

这样 Vulkan 重建逻辑仍然集中在渲染上下文里。

---

## 60. 第二阶段源码逐函数复盘的最终结论

如果把这些函数再压缩成一句话：

- `Window` 负责窗口事件和尺寸
- `Application` 负责把窗口事件交给 Vulkan
- `VulkanBootstrap` 负责 instance 和 surface
- `VulkanDevice` 负责主设备和队列
- `VulkanSwapchain` 负责显示图像链
- `VulkanRender` 负责一帧命令和同步
- `VulkanContext` 负责把这些模块组织成“可初始化、可每帧执行、可重建”的完整系统

所以第二阶段真正学到的，不只是 API 名字，而是：

`如何把一套 Vulkan 显示链组织成一个能长期维护的最小框架`

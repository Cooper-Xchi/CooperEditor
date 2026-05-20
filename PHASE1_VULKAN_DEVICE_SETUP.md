# 第一阶段完整文档：Vulkan 设备初始化

本文档专门对应当前工程第一阶段的代码，也就是：

- 查询 Vulkan 运行时版本
- 创建 `VkInstance`
- 枚举 `VkPhysicalDevice`
- 查询 `Queue Family`
- 选择图形队列族
- 创建 `VkDevice`
- 获取 `VkQueue`

这份文档不是针对旧的单文件示例，而是针对你当前已经整理好的模块化架构。

对应文件：

- [main.cpp](/Users/chenhongchi/Desktop/Engine/main.cpp:1)
- [include/engine/Application.hpp](/Users/chenhongchi/Desktop/Engine/include/engine/Application.hpp:1)
- [src/engine/Application.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/Application.cpp:1)
- [include/engine/vulkan/VulkanContext.hpp](/Users/chenhongchi/Desktop/Engine/include/engine/vulkan/VulkanContext.hpp:1)
- [src/engine/vulkan/VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:1)

---

## 1. 这一阶段到底做了什么

当前阶段还没有窗口、没有 `Surface`、没有 `Swapchain`、没有 `Pipeline`、没有绘制命令。

它做的事情只有一条主线：

```text
程序启动
  -> 查询 Vulkan API 版本
  -> 创建 VkInstance
  -> 找出所有 VkPhysicalDevice
  -> 查询每块设备的队列族
  -> 找到支持 Graphics 的队列族
  -> 创建 VkDevice
  -> 取出 VkQueue
```

如果用“工厂”比喻来描述：

- `VkPhysicalDevice`：工厂本体
- `Queue Family`：生产线类别
- `VkDevice`：你的工作台
- `VkQueue`：真正派活的通道

所以这一阶段可以叫：

`设备初始化阶段`

或者更准确一点：

`让程序获得 GPU 工作能力的阶段`

---

## 2. 当前架构里每个模块在做什么

### `main.cpp`

这里只做三件事：

1. 创建 `engine::Application`
2. 调用 `run()`
3. 统一捕获顶层异常

它不再直接操作 Vulkan。

### `Application`

`Application` 负责应用层流程编排。

当前做的事情：

1. 创建 `VulkanContext`
2. 调用 `initialize()`
3. 把 Vulkan 查询到的结果打印出来

它负责“流程组织”，不负责底层 Vulkan 创建细节。

### `VulkanContext`

`VulkanContext` 是当前阶段最核心的类。

它负责：

1. 查询运行时 Vulkan 版本
2. 创建实例
3. 枚举物理设备
4. 查询队列族
5. 选择图形队列族
6. 创建逻辑设备
7. 获取图形队列
8. 销毁 Vulkan 资源

换句话说，当前阶段所有真正有学习价值的 Vulkan API，都集中在 `VulkanContext.cpp` 里。

---

## 3. 当前阶段涉及到的 Vulkan 官方 API 列表

这份代码里真正调用到的 Vulkan 官方函数有：

1. `vkGetInstanceProcAddr`
2. `vkCreateInstance`
3. `vkEnumeratePhysicalDevices`
4. `vkGetPhysicalDeviceQueueFamilyProperties`
5. `vkGetPhysicalDeviceProperties`
6. `vkCreateDevice`
7. `vkGetDeviceQueue`
8. `vkDestroyDevice`
9. `vkDestroyInstance`

此外还大量用到了 Vulkan 的：

- 句柄类型
- 结构体
- 宏
- 标志位

比如：

- `VkInstance`
- `VkPhysicalDevice`
- `VkDevice`
- `VkQueue`
- `VkApplicationInfo`
- `VkInstanceCreateInfo`
- `VkDeviceQueueCreateInfo`
- `VkDeviceCreateInfo`
- `VK_QUEUE_GRAPHICS_BIT`
- `VK_SUCCESS`
- `VK_NULL_HANDLE`

下面按执行顺序展开。

---

## 4. `vkGetInstanceProcAddr` 是做什么的

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:13)

```cpp
vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion")
```

### 作用

`vkGetInstanceProcAddr` 用来：

`按名字查询 Vulkan 函数入口地址`

你可以先把它理解成：

“去 Vulkan loader 里找一个函数”

### 为什么需要它

Vulkan 不是所有函数都一定能直接静态可用。

有些函数是：

- 新版本才有
- 扩展提供的
- 需要运行时查询

所以 Vulkan 提供了 `vkGetInstanceProcAddr`。

### 当前代码里它具体在干什么

当前它是在找：

```cpp
"vkEnumerateInstanceVersion"
```

也就是说：

“当前运行环境里，有没有 `vkEnumerateInstanceVersion` 这个函数？”

如果有，就拿到它的函数地址。

### 参数解释

```cpp
vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion")
```

- 第一个参数 `nullptr`
  表示这次查询不依赖某个已经存在的实例对象

- 第二个参数 `"vkEnumerateInstanceVersion"`
  表示要查询的函数名

### 返回值

它返回的是一个“通用函数地址”。

所以代码里要做：

```cpp
reinterpret_cast<PFN_vkEnumerateInstanceVersion>(...)
```

把它转成正确的函数指针类型。

---

## 5. `vkEnumerateInstanceVersion` 是做什么的

当前代码里不是直接调用，而是通过函数指针调用。

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:18)

```cpp
enumerateInstanceVersion(&version);
```

### 作用

`vkEnumerateInstanceVersion` 用来查询：

`当前 Vulkan loader 支持的实例级 API 版本`

### 这里为什么不是字符串版本号

Vulkan 版本不是字符串存储，而是打包进一个 `uint32_t`。

所以这里传入的是：

```cpp
&version
```

表示把查询结果写入这个变量。

### 当前代码的逻辑

```cpp
uint32_t version = VK_API_VERSION_1_0;
```

先给一个默认保底值 `1.0`。

然后如果查询函数存在，就让它把真实版本写进去。

### 结果的意义

这个结果表示：

`当前系统这套 Vulkan 运行时，大概支持到哪一代 Vulkan API`

例如你看到的：

```text
Vulkan loader API: 1.4.350
```

就是从这里来的。

---

## 6. `vkCreateInstance` 是做什么的

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:145)

```cpp
vkCreateInstance(&createInfo, nullptr, &instance_)
```

### 作用

`vkCreateInstance` 用来创建 Vulkan 实例对象 `VkInstance`

你可以把 `VkInstance` 理解成：

`程序进入 Vulkan 世界后的总入口对象`

没有它，你后面几乎什么都做不了。

### 参数解释

```cpp
vkCreateInstance(&createInfo, nullptr, &instance_)
```

- 第一个参数 `&createInfo`
  实例创建配置

- 第二个参数 `nullptr`
  不使用自定义分配器

- 第三个参数 `&instance_`
  输出参数，把创建好的实例写到这里

### 它内部大致发生什么

从概念上看，它大致会做这些事情：

1. Vulkan loader 检查你的创建信息是否合法
2. 检查你要求的扩展是否可用
3. 初始化实例级状态
4. 返回一个可用的 `VkInstance`

### 为什么它是这一阶段最关键的第一个创建函数

因为后面的物理设备枚举、surface、debug callback 等，都建立在实例之上。

---

## 7. `vkEnumeratePhysicalDevices` 是做什么的

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:154)

```cpp
vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr)
vkEnumeratePhysicalDevices(instance_, &deviceCount, physicalDevices.data())
```

### 作用

它用来：

`枚举当前 Vulkan 实例可见的物理设备`

物理设备通常就是 GPU。

### 为什么调用两次

这是 Vulkan/C 风格 API 的经典模式：

1. 第一次查询数量
2. 分配容器
3. 第二次读取真正的数据

### 第一次调用

```cpp
vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr)
```

表示：

“先不要设备列表，先告诉我有多少个设备”

### 第二次调用

```cpp
vkEnumeratePhysicalDevices(instance_, &deviceCount, physicalDevices.data())
```

表示：

“我现在已经准备好了数组，请把设备对象写进来”

### 参数解释

- `instance_`
  从哪个实例里枚举设备

- `&deviceCount`
  输入输出参数，用来返回数量

- `nullptr` 或 `physicalDevices.data()`
  第一次不要列表，第二次要真正数据

### 当前阶段它为什么重要

因为只有拿到 `VkPhysicalDevice`，你后面才能继续：

- 查属性
- 查队列族
- 创建设备

---

## 8. `vkGetPhysicalDeviceProperties` 是做什么的

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:169)

```cpp
vkGetPhysicalDeviceProperties(physicalDevice, &deviceInfo.properties);
```

### 作用

读取某块物理设备的基础属性。

比如：

- 设备名
- API 版本
- 驱动版本
- 设备类型
- 厂商 ID

### 参数解释

- 第一个参数 `physicalDevice`
  要查询哪块设备

- 第二个参数 `&deviceInfo.properties`
  输出结构体，Vulkan 会把结果写进去

### 当前代码里它的用途

最直接的用途是打印：

```cpp
device.properties.deviceName
```

所以终端里才会有：

```text
[0] Apple M5 Pro
```

---

## 9. `vkGetPhysicalDeviceQueueFamilyProperties` 是做什么的

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:33)

```cpp
vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
                                         &queueFamilyCount,
                                         nullptr);
vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
                                         &queueFamilyCount,
                                         queueFamilies.data());
```

### 作用

这个 API 用来查询：

`某块物理设备有哪些队列族，以及每个队列族支持什么能力`

### 为什么也调用两次

原因和枚举物理设备一样：

1. 第一次查数量
2. 分配数组
3. 第二次查具体数据

### 队列族是什么

你可以先把 `Queue Family` 理解成：

`一组能力相同的队列`

比如一个队列族可能支持：

- 图形
- 计算
- 传输

### 当前代码里这个 API 的作用

它支撑了两件事：

1. 把所有 queue family 信息保存到 `deviceInfo.queueFamilies`
2. 帮助后面查找支持 `Graphics` 的队列族

### 为什么这一阶段必须学它

因为 Vulkan 不允许你模糊地说“给我一个设备然后开始画图”。

它要求你先明确：

`我从哪个 queue family 里拿队列`

---

## 10. `vkCreateDevice` 是做什么的

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:73)

```cpp
vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice)
```

### 作用

基于某个 `VkPhysicalDevice`，创建一个 `VkDevice`。

`VkDevice` 可以理解成：

`程序真正用于操作这块 GPU 的逻辑设备对象`

这是这一阶段新增的最关键 API。

### 它和 `VkPhysicalDevice` 的区别

- `VkPhysicalDevice`
  更偏“这块硬件是谁、支持什么”

- `VkDevice`
  更偏“我已经拿到了这块硬件的工作接口”

### 参数解释

- 第一个参数 `physicalDevice`
  从哪块物理设备创建逻辑设备

- 第二个参数 `&deviceCreateInfo`
  设备创建配置

- 第三个参数 `nullptr`
  不用自定义分配器

- 第四个参数 `&logicalDevice`
  输出参数，返回创建好的 `VkDevice`

### 当前代码里它依赖什么

它依赖前面已经做好的工作：

1. 先选中物理设备
2. 先找到图形队列族索引
3. 再通过 `VkDeviceQueueCreateInfo` 申请队列

### 它完成后意味着什么

意味着程序已经不仅仅是“看见 GPU”，而是已经正式获得了操作这块 GPU 的逻辑入口。

---

## 11. `vkGetDeviceQueue` 是做什么的

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:83)

```cpp
vkGetDeviceQueue(logicalDevice, graphicsQueueFamilyIndex, 0, &graphicsQueue);
```

### 作用

从已经创建好的 `VkDevice` 里，取出一条具体的 `VkQueue`。

### 为什么需要它

因为 `vkCreateDevice` 虽然配置了队列资源，但不会直接把某条队列句柄塞给你。

你还要自己去“领出来”。

### 参数解释

- `logicalDevice`
  从哪个逻辑设备里拿

- `graphicsQueueFamilyIndex`
  从哪个队列族里拿

- `0`
  拿这个队列族里的第 0 条队列

- `&graphicsQueue`
  输出参数，把队列句柄写到这里

### 它和 `vkCreateDevice` 的关系

可以这样记：

- `vkCreateDevice`
  申请并配置队列资源

- `vkGetDeviceQueue`
  从 device 里取出具体某条 queue

### 为什么要分两步

因为 queue 属于 device，不是一个独立自由创建的对象。

---

## 12. `vkDestroyDevice` 是做什么的

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:107)

```cpp
vkDestroyDevice(device.logicalDevice, nullptr);
```

### 作用

销毁前面创建的 `VkDevice`。

### 为什么必须销毁

Vulkan 的资源管理很显式：

- 创建了什么
- 通常就要自己销毁什么

所以它和 `vkCreateDevice` 是成对出现的。

### 当前代码里的顺序为什么合理

在 `shutdown()` 里，先销毁所有 `VkDevice`，再销毁 `VkInstance`。

这符合 Vulkan 常见生命周期顺序：

`子对象先销毁，父对象最后销毁`

---

## 13. `vkDestroyInstance` 是做什么的

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:116)

```cpp
vkDestroyInstance(instance_, nullptr);
```

### 作用

销毁实例对象 `VkInstance`。

### 为什么放在最后

因为 `VkInstance` 是这一阶段的顶层 Vulkan 对象。

物理设备、逻辑设备、很多后续对象都依附于它的生命周期。

所以一般会在所有下层对象释放完之后，再销毁它。

---

## 14. 当前阶段出现的 Vulkan 结构体都在做什么

这一阶段虽然真正的 Vulkan 函数不算很多，但结构体非常重要。

---

### `VkApplicationInfo`

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:124)

作用：

`描述你的应用和引擎信息，并声明你打算使用的 Vulkan API 版本`

关键字段：

- `sType`
  结构体类型标识

- `pApplicationName`
  应用名

- `applicationVersion`
  应用版本

- `pEngineName`
  引擎名

- `engineVersion`
  引擎版本

- `apiVersion`
  想使用的 Vulkan API 版本

---

### `VkInstanceCreateInfo`

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:132)

作用：

`打包创建 VkInstance 所需的全部参数`

关键字段：

- `sType`
  结构体类型标识

- `pApplicationInfo`
  应用信息

- `enabledExtensionCount`
  启用的实例扩展数量

- `ppEnabledExtensionNames`
  实例扩展名数组

- `flags`
  实例创建标志

在 macOS 上，这里还加了：

```cpp
VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
```

这是为了让 MoltenVK 的 portability 设备能被正确枚举。

---

### `VkQueueFamilyProperties`

代码里主要被保存到：

```cpp
std::vector<VkQueueFamilyProperties> queueFamilies;
```

作用：

`描述某个队列族的能力`

关键字段：

- `queueCount`
  这个队列族里有多少条队列

- `queueFlags`
  这个队列族支持哪些类型的工作

比如：

- `VK_QUEUE_GRAPHICS_BIT`
- `VK_QUEUE_COMPUTE_BIT`
- `VK_QUEUE_TRANSFER_BIT`

---

### `VkDeviceQueueCreateInfo`

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:63)

作用：

`告诉 Vulkan：创建 VkDevice 时，我想从哪个 queue family 里申请几条 queue`

关键字段：

- `sType`
  结构体类型标识

- `queueFamilyIndex`
  从哪个队列族申请

- `queueCount`
  申请几条队列

- `pQueuePriorities`
  每条队列的优先级数组

当前只申请 1 条图形队列，所以：

- `queueCount = 1`
- `queuePriority = 1.0f`

---

### `VkDeviceCreateInfo`

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:69)

作用：

`打包创建 VkDevice 所需的全部参数`

当前阶段它最主要承载的是：

- 队列创建信息

关键字段：

- `sType`
  结构体类型标识

- `queueCreateInfoCount`
  一共传了多少个队列申请单

- `pQueueCreateInfos`
  队列申请单数组

---

## 15. 当前阶段出现的 Vulkan 宏和标志位是什么意思

---

### `VK_SUCCESS`

表示 Vulkan 函数调用成功。

当前 `check(...)` 函数就是围绕它判断的。

---

### `VK_NULL_HANDLE`

表示空句柄。

用于初始化：

- `VkInstance`
- `VkDevice`
- `VkQueue`
- `VkPhysicalDevice`

你可以把它理解成 Vulkan 世界里的“空对象”。

---

### `VK_API_VERSION_1_0`

表示 Vulkan 1.0 的默认版本值。

在查询运行时版本前，代码先用它作为保底初值。

---

### `VK_QUEUE_GRAPHICS_BIT`

表示：

`这个队列族支持图形工作`

当前阶段它最关键，因为我们就是用它来判断“能不能画图”。

代码判断方式：

```cpp
(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0
```

---

### `VK_QUEUE_COMPUTE_BIT`

表示：

`这个队列族支持计算工作`

当前阶段只是打印出来，还没有真正使用。

---

### `VK_QUEUE_TRANSFER_BIT`

表示：

`这个队列族支持传输工作`

当前阶段也只是打印出来，还没有单独使用。

---

### `VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME`

这是实例扩展名。

在 macOS + MoltenVK 下，通常需要它才能正确枚举 portability 设备。

---

### `VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR`

这是实例创建标志位。

它和前面的 portability 扩展配套使用。

作用可以理解成：

`在枚举物理设备时，把 portability 设备也考虑进去`

---

### `VK_VERSION_MAJOR / MINOR / PATCH`

这些宏在 `Application.cpp` 里用来把打包的 Vulkan 版本号拆开，输出成：

```text
1.4.350
```

它们不是查询函数，而是“解包版本号的工具宏”。

---

## 16. 当前阶段执行顺序逐步解释

这一段最适合你拿着代码对照。

### 第一步：`Application::run()`

代码位置：
[Application.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/Application.cpp:27)

它先创建：

```cpp
vulkan::VulkanContext context;
```

然后调用：

```cpp
context.initialize();
```

这就进入 Vulkan 初始化阶段。

---

### 第二步：`VulkanContext::initialize()`

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:92)

执行顺序是：

1. `shutdown()`
2. `queryRuntimeApiVersion()`
3. `createInstance()`
4. `discoverDevices()`

也就是说：

- 先清理旧状态
- 再查询版本
- 再创建实例
- 再找设备

---

### 第三步：`queryRuntimeApiVersion()`

这里通过：

- `vkGetInstanceProcAddr`
- `vkEnumerateInstanceVersion`

拿到运行时版本。

结果保存进：

```cpp
runtimeApiVersion_
```

---

### 第四步：`createInstance()`

这里准备：

- `VkApplicationInfo`
- `VkInstanceCreateInfo`

然后调用：

```cpp
vkCreateInstance(...)
```

得到一个有效的 `VkInstance`。

---

### 第五步：`discoverDevices()`

这里用：

```cpp
vkEnumeratePhysicalDevices(...)
```

拿到所有物理设备。

---

### 第六步：针对每块设备收集信息

对每个 `VkPhysicalDevice`：

1. `vkGetPhysicalDeviceProperties`
2. `vkGetPhysicalDeviceQueueFamilyProperties`
3. 找图形队列族
4. 如果找到了图形队列族，就 `vkCreateDevice`
5. 然后 `vkGetDeviceQueue`

最终把结果打包进 `DeviceInfo`。

---

### 第七步：`Application` 打印结果

`Application::run()` 再通过：

```cpp
context.runtimeApiVersion()
context.devices()
```

把这些信息打印出来。

也就是说：

- `VulkanContext` 负责准备数据
- `Application` 负责展示数据

这就是当前架构的边界。

---

## 17. 当前阶段为什么还不能画图

虽然第一阶段已经有了：

- `VkInstance`
- `VkPhysicalDevice`
- `VkDevice`
- `VkQueue`

但还没有：

- 窗口
- `Surface`
- `Swapchain`
- `RenderPass`
- `Pipeline`
- `CommandBuffer`

所以现在只是：

`GPU 设备初始化完成`

还不是：

`渲染系统完成`

这也是为什么第一阶段的名字更适合叫“设备初始化阶段”。

---

## 18. 第一阶段学完后，你应该真正掌握什么

如果把这份文档压缩成最核心的学习成果，就是下面这几条：

1. `vkCreateInstance`
   让程序进入 Vulkan 世界

2. `vkEnumeratePhysicalDevices`
   找出系统里有哪些可用 GPU

3. `vkGetPhysicalDeviceQueueFamilyProperties`
   了解每块 GPU 有哪些队列族

4. `vkCreateDevice`
   基于选中的 GPU 创建逻辑设备

5. `vkGetDeviceQueue`
   从逻辑设备里取出真正能提交工作的队列

如果再压缩成一句话：

`第一阶段的本质，就是让程序获得“控制 GPU 干活”的资格。`

---

## 19. 下一阶段会进入什么内容

第一阶段结束后，最自然的下一阶段是：

`把 GPU 结果送到窗口上`

届时会开始出现：

- 窗口库
- `Surface`
- `Swapchain`
- 呈现图像

所以你可以把当前文档当成“第二阶段之前的地基说明书”。

---

## 20. 源码逐函数讲解

这一节专门按当前代码的真实函数顺序，一段一段解释源码在干什么。

对应文件：

- [main.cpp](/Users/chenhongchi/Desktop/Engine/main.cpp:1)
- [src/engine/Application.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/Application.cpp:1)
- [src/engine/vulkan/VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:1)

---

### 20.1 `main()`

代码位置：
[main.cpp](/Users/chenhongchi/Desktop/Engine/main.cpp:6)

```cpp
int main() {
    try {
        engine::Application application;
        application.run();
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "Startup failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
```

#### 这个函数的职责

这里只负责程序最顶层入口。

它做的事情很少：

1. 创建 `Application`
2. 运行 `Application::run()`
3. 如果中间任何地方抛异常，就统一在这里捕获并打印

#### 为什么这里不直接写 Vulkan 代码了

因为在引擎架构里，`main()` 应该尽量薄。

如果把 Vulkan 初始化、设备枚举、窗口创建、主循环都堆在 `main()`，后面很快会失控。

所以这次整理后，`main()` 的职责被压缩成：

`只启动应用，不处理底层细节`

---

### 20.2 `Application::run()`

代码位置：
[Application.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/Application.cpp:27)

```cpp
void Application::run() {
    vulkan::VulkanContext context;
    context.initialize();

    const auto apiVersion = context.runtimeApiVersion();
    std::cout << "Vulkan loader API: " << VK_VERSION_MAJOR(apiVersion) << '.'
              << VK_VERSION_MINOR(apiVersion) << '.'
              << VK_VERSION_PATCH(apiVersion) << '\n';

    const auto& devices = context.devices();
    std::cout << "Detected physical devices: " << devices.size() << '\n';

    for (uint32_t deviceIndex = 0;
         deviceIndex < static_cast<uint32_t>(devices.size());
         ++deviceIndex) {
        const auto& device = devices[deviceIndex];
        std::cout << "[" << deviceIndex << "] " << device.properties.deviceName
                  << '\n';

        for (uint32_t familyIndex = 0;
             familyIndex < static_cast<uint32_t>(device.queueFamilies.size());
             ++familyIndex) {
            printQueueFamily(familyIndex, device.queueFamilies[familyIndex]);
        }

        if (device.graphicsQueueFamilyIndex.has_value()) {
            std::cout << "  First graphics queue family index: "
                      << device.graphicsQueueFamilyIndex.value() << '\n';
            std::cout << "  Logical device created successfully.\n";
            std::cout << "  Graphics queue acquired successfully.\n";
        } else {
            std::cout << "  No graphics queue family found.\n";
        }
    }

    std::cout << "Vulkan setup looks healthy.\n";
}
```

#### 这个函数的职责

它是“应用层编排函数”。

它不创建 Vulkan 对象，但它负责：

1. 让 Vulkan 上下文初始化
2. 把结果整理成用户能看懂的输出

#### 第一行

```cpp
vulkan::VulkanContext context;
```

创建一个 Vulkan 子系统对象。

你可以先把它理解成：

`这一轮程序运行所需的 Vulkan 世界管理器`

#### 第二行

```cpp
context.initialize();
```

这是第一阶段真正的起点。

后面所有 Vulkan 设备初始化工作，都从这里开始。

#### 打印版本号

```cpp
const auto apiVersion = context.runtimeApiVersion();
```

这里从 `VulkanContext` 里取出先前已经查询好的版本号。

然后：

```cpp
VK_VERSION_MAJOR(apiVersion)
VK_VERSION_MINOR(apiVersion)
VK_VERSION_PATCH(apiVersion)
```

把打包整数拆成可读版本字符串。

#### 打印设备数量

```cpp
const auto& devices = context.devices();
std::cout << "Detected physical devices: " << devices.size() << '\n';
```

这里不是重新枚举设备，而是读取 `VulkanContext` 已经准备好的设备列表。

#### 遍历设备

外层 `for` 的作用是：

“逐个打印每块物理设备的信息”

#### 打印设备名字

```cpp
device.properties.deviceName
```

这来自前面 `vkGetPhysicalDeviceProperties(...)` 填入的结构体。

#### 打印队列族

```cpp
printQueueFamily(familyIndex, device.queueFamilies[familyIndex]);
```

这一步只是做输出格式化，真正的队列族查询已经在 `VulkanContext` 里做完了。

#### 打印 graphics queue family index

```cpp
if (device.graphicsQueueFamilyIndex.has_value()) {
```

表示：

“如果这块设备里确实找到了图形队列族，就把它打印出来”

#### 打印逻辑设备和图形队列状态

这里打印：

- `Logical device created successfully.`
- `Graphics queue acquired successfully.`

本质上是在把 `VulkanContext` 的初始化结果翻译成人类能看懂的状态。

#### 最后一行

```cpp
std::cout << "Vulkan setup looks healthy.\n";
```

表示第一阶段整个链路走通了。

---

### 20.3 `printQueueFamily(...)`

代码位置：
[Application.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/Application.cpp:9)

```cpp
void printQueueFamily(uint32_t familyIndex,
                      const VkQueueFamilyProperties& queueFamily) {
    std::cout << "  Queue family " << familyIndex
              << ": queueCount=" << queueFamily.queueCount << ", flags=";

    if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
        std::cout << "[Graphics] ";
    }
    if ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) {
        std::cout << "[Compute] ";
    }
    if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0) {
        std::cout << "[Transfer] ";
    }

    std::cout << '\n';
}
```

#### 这个函数的职责

它不负责查询队列族，只负责：

`把一个 VkQueueFamilyProperties 打印成好读的文本`

#### 为什么用按位与判断

```cpp
(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0
```

因为 `queueFlags` 不是单选值，而是“多个能力位的组合”。

所以不能写成：

```cpp
queueFlags == VK_QUEUE_GRAPHICS_BIT
```

要用位运算判断某一项能力是否存在。

#### 这段代码展示了什么

它把底层的 bitmask 结果翻译成：

```text
[Graphics] [Compute] [Transfer]
```

方便你学习和理解。

---

### 20.4 `check(...)`

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:9)

```cpp
void check(VkResult result, const char* message) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(message);
    }
}
```

#### 这个函数的职责

统一检查 Vulkan API 调用结果。

因为 Vulkan 很多函数都返回 `VkResult`，所以用这个辅助函数可以减少大量重复错误处理。

#### 为什么要抛异常

当前项目还是学习阶段。

这时最重要的是：

- 一旦失败，立刻停下来
- 把错误抛到顶层
- 打印清楚是哪个阶段失败

所以这里用异常处理是合理的。

---

### 20.5 `queryRuntimeApiVersion()`

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:15)

```cpp
uint32_t queryRuntimeApiVersion() {
    uint32_t version = VK_API_VERSION_1_0;

#if VK_HEADER_VERSION >= 70
    const auto enumerateInstanceVersion =
        reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
            vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
    if (enumerateInstanceVersion != nullptr) {
        enumerateInstanceVersion(&version);
    }
#endif

    return version;
}
```

#### 这段函数在做什么

它负责：

`向 Vulkan loader 查询当前运行时支持的 API 版本`

#### 为什么先给默认值 `VK_API_VERSION_1_0`

因为查询函数不一定总能拿到。

所以先给最基础的 Vulkan 1.0 作为保底值。

#### `#if VK_HEADER_VERSION >= 70`

表示：

“只有当前头文件版本足够新，才编译这段查询逻辑”

这是为了兼容性。

#### `vkGetInstanceProcAddr(...)`

这一句是在查询：

“当前运行时有没有 `vkEnumerateInstanceVersion` 这个函数”

#### `reinterpret_cast<PFN_vkEnumerateInstanceVersion>`

这是把通用函数地址转换成“这个函数真正该有的类型”。

#### `enumerateInstanceVersion(&version)`

真正调用查询函数，把结果写进 `version`。

#### 返回值

最后返回一个打包好的 Vulkan 版本整数。

---

### 20.6 `requiredInstanceExtensions()`

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:28)

```cpp
std::vector<const char*> requiredInstanceExtensions() {
    std::vector<const char*> extensions;
#ifdef __APPLE__
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
    return extensions;
}
```

#### 这个函数的职责

统一返回：

`当前平台创建 VkInstance 时必须启用的扩展名列表`

#### 为什么要单独写成函数

因为以后这个列表会继续长。

比如接入窗口后，你还会加入 surface 相关扩展。

所以现在单独封装成函数，比把扩展列表直接写死在 `createInstance()` 里更利于扩展。

#### 当前为什么只在 macOS 下加一个扩展

因为你现在运行在 Apple 平台，通过 MoltenVK 走 portability 路线。

所以需要：

```cpp
VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
```

---

### 20.7 `queryQueueFamilies(...)`

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:35)

```cpp
std::vector<VkQueueFamilyProperties> queryQueueFamilies(
    VkPhysicalDevice physicalDevice) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
                                             &queueFamilyCount,
                                             nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
                                             &queueFamilyCount,
                                             queueFamilies.data());
    return queueFamilies;
}
```

#### 这个函数的职责

给一块物理设备，返回它完整的队列族列表。

#### 两次调用 API 的原因

第一次查数量，第二次查内容。

这和枚举物理设备时的模式一样。

#### 为什么返回 `std::vector<VkQueueFamilyProperties>`

因为当前代码后面既要：

- 查图形队列族
- 也要把完整队列族打印出来

所以直接把整个列表保存起来是最合适的。

---

### 20.8 `findGraphicsQueueFamily(...)`

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:47)

```cpp
std::optional<uint32_t> findGraphicsQueueFamily(
    const std::vector<VkQueueFamilyProperties>& queueFamilies) {
    for (uint32_t familyIndex = 0;
         familyIndex < static_cast<uint32_t>(queueFamilies.size());
         ++familyIndex) {
        if ((queueFamilies[familyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) !=
            0) {
            return familyIndex;
        }
    }

    return std::nullopt;
}
```

#### 这个函数的职责

从一组队列族里找到：

`第一个支持 Graphics 的 queue family index`

#### 为什么返回 `std::optional<uint32_t>`

因为：

- 可能找到
- 也可能找不到

`optional` 比返回特殊数字更清楚。

#### 为什么策略是“找第一个”

因为当前阶段是最小学习程序。

第一版逻辑最简单：

`只要找到一个支持 Graphics 的队列族就够了`

以后更复杂的引擎才会进一步比较：

- present 支持
- 专用 transfer queue
- 专用 compute queue

---

### 20.9 `createLogicalDevice(...)`

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:59)

```cpp
VkDevice createLogicalDevice(VkPhysicalDevice physicalDevice,
                             uint32_t graphicsQueueFamilyIndex) {
    const float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;

    VkDevice logicalDevice = VK_NULL_HANDLE;
    check(vkCreateDevice(physicalDevice,
                         &deviceCreateInfo,
                         nullptr,
                         &logicalDevice),
          "vkCreateDevice failed");
    return logicalDevice;
}
```

#### 这个函数的职责

基于一块物理设备和一个图形队列族索引，创建 `VkDevice`。

#### `queuePriority = 1.0f`

表示这条申请的队列优先级是 1.0。

当前只申请 1 条队列，所以它只是一个必须填的合法参数。

#### `VkDeviceQueueCreateInfo`

这是队列申请单。

这里是在说：

“我想从这个图形队列族里申请 1 条队列”

#### `VkDeviceCreateInfo`

这是逻辑设备创建说明书。

这里把刚才那张队列申请单挂进去。

#### `vkCreateDevice(...)`

真正创建逻辑设备。

成功后意味着：

`这块物理设备的工作台已经建立起来了`

---

### 20.10 `acquireGraphicsQueue(...)`

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:81)

```cpp
VkQueue acquireGraphicsQueue(VkDevice logicalDevice,
                             uint32_t graphicsQueueFamilyIndex) {
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    vkGetDeviceQueue(logicalDevice,
                     graphicsQueueFamilyIndex,
                     0,
                     &graphicsQueue);
    return graphicsQueue;
}
```

#### 这个函数的职责

从已经创建好的 `VkDevice` 里，取出一条真正可用的图形队列。

#### 为什么这里不是创建 queue

因为 queue 不是独立创建对象。

你在 `vkCreateDevice` 时已经声明了需要哪些 queue。

这里是在“领出来”。

#### 参数里的 `0` 是什么

表示取这个队列族里的第 0 条队列。

因为当前我们只申请了 1 条，所以只能拿第 0 条。

---

### 20.11 `VulkanContext::~VulkanContext()`

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:92)

```cpp
VulkanContext::~VulkanContext() {
    shutdown();
}
```

#### 这个析构函数的职责

保证对象离开作用域时自动做资源清理。

这是 C++ RAII 风格的核心体现之一。

即使你忘了手动调用 `shutdown()`，析构也会兜底清理。

---

### 20.12 `VulkanContext::initialize()`

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:96)

```cpp
void VulkanContext::initialize() {
    shutdown();

    runtimeApiVersion_ = queryRuntimeApiVersion();
    createInstance();
    discoverDevices();
}
```

#### 这个函数的职责

这是第一阶段最核心的初始化入口。

#### 第一步 `shutdown()`

先清理旧状态。

这样可以保证即使重复调用 `initialize()`，也不会把旧资源和新资源混在一起。

#### 第二步 `queryRuntimeApiVersion()`

拿到当前运行时支持的 Vulkan 版本。

#### 第三步 `createInstance()`

创建实例入口。

#### 第四步 `discoverDevices()`

扫描设备、队列族、逻辑设备和图形队列。

#### 这四步连起来的意义

这就是第一阶段完整的设备初始化链。

---

### 20.13 `VulkanContext::shutdown()`

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:104)

```cpp
void VulkanContext::shutdown() {
    for (auto& device : devices_) {
        if (device.logicalDevice != VK_NULL_HANDLE) {
            vkDestroyDevice(device.logicalDevice, nullptr);
            device.logicalDevice = VK_NULL_HANDLE;
            device.graphicsQueue = VK_NULL_HANDLE;
        }
    }

    devices_.clear();

    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
}
```

#### 这个函数的职责

按正确顺序释放第一阶段创建的 Vulkan 资源。

#### 为什么先销毁 `VkDevice`

因为逻辑设备依赖于实例存在。

资源清理通常遵循：

`先销毁子对象，再销毁父对象`

#### 为什么 `devices_.clear()`

因为 `devices_` 是缓存的设备信息列表。

底层对象销毁之后，这些缓存也应该清空。

#### 最后为什么才销毁 `VkInstance`

因为它是顶层对象。

---

### 20.14 `VulkanContext::runtimeApiVersion()`

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:120)

```cpp
uint32_t VulkanContext::runtimeApiVersion() const noexcept {
    return runtimeApiVersion_;
}
```

#### 这个函数的职责

只做一件事：

把已经查询好的版本号返回给上层。

它本身不调用 Vulkan API，是一个简单的只读访问接口。

---

### 20.15 `VulkanContext::devices()`

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:124)

```cpp
const std::vector<DeviceInfo>& VulkanContext::devices() const noexcept {
    return devices_;
}
```

#### 这个函数的职责

把已经整理好的设备列表暴露给上层。

这里返回的是常量引用：

```cpp
const std::vector<DeviceInfo>&
```

这样可以避免拷贝整份设备列表。

---

### 20.16 `VulkanContext::createInstance()`

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:128)

```cpp
void VulkanContext::createInstance() {
    const auto extensions = requiredInstanceExtensions();

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "LearningEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = runtimeApiVersion_;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount =
        static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames =
        extensions.empty() ? nullptr : extensions.data();

#ifdef __APPLE__
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    check(vkCreateInstance(&createInfo, nullptr, &instance_),
          "vkCreateInstance failed");
}
```

#### 这个函数的职责

准备好创建实例所需的所有结构体，然后创建 `VkInstance`。

#### 第一步：取扩展列表

```cpp
const auto extensions = requiredInstanceExtensions();
```

让平台相关需求和实例创建逻辑分离。

#### 第二步：构造 `VkApplicationInfo`

这是应用描述信息。

这里告诉 Vulkan：

- 程序名
- 引擎名
- 程序版本
- 引擎版本
- 希望使用的 API 版本

#### 第三步：构造 `VkInstanceCreateInfo`

这是实例创建说明书。

它把：

- 应用信息
- 扩展数量
- 扩展名数组

打包起来。

#### 第四步：在 macOS 下设置 portability flag

这一步是为了 MoltenVK 能正确枚举设备。

#### 第五步：调用 `vkCreateInstance`

真正创建实例。

---

### 20.17 `VulkanContext::discoverDevices()`

代码位置：
[VulkanContext.cpp](/Users/chenhongchi/Desktop/Engine/src/engine/vulkan/VulkanContext.cpp:151)

```cpp
void VulkanContext::discoverDevices() {
    uint32_t deviceCount = 0;
    check(vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr),
          "vkEnumeratePhysicalDevices count failed");

    if (deviceCount == 0) {
        return;
    }

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    check(vkEnumeratePhysicalDevices(instance_,
                                     &deviceCount,
                                     physicalDevices.data()),
          "vkEnumeratePhysicalDevices list failed");

    devices_.reserve(deviceCount);

    for (const auto physicalDevice : physicalDevices) {
        DeviceInfo deviceInfo;
        deviceInfo.physicalDevice = physicalDevice;
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceInfo.properties);

        deviceInfo.queueFamilies = queryQueueFamilies(physicalDevice);
        deviceInfo.graphicsQueueFamilyIndex =
            findGraphicsQueueFamily(deviceInfo.queueFamilies);

        if (deviceInfo.graphicsQueueFamilyIndex.has_value()) {
            deviceInfo.logicalDevice =
                createLogicalDevice(physicalDevice,
                                    deviceInfo.graphicsQueueFamilyIndex.value());
            deviceInfo.graphicsQueue =
                acquireGraphicsQueue(deviceInfo.logicalDevice,
                                     deviceInfo.graphicsQueueFamilyIndex.value());
        }

        devices_.push_back(deviceInfo);
    }
}
```

#### 这个函数的职责

这是第一阶段真正最“实干”的函数。

它把：

- 设备枚举
- 设备属性读取
- 队列族读取
- 图形队列族选择
- 逻辑设备创建
- 图形队列获取

全部串起来了。

#### 第一步：查设备数量

```cpp
vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr)
```

拿到数量。

#### 第二步：拿设备列表

```cpp
std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
vkEnumeratePhysicalDevices(..., physicalDevices.data())
```

真正拿到物理设备数组。

#### 第三步：`devices_.reserve(deviceCount)`

提前给 `devices_` 预留容量。

表示：

“我已经知道接下来大概要存多少个 `DeviceInfo` 了，先把空间留出来”

#### 第四步：遍历每块设备

对每块物理设备做完整信息整理。

#### 第五步：读设备属性

```cpp
vkGetPhysicalDeviceProperties(...)
```

拿设备名等基础信息。

#### 第六步：读队列族

```cpp
deviceInfo.queueFamilies = queryQueueFamilies(physicalDevice);
```

拿完整队列族列表。

#### 第七步：找图形队列族

```cpp
deviceInfo.graphicsQueueFamilyIndex =
    findGraphicsQueueFamily(deviceInfo.queueFamilies);
```

这一步是从“看设备”进入“准备开工”的关键步骤。

#### 第八步：如果找到了图形队列族，就创建设备和队列

```cpp
deviceInfo.logicalDevice = createLogicalDevice(...)
deviceInfo.graphicsQueue = acquireGraphicsQueue(...)
```

这一步标志着：

`这块设备已经从“被发现”进入“可工作”状态`

#### 第九步：`devices_.push_back(deviceInfo)`

把整理好的完整设备信息放进设备列表缓存。

这样上层就可以统一读取和展示了。

---

## 21. 读源码时最该关注的调用链

如果你现在想回头自己读代码，最推荐按这个顺序走：

1. `main()`
2. `Application::run()`
3. `VulkanContext::initialize()`
4. `queryRuntimeApiVersion()`
5. `createInstance()`
6. `discoverDevices()`
7. `queryQueueFamilies()`
8. `findGraphicsQueueFamily()`
9. `createLogicalDevice()`
10. `acquireGraphicsQueue()`
11. `shutdown()`

因为这条链正好就是当前阶段完整生命周期。

---

## 22. 这一节源码讲解的最终结论

如果你把逐函数讲解全部消化掉，说明你已经真正理解了第一阶段的代码，不只是“知道输出是什么”，而是知道：

- 哪些函数负责查询
- 哪些函数负责创建
- 哪些函数负责选择
- 哪些函数负责销毁
- `Application` 和 `VulkanContext` 为什么要分层

这时第一阶段就算真的学完了。

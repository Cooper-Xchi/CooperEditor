# Vulkan 最小示例逐行讲解

本文档专门解释当前仓库这份最小 Vulkan 示例代码，目标不是“快速抄完”，而是让你真正理解：

- 这份代码在做什么
- 每个 Vulkan API 的作用是什么
- 每个结构体为什么要这样填
- 这段代码运行时，Vulkan 内部大致发生了什么

对应源文件：

- [main.cpp](/Users/chenhongchi/Desktop/Engine/main.cpp:1)

当前代码的目标非常单纯，只做三件事：

1. 查询 Vulkan 运行时支持的 API 版本
2. 创建一个 `VkInstance`
3. 枚举当前机器上的物理设备，并打印设备名字

这还不是完整渲染器，甚至还没有窗口、逻辑设备、交换链、渲染管线。  
它只是 Vulkan 学习的第一个正确落点。

---

## 1. 先看完整流程

这份代码的执行顺序可以先概括成：

```text
main
  -> 查询 Vulkan API 版本
  -> 创建 VkInstance
  -> 枚举 VkPhysicalDevice
  -> 读取每个设备的属性
  -> 打印设备名称
  -> 销毁 VkInstance
```

如果你以后把 Vulkan 学下去，会发现几乎所有内容都是在这个基础上往后扩展：

```text
VkInstance
  -> VkPhysicalDevice
    -> VkDevice
      -> Queue / Surface / Swapchain / Pipeline / Command Buffer ...
```

所以这份代码的重要性在于：它把最底层入口打通了。

---

## 2. 代码整体结构

当前 [main.cpp](/Users/chenhongchi/Desktop/Engine/main.cpp:1) 大致分成四部分：

1. 头文件
2. 工具函数 `check`
3. Vulkan 初始化相关函数
4. `main`

源码如下：

```cpp
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <vulkan/vulkan.h>

namespace {

void check(VkResult result, const char* message) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(message);
    }
}

uint32_t runtimeApiVersion() {
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

VkInstance createInstance() {
    const auto apiVersion = runtimeApiVersion();

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "LearningEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = apiVersion;

    std::vector<const char*> extensions;
#ifdef __APPLE__
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

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

    VkInstance instance = VK_NULL_HANDLE;
    check(vkCreateInstance(&createInfo, nullptr, &instance),
          "vkCreateInstance failed");
    return instance;
}

void printDevices(VkInstance instance) {
    uint32_t deviceCount = 0;
    check(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr),
          "vkEnumeratePhysicalDevices count failed");

    std::cout << "Detected physical devices: " << deviceCount << '\n';

    if (deviceCount == 0) {
        std::cout << "No Vulkan-compatible GPU was found.\n";
        return;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    check(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()),
          "vkEnumeratePhysicalDevices list failed");

    for (uint32_t i = 0; i < deviceCount; ++i) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(devices[i], &properties);

        std::cout << "[" << i << "] " << properties.deviceName << '\n';
    }
}

}  // namespace

int main() {
    try {
        const auto apiVersion = runtimeApiVersion();
        std::cout << "Vulkan loader API: "
                  << VK_VERSION_MAJOR(apiVersion) << '.'
                  << VK_VERSION_MINOR(apiVersion) << '.'
                  << VK_VERSION_PATCH(apiVersion) << '\n';

        const VkInstance instance = createInstance();
        printDevices(instance);
        vkDestroyInstance(instance, nullptr);

        std::cout << "Vulkan setup looks healthy.\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "Startup failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
```

下面我们按概念和调用顺序展开。

---

## 3. 头文件分别是干什么的

```cpp
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <vulkan/vulkan.h>
```

### `#include <cstdlib>`

提供 `EXIT_SUCCESS` 和 `EXIT_FAILURE`。

这里的作用是让 `main()` 可以返回：

```cpp
return EXIT_SUCCESS;
return EXIT_FAILURE;
```

它们本质上就是更语义化的返回码。

### `#include <iostream>`

提供：

- `std::cout`
- `std::cerr`

用于在终端打印信息。

### `#include <stdexcept>`

提供：

- `std::runtime_error`

这里用它来在 Vulkan API 调用失败时抛异常。

### `#include <vector>`

提供：

- `std::vector`

这里用它来存放扩展名称列表，以及存放 `VkPhysicalDevice` 数组。

### `#include <vulkan/vulkan.h>`

这是 Vulkan 开发最核心的头文件。

它定义了你现在看到的几乎所有 Vulkan 内容：

- `VkInstance`
- `VkPhysicalDevice`
- `VkResult`
- `VkApplicationInfo`
- `VkInstanceCreateInfo`
- `vkCreateInstance`
- `vkEnumeratePhysicalDevices`
- `vkGetPhysicalDeviceProperties`
- `vkDestroyInstance`

如果没有这个头文件，当前代码里所有 `Vk...` 和 `vk...` 名字几乎都不存在。

---

## 4. `namespace {}` 是什么

```cpp
namespace {
    ...
}
```

这是一个匿名命名空间。

你可以先把它理解成：

“里面这些函数只给当前这个 `.cpp` 文件自己用，不想暴露给别的文件。”

当前放进去的函数有：

- `check`
- `runtimeApiVersion`
- `createInstance`
- `printDevices`

它们都是当前文件的内部工具函数。

---

## 5. `check` 函数是做什么的

代码：

```cpp
void check(VkResult result, const char* message) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(message);
    }
}
```

### 作用

这个函数的目的是统一检查 Vulkan API 是否调用成功。

很多 Vulkan API 返回值类型都是：

```cpp
VkResult
```

`VkResult` 可以先理解成：

“Vulkan 函数的状态码。”

最常见的成功值是：

```cpp
VK_SUCCESS
```

所以 `check` 的逻辑非常直接：

- 如果返回值是 `VK_SUCCESS`，说明成功，什么都不做
- 如果不是 `VK_SUCCESS`，说明失败，直接抛异常

### 为什么要这样写

Vulkan 很底层，函数很多，错误处理如果每次都手写会很重复。

例如原本你可能要写成：

```cpp
VkResult result = vkCreateInstance(...);
if (result != VK_SUCCESS) {
    throw std::runtime_error("vkCreateInstance failed");
}
```

现在可以压缩成：

```cpp
check(vkCreateInstance(...), "vkCreateInstance failed");
```

这样更短，也更统一。

### `const char* message` 是什么

它是一个 C 风格字符串。

比如这里传进来的可能是：

```cpp
"vkCreateInstance failed"
```

当失败时，用它来构造异常对象。

---

## 6. `runtimeApiVersion()` 是做什么的

代码：

```cpp
uint32_t runtimeApiVersion() {
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

### 作用

这个函数用来查询：

“当前 Vulkan 运行时支持的实例 API 版本是多少。”

换句话说，它是在问系统：

“你这套 Vulkan loader 对外支持哪一代 Vulkan API？”

### 返回值为什么是 `uint32_t`

Vulkan 版本号不是以字符串 `"1.4.350"` 存储的，而是打包进一个 32 位整数里。

所以这里返回：

```cpp
uint32_t
```

也就是一个无符号 32 位整数。

### `uint32_t version = VK_API_VERSION_1_0;`

先把版本默认设成 Vulkan 1.0。

这是一个保底值。

意思是：

- 如果后面没有成功查到更高版本
- 那至少我们先按 Vulkan 1.0 来看待

### `#if VK_HEADER_VERSION >= 70`

这是一个预处理判断。

意思是：

“只有当前 Vulkan 头文件版本足够新时，才编译下面那段逻辑。”

原因是 `vkEnumerateInstanceVersion` 不是特别早期的 Vulkan 头文件里就有的。

### `vkGetInstanceProcAddr`

这行最关键：

```cpp
vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion")
```

这是一个 Vulkan API。

#### 作用

它的作用是：

“按函数名去 Vulkan 运行时里查询函数入口地址。”

你可以先把它理解成：

“向 Vulkan 要一个函数指针。”

#### 为什么要这样做

在 Vulkan 里，不是所有函数都直接天然可用。

有些函数是：

- 新版本引入的
- 扩展提供的
- 或者需要在运行时动态查询

所以 Vulkan 提供了 `vkGetInstanceProcAddr`，让你自己按名字去找函数。

#### 参数解释

```cpp
vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion")
```

- 第一个参数：`nullptr`
  这里表示这次查询不依赖某个具体实例对象

- 第二个参数：`"vkEnumerateInstanceVersion"`
  表示要查询的函数名字

#### 返回什么

返回的是一个通用函数地址。

所以不能直接拿来调用，需要转换成正确的函数指针类型。

### `PFN_vkEnumerateInstanceVersion`

```cpp
reinterpret_cast<PFN_vkEnumerateInstanceVersion>(...)
```

`PFN_vkEnumerateInstanceVersion` 是 Vulkan 为这个函数定义好的函数指针类型。

你可以把它理解成：

“`vkEnumerateInstanceVersion` 这种函数地址，应该长成什么类型。”

这里做的事情是：

1. 用 `vkGetInstanceProcAddr` 拿到一个通用地址
2. 强制转换成 `PFN_vkEnumerateInstanceVersion`
3. 存进 `enumerateInstanceVersion`

### `if (enumerateInstanceVersion != nullptr)`

表示：

“只有当这个函数在当前运行时里真的存在，才调用它。”

这样做是为了兼容性。

### `enumerateInstanceVersion(&version);`

这是真正调用函数。

#### 作用

把当前 Vulkan 运行时支持的版本写进 `version`。

#### 为什么传 `&version`

因为这个 API 需要你提供一个输出地址，让它把结果填进去。

这是 C 风格 API 很常见的设计。

---

## 7. `VkApplicationInfo` 是什么

代码片段：

```cpp
VkApplicationInfo appInfo{};
appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
appInfo.pApplicationName = "Engine";
appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
appInfo.pEngineName = "LearningEngine";
appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
appInfo.apiVersion = apiVersion;
```

### 它不是 API 函数，而是结构体

`VkApplicationInfo` 是一个 Vulkan 结构体。

它的作用是：

“描述你的程序和引擎的一些基础信息，并声明你希望使用哪个 Vulkan API 版本。”

### `VkApplicationInfo appInfo{};`

这里创建了一个结构体变量，并用 `{}` 做零初始化。

这在 Vulkan 里是个非常好的习惯。

因为 Vulkan 结构体字段很多，如果你不初始化，可能会留下垃圾值。

### `appInfo.sType`

```cpp
appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
```

这是 Vulkan 结构体最常见的字段之一。

#### 作用

告诉 Vulkan：

“我现在传给你的这个结构体，类型是 `VkApplicationInfo`。”

#### 为什么 Vulkan 要这样设计

因为 Vulkan 有很多结构体，而且很多结构体可以通过指针串起来传递。

`sType` 就像每个结构体自带的“身份证类型标识”。

你几乎可以把它当成固定动作：

- 写 `VkXxxInfo`
- 就要把 `sType` 设成对应的 `VK_STRUCTURE_TYPE_XXX`

### `pApplicationName`

```cpp
appInfo.pApplicationName = "Engine";
```

表示程序名。

它主要是元信息，不直接决定渲染效果。

### `applicationVersion`

```cpp
appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
```

表示你自己程序的版本。

注意这不是 Vulkan 版本，而是你的应用版本。

`VK_MAKE_VERSION(1, 0, 0)` 用来把版本打包成 Vulkan 习惯使用的整数格式。

### `pEngineName`

```cpp
appInfo.pEngineName = "LearningEngine";
```

表示你使用的引擎名。

这里虽然当前项目还谈不上真正引擎，但先填一个名字没问题。

### `engineVersion`

```cpp
appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
```

表示引擎版本。

同样是你自己的版本信息，不是 Vulkan 版本。

### `apiVersion`

```cpp
appInfo.apiVersion = apiVersion;
```

这是 `VkApplicationInfo` 里最重要的字段之一。

它告诉 Vulkan：

“我这份程序打算使用哪个 Vulkan API 版本。”

这里直接使用前面查出来的运行时版本。

这样写在当前学习项目里最简单直接。

---

## 8. `std::vector<const char*> extensions` 是做什么的

代码：

```cpp
std::vector<const char*> extensions;
#ifdef __APPLE__
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
```

### 作用

用来存放“要启用的实例扩展名称”。

Vulkan 的很多能力不是默认全开的，而是要你显式声明。

### 什么是扩展

你可以先把扩展理解成：

“核心 Vulkan 规范之外的附加能力。”

有些扩展后来会被吸收进更高版本核心 API，有些则保持扩展形式。

### 为什么这里用字符串列表

因为 Vulkan 启用扩展时，通常是通过“扩展名字符串数组”来传给创建信息结构体。

所以这里需要：

```cpp
std::vector<const char*>
```

每个元素都是一个扩展名。

### 为什么 macOS 需要 `VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME`

因为你当前在 macOS 上，底层依赖 MoltenVK。

MoltenVK 不是传统意义上的原生 Vulkan 驱动，而是把 Vulkan 转成 Metal。

在这种路径下，为了让设备枚举正常工作，需要启用 portability 相关支持。

如果少了它，常见情况是：

- `VkInstance` 创建成功
- 但是枚举不到设备

---

## 9. `VkInstanceCreateInfo` 是什么

代码片段：

```cpp
VkInstanceCreateInfo createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
createInfo.pApplicationInfo = &appInfo;
createInfo.enabledExtensionCount =
    static_cast<uint32_t>(extensions.size());
createInfo.ppEnabledExtensionNames =
    extensions.empty() ? nullptr : extensions.data();
```

### 作用

`VkInstanceCreateInfo` 是创建 `VkInstance` 时最核心的配置结构体。

你可以把它理解成：

“`vkCreateInstance` 的参数说明书。”

### `VkInstanceCreateInfo createInfo{};`

同样先零初始化。

### `createInfo.sType`

```cpp
createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
```

和前面的 `VkApplicationInfo` 一样，告诉 Vulkan：

“这是一个 `VkInstanceCreateInfo` 结构体。”

### `createInfo.pApplicationInfo = &appInfo;`

这里把前面准备好的 `VkApplicationInfo` 挂进来。

也就是说：

“创建实例时，同时附带应用信息。”

### `enabledExtensionCount`

```cpp
createInfo.enabledExtensionCount =
    static_cast<uint32_t>(extensions.size());
```

表示你启用了多少个实例扩展。

这里为什么要 `static_cast<uint32_t>`：

因为 `extensions.size()` 返回的是 C++ 的容器大小类型，而 Vulkan 这里需要 `uint32_t`。

### `ppEnabledExtensionNames`

```cpp
createInfo.ppEnabledExtensionNames =
    extensions.empty() ? nullptr : extensions.data();
```

表示扩展名数组的首地址。

这个名字看起来怪，是因为 Vulkan/C 风格命名里：

- `p` 表示 pointer
- `pp` 表示 pointer to pointer

这里本质上是：

“一个字符串指针数组的首地址”

因为每个扩展名本身就是 `const char*`，所以整个数组的地址就是 `const char**`。

### `extensions.empty() ? nullptr : extensions.data()`

这句的意思是：

- 如果一个扩展都没有，就传 `nullptr`
- 如果有扩展，就把数组首地址传进去

这样写更严谨。

---

## 10. `createInfo.flags` 在 macOS 下做了什么

代码：

```cpp
#ifdef __APPLE__
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
```

### 作用

这是 macOS 下的附加实例创建标记。

它和前面的 portability 扩展是配套的。

你可以先理解成：

“告诉 Vulkan：枚举物理设备时，把 portability 设备也考虑进去。”

在 Apple 平台配合 MoltenVK 时，这通常是必须的。

如果没有这项，即使扩展名加了，设备枚举依然可能不符合预期。

### `|=` 是什么意思

表示“把某个标志位加进去”。

Vulkan 里很多 `flags` 字段都不是单一值，而是多个二进制标志位的组合。

---

## 11. `vkCreateInstance` 是什么，怎么工作

代码：

```cpp
VkInstance instance = VK_NULL_HANDLE;
check(vkCreateInstance(&createInfo, nullptr, &instance),
      "vkCreateInstance failed");
return instance;
```

### 这是当前代码最关键的 Vulkan API

它的作用是：

“根据你提供的创建信息，创建一个 Vulkan 实例对象。”

### `VkInstance` 是什么

可以先理解成：

“程序进入 Vulkan 世界后的总入口对象。”

没有它，你后面基本没法继续做：

- 枚举物理设备
- 创建 surface
- 启用实例级扩展

### `VK_NULL_HANDLE`

```cpp
VkInstance instance = VK_NULL_HANDLE;
```

它相当于 Vulkan 世界里的“空对象句柄”。

可以把它类比成：

- 空指针
- 尚未创建的句柄

### `vkCreateInstance` 参数解释

```cpp
vkCreateInstance(&createInfo, nullptr, &instance)
```

#### 第一个参数：`&createInfo`

把实例创建配置传给 Vulkan。

#### 第二个参数：`nullptr`

表示不使用自定义内存分配器。

Vulkan 允许你自己接管某些内存分配行为，但初学阶段完全不用碰。

#### 第三个参数：`&instance`

输出参数。

表示：

“如果创建成功，请把创建好的 `VkInstance` 写到这个变量里。”

### 它内部大致做了什么

从概念上可以这样理解：

1. Vulkan loader 收到创建请求
2. 检查你传的结构体和扩展是否合理
3. 结合系统中的 Vulkan 实现初始化实例级状态
4. 返回一个可以继续使用的 `VkInstance`

---

## 12. `printDevices(instance)` 在做什么

这个函数的目标是：

“用已经创建好的 `VkInstance` 去查看当前机器上有哪些 Vulkan 物理设备。”

代码：

```cpp
void printDevices(VkInstance instance) {
    uint32_t deviceCount = 0;
    check(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr),
          "vkEnumeratePhysicalDevices count failed");

    std::cout << "Detected physical devices: " << deviceCount << '\n';

    if (deviceCount == 0) {
        std::cout << "No Vulkan-compatible GPU was found.\n";
        return;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    check(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()),
          "vkEnumeratePhysicalDevices list failed");

    for (uint32_t i = 0; i < deviceCount; ++i) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(devices[i], &properties);

        std::cout << "[" << i << "] " << properties.deviceName << '\n';
    }
}
```

---

## 13. `vkEnumeratePhysicalDevices` 是什么，为什么调用两次

### 作用

这个 API 的作用是：

“枚举当前 `VkInstance` 可见的物理设备列表。”

物理设备通常就是 GPU。

### 第一次调用

```cpp
uint32_t deviceCount = 0;
check(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr),
      "vkEnumeratePhysicalDevices count failed");
```

这次调用的重点是：

- 第三个参数传 `nullptr`

这表示：

“这次先不要真正给我设备数组，只告诉我有多少个设备。”

调用完成后，`deviceCount` 会被写成设备数量。

### 为什么要先查数量

因为你还不知道应该分配多大的数组。

Vulkan 和很多 C 风格 API 都喜欢这种模式：

1. 第一次调用，查询数量
2. 根据数量分配空间
3. 第二次调用，读取实际数据

### 第二次调用

```cpp
std::vector<VkPhysicalDevice> devices(deviceCount);
check(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()),
      "vkEnumeratePhysicalDevices list failed");
```

这里先创建一个长度为 `deviceCount` 的数组。

然后第二次调用 API，把设备对象真正写入这个数组。

### 这两个参数分别是干什么的

```cpp
vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data())
```

- 第一个参数：`instance`
  表示从哪个 Vulkan 实例里枚举设备

- 第二个参数：`&deviceCount`
  输入输出参数
  一方面告诉 API 你准备了多大数组，另一方面也会返回实际写入的数量

- 第三个参数：`devices.data()`
  表示设备数组的首地址

---

## 14. `VkPhysicalDevice` 是什么

`VkPhysicalDevice` 表示：

“一块实际存在的物理设备。”

对大多数桌面系统来说，你可以先把它理解成 GPU。

比如：

- 独立显卡
- 集成显卡
- Apple 芯片里的 GPU

在你的机器上，当前看到的是：

```text
Apple M5 Pro
```

这意味着：

- Vulkan loader 工作正常
- MoltenVK 工作正常
- 当前实例已经能看到 Metal 后端对应的设备

---

## 15. `vkGetPhysicalDeviceProperties` 是什么

代码：

```cpp
VkPhysicalDeviceProperties properties{};
vkGetPhysicalDeviceProperties(devices[i], &properties);
```

### 作用

读取某个物理设备的基础属性。

这些属性通常包括：

- 设备名字
- API 版本
- 驱动版本
- 设备类型
- 厂商 ID
- 一些硬件能力上限

### 参数解释

```cpp
vkGetPhysicalDeviceProperties(devices[i], &properties);
```

- 第一个参数：`devices[i]`
  表示要查询哪一块物理设备

- 第二个参数：`&properties`
  输出结构体地址，Vulkan 会把查询结果写进去

### 为什么这里不返回 `VkResult`

不是所有 Vulkan API 都返回 `VkResult`。

像 `vkGetPhysicalDeviceProperties` 这种读取设备静态信息的函数，直接通过输出结构体返回结果，不单独返回状态码。

### `properties.deviceName`

```cpp
std::cout << "[" << i << "] " << properties.deviceName << '\n';
```

这里拿到的是设备名称字符串。

所以程序最终能打印出：

```text
[0] Apple M5 Pro
```

---

## 16. `main()` 是怎么组织这些 API 的

代码：

```cpp
int main() {
    try {
        const auto apiVersion = runtimeApiVersion();
        std::cout << "Vulkan loader API: "
                  << VK_VERSION_MAJOR(apiVersion) << '.'
                  << VK_VERSION_MINOR(apiVersion) << '.'
                  << VK_VERSION_PATCH(apiVersion) << '\n';

        const VkInstance instance = createInstance();
        printDevices(instance);
        vkDestroyInstance(instance, nullptr);

        std::cout << "Vulkan setup looks healthy.\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "Startup failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
```

---

## 17. `VK_VERSION_MAJOR/MINOR/PATCH` 在做什么

代码：

```cpp
std::cout << "Vulkan loader API: "
          << VK_VERSION_MAJOR(apiVersion) << '.'
          << VK_VERSION_MINOR(apiVersion) << '.'
          << VK_VERSION_PATCH(apiVersion) << '\n';
```

### 作用

把前面拿到的整数版本号拆成：

- 主版本
- 次版本
- 补丁版本

然后按 `1.4.350` 这种人能看懂的形式打印出来。

### 为什么需要这些宏

因为 `apiVersion` 不是字符串，而是一个被编码过的整数。

所以 Vulkan 提供宏来拆解它。

### 这不是 GPU 版本

这里打印的是：

“当前 Vulkan loader 支持的 API 版本”

不是：

- 你的程序版本
- GPU 型号
- macOS 版本

---

## 18. `vkDestroyInstance` 是什么

代码：

```cpp
vkDestroyInstance(instance, nullptr);
```

### 作用

销毁前面创建的 `VkInstance`。

### 为什么必须销毁

Vulkan 很强调显式资源管理。

基本规律是：

- 你创建的对象，通常要自己销毁
- 它不会像某些高级框架一样全帮你自动回收

所以这里有一组非常重要的配对关系：

- `vkCreateInstance`
- `vkDestroyInstance`

以后你还会见到很多类似配对：

- `vkCreateDevice` / `vkDestroyDevice`
- `vkCreateBuffer` / `vkDestroyBuffer`

### 第二个参数为什么还是 `nullptr`

和创建实例时一样，这里仍然表示：

“不使用自定义内存分配器。”

---

## 19. `try/catch` 在这里的作用

代码：

```cpp
try {
    ...
} catch (const std::exception& error) {
    std::cerr << "Startup failed: " << error.what() << '\n';
    return EXIT_FAILURE;
}
```

### 作用

统一处理前面 `check(...)` 抛出的异常。

如果某个 Vulkan 步骤失败了，例如：

- `vkCreateInstance` 失败
- `vkEnumeratePhysicalDevices` 失败

那么异常会一路传到这里，被捕获，然后打印错误信息。

这让当前示例在学习阶段更清晰：

- 成功时打印正常信息
- 失败时打印明确错误

---

## 20. 这份代码运行时 Vulkan 内部大致发生了什么

从“概念上的内部流程”看，可以理解成这样：

### 阶段 1：查询版本

程序先问 Vulkan loader：

```text
你支持哪一代 Vulkan API？
```

loader 返回类似：

```text
1.4.350
```

### 阶段 2：创建实例

程序把自己的应用信息和扩展需求交给 Vulkan：

```text
程序名是什么？
引擎名是什么？
要用哪个 API 版本？
要不要启用某些实例扩展？
```

然后 Vulkan 创建一个 `VkInstance`。

### 阶段 3：枚举物理设备

程序再通过这个实例去问：

```text
当前系统里有哪些 Vulkan 可见设备？
```

Vulkan 返回设备列表。

### 阶段 4：读取设备属性

程序再逐个读取设备的属性信息，并把设备名打印出来。

### 阶段 5：销毁实例

程序结束前，把创建的实例释放掉。

---

## 21. 当前代码已经做到什么，还没做到什么

### 已经做到

- Vulkan SDK 已经能被项目找到
- 程序已经能调用 Vulkan API
- 程序已经能创建 `VkInstance`
- 程序已经能枚举 `VkPhysicalDevice`
- 程序已经能读取并打印设备名

### 还没做到

- 选择合适的物理设备
- 查询队列族
- 创建 `VkDevice`
- 获取图形队列
- 创建窗口
- 创建 surface
- 创建 swapchain
- 真正绘制图像

所以当前代码的位置非常准确：

它还处在“真正开始用 GPU 之前”的准备阶段。

---

## 22. 当前你应该记住的最重要结论

如果把整份代码压缩成 5 句话：

1. `vkGetInstanceProcAddr`：按名字查询 Vulkan 函数入口
2. `vkCreateInstance`：创建 Vulkan 总入口对象
3. `vkEnumeratePhysicalDevices`：查询有哪些物理设备
4. `vkGetPhysicalDeviceProperties`：读取设备属性
5. `vkDestroyInstance`：释放实例资源

再进一步压缩成一句话：

`这份代码的本质是：进入 Vulkan 世界，看看这台机器上有哪些可用 GPU，然后干净退出。`

---

## 23. 接下来最适合学习的主题

在完全理解这份代码之后，下一步最合理的内容不是窗口，也不是渲染，而是：

1. 什么是 `VkDevice`
2. 为什么找到 `VkPhysicalDevice` 以后还不能直接画图
3. 什么是队列族 `Queue Family`
4. 为什么创建逻辑设备时要先选队列

因为 Vulkan 的下一层核心关系就是：

```text
VkInstance
  -> VkPhysicalDevice
    -> Queue Family
      -> VkDevice
```

如果这个链条稳了，后面的窗口和渲染才不会变成死记硬背。

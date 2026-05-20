# macOS Vulkan 环境配置与检查

本文档记录这次在 Mac 上为本仓库配置 Vulkan 开发环境的完整过程，适合刚接触 Vulkan 的新手直接照着做。

当前机器环境：

- 系统：`macOS 26.4.1`
- 架构：`Apple Silicon / arm64`
- 编译器：`Apple clang 21.0.0`
- CMake：`4.3.1`
- Vulkan SDK 版本：`1.4.350.0`

## 1. 先理解你装的是什么

macOS 没有原生 Vulkan 驱动。

在 Mac 上学习 Vulkan，通常依赖这条链路：

1. 你的 C++ 程序调用 Vulkan API
2. Vulkan SDK 提供头文件、loader、工具和验证层
3. MoltenVK 把 Vulkan 调用转换成 Apple 的 Metal

所以这次安装的核心不是单独一个库，而是一整套 `Vulkan SDK + MoltenVK` 环境。

## 2. 配置前检查

先确认编译环境是好的。

执行：

```bash
xcode-select -p
clang++ --version
cmake --version
```

本次机器的正常结果是：

- `xcode-select -p` 输出 `/Library/Developer/CommandLineTools`
- `clang++ --version` 能看到 Apple clang 版本
- `cmake --version` 能正常输出版本号

如果 `xcode-select -p` 失败，先安装命令行工具：

```bash
xcode-select --install
```

如果 `cmake` 不存在：

```bash
brew install cmake
```

## 3. 下载官方 SDK

下载地址：

```text
https://vulkan.lunarg.com/sdk/home
```

在 macOS 页面，下载最新版的 `SDK - SDK Installer`。

这次使用的文件是：

```text
vulkansdk-macos-1.4.350.0.zip
```

不要下载 `config.json`，那个不是给手动安装用的。

## 4. 图形界面安装

如果你已经像这次一样双击运行了安装器，并看到 `Vulkan SDK 安装程序` 正在下载组件，那么你的下载选择就是正确的。

图形界面安装完成后，真正可用的 macOS SDK 内容通常会出现在：

```bash
~/VulkanSDK/1.4.350.0/macOS
```

安装完成后先检查目录：

```bash
ls ~/VulkanSDK
ls ~/VulkanSDK/1.4.350.0
ls ~/VulkanSDK/1.4.350.0/macOS
```

这次机器上的实际结构是：

- `~/VulkanSDK/1.4.350.0` 下面有 `setup-env.sh`
- `~/VulkanSDK/1.4.350.0/macOS` 下面有真正的 macOS 头文件、库和工具

正常情况下，你会看到类似这些目录：

- `bin`
- `include`
- `lib`
- `share`

注意：这次实际安装中，`setup-env.sh` 位于：

```bash
~/VulkanSDK/1.4.350.0/setup-env.sh
```

不在 `macOS` 子目录里。

## 5. 终端安装命令

如果以后你想完全用终端安装，也可以。

先下载并解压：

```bash
cd ~/Downloads
unzip vulkansdk-macos-1.4.350.0.zip
```

然后执行安装：

```bash
cd ~/Downloads
./vulkansdk-macOS-1.4.350.0.app/Contents/MacOS/vulkansdk-macOS-1.4.350.0 \
  --root ~/VulkanSDK/1.4.350.0 \
  --accept-licenses \
  --default-answer \
  --confirm-command install \
  copy_only=1
```

`copy_only=1` 代表只安装到用户目录，适合学习阶段，风险更低。

## 6. 临时加载环境

安装完成后，先在当前终端临时加载 SDK 环境：

```bash
cd ~/VulkanSDK/1.4.350.0
source setup-env.sh
```

这一步只对当前终端窗口生效。

本次机器执行脚本时，实际提示为：

```text
This script is now using VK_ADD_LAYER_PATH instead of VK_LAYER_PATH
```

这是正常提示，不是报错。

然后立刻检查：

```bash
echo $VULKAN_SDK
which vulkaninfo
vulkaninfo | head
```

### 这几条命令分别在检查什么

`echo $VULKAN_SDK`

- 检查 SDK 根路径是否已经进入环境变量
- 正常应输出：

```bash
/Users/chenhongchi/VulkanSDK/1.4.350.0/macOS
```

`which vulkaninfo`

- 检查命令行工具是否能找到 Vulkan SDK 自带工具
- 正常应输出一个可执行文件路径

`vulkaninfo | head`

- 检查 Vulkan loader、MoltenVK 和基础运行时是否真正可工作
- 如果这一步能输出 Vulkan 信息，说明 SDK 基本装通了

## 7. 永久写入环境变量

如果你希望以后每次打开终端都能直接使用 Vulkan，把这些内容写入 `~/.zshrc`。

先打开：

```bash
nano ~/.zshrc
```

追加以下内容：

```bash
export VULKAN_SDK="$HOME/VulkanSDK/1.4.350.0/macOS"
export PATH="$VULKAN_SDK/bin:$PATH"
export DYLD_LIBRARY_PATH="$VULKAN_SDK/lib:$DYLD_LIBRARY_PATH"
export VK_ICD_FILENAMES="$VULKAN_SDK/share/vulkan/icd.d/MoltenVK_icd.json"
export VK_ADD_LAYER_PATH="$VULKAN_SDK/share/vulkan/explicit_layer.d"
```

保存后执行：

```bash
source ~/.zshrc
```

再次检查：

```bash
echo $VULKAN_SDK
which vulkaninfo
vulkaninfo | head
```

如果这些命令都正常，说明环境变量已经永久生效。

如果你看到类似下面的 warning：

- `Removing layer ... because it is a duplicate`
- `Path to given binary ... differ from OS loaded path ...`

通常表示系统目录和 SDK 目录里同时存在同名 Vulkan layer。只要 `vulkaninfo` 仍然能正常输出 Vulkan 版本和扩展信息，这类 warning 一般不是致命问题。

## 8. 更进一步的检查命令

想确认 GPU、MoltenVK、API 版本时，可以执行：

```bash
vulkaninfo | rg "deviceName|apiVersion|driverName|MoltenVK"
```

你应该能看到：

- GPU 名字
- Vulkan API 版本
- `MoltenVK`

如果这里有输出，通常说明图形驱动链路已经通了。

这次项目运行的实际结果是：

```text
Vulkan loader API: 1.4.350
Detected physical devices: 1
[0] Apple M5 Pro
Vulkan setup looks healthy.
```

这说明：

- Vulkan loader 工作正常
- 当前机器可见 GPU 是 `Apple M5 Pro`
- 这个仓库已经可以正常创建 `VkInstance`
- 物理设备枚举已经成功

## 9. 检查本仓库是否能使用 Vulkan

本仓库已经被改成一个最小 Vulkan 项目。

回到仓库根目录执行：

```bash
cd /Users/chenhongchi/Desktop/Engine
cmake -S . -B build
cmake --build build
./build/Engine
```

### 每一步在检查什么

`cmake -S . -B build`

- 检查 CMake 能否找到 Vulkan 头文件和库

`cmake --build build`

- 检查项目是否能成功编译和链接

`./build/Engine`

- 检查程序是否能创建 `VkInstance`
- 检查程序是否能枚举物理设备

正常情况下，程序会打印：

- Vulkan loader API 版本
- 检测到的 GPU 数量
- GPU 名字

## 10. 如何判断是否真的配置成功

你可以按下面这个标准判断。

### 第一层：SDK 已安装

下面命令有结果：

```bash
ls ~/VulkanSDK/1.4.350.0/macOS
```

并且能看到 `bin`、`include`、`lib`、`share`。

### 第二层：终端环境已通

下面命令有结果：

```bash
echo $VULKAN_SDK
which vulkaninfo
vulkaninfo | head
```

### 第三层：项目已通

下面命令成功：

```bash
cd /Users/chenhongchi/Desktop/Engine
cmake -S . -B build
cmake --build build
./build/Engine
```

只有走到第三层，才算这个仓库的 Vulkan 环境真正打通。

## 11. 常见问题

### 1. `which vulkaninfo` 没输出

原因通常是环境变量还没加载。

先执行：

```bash
cd ~/VulkanSDK/1.4.350.0
source setup-env.sh
which vulkaninfo
```

如果这样可以，说明只是 `~/.zshrc` 还没写好或还没重新加载。

### 2. `vulkaninfo` 报错

先检查这几个路径是否存在：

```bash
ls "$VULKAN_SDK/bin"
ls "$VULKAN_SDK/lib"
ls "$VULKAN_SDK/share/vulkan/icd.d"
ls "$VULKAN_SDK/share/vulkan/explicit_layer.d"
```

如果这些目录都存在，再检查：

```bash
echo $VK_ICD_FILENAMES
echo $VK_ADD_LAYER_PATH
```

### 3. CMake 报找不到 Vulkan

先确认：

```bash
echo $VULKAN_SDK
ls "$VULKAN_SDK/include/vulkan"
```

如果这里没问题，再重新开一个终端，重新进入项目执行构建。

### 4. CLion 找不到 Vulkan

macOS 上图形界面程序有时拿不到 shell 里的环境变量。

先在终端确认 Vulkan 环境正常，再完全退出 CLion，重新打开并 `Reload CMake Project`。

如果还是不行，可以尝试从终端启动 CLion：

```bash
open -a CLion
```

## 12. 推荐的检查顺序

每次你怀疑环境有问题时，按这个顺序查：

1. `echo $VULKAN_SDK`
2. `which vulkaninfo`
3. `vulkaninfo | head`
4. `cd /Users/chenhongchi/Desktop/Engine`
5. `cmake -S . -B build`
6. `cmake --build build`
7. `./build/Engine`

这个顺序的好处是能快速判断问题属于：

- SDK 没装好
- 环境变量没生效
- 工具链没接通
- 还是项目本身代码问题

## 13. 本次配置结论

这次机器的基础 C++ 编译环境已经正常。

只要 Vulkan SDK 安装完成，并且 `vulkaninfo` 可以运行，本仓库就应该能够继续完成：

- `VkInstance` 创建
- GPU 枚举
- 后续的逻辑设备、窗口、交换链学习

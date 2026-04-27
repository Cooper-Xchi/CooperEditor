# CooperEditor - OpenGL 项目框架

这是一个基于 OpenGL 和 GLFW 的项目框架。

## 项目结构

```
CooperEditor/
├── src/              # 源文件
│   └── main.cpp     # 程序入口
├── include/         # 头文件
│   ├── shader.h     # 着色器类
│   ├── renderer.h   # 渲染器类
│   └── window.h     # 窗口管理类
├── shaders/         # GLSL 着色器文件
│   ├── vertex.glsl   # 顶点着色器
│   └── fragment.glsl # 片段着色器
├── build/           # 构建输出目录
├── CMakeLists.txt   # CMake 构建配置
└── README.md        # 项目说明
```

## 依赖项

- **OpenGL 3.3+**
- **GLFW3** - 窗口和输入管理
- **GLM** - 数学库（可选）
- **CMake 3.16+**

## 在 macOS 上安装依赖

```bash
brew install glfw3
brew install glm
```

## 构建和运行

```bash
# 创建构建目录并进入
cd build

# 使用 CMake 配置项目
cmake ..

# 编译项目
make

# 运行程序
./bin/CooperEditor
```

## 项目文件说明

### src/main.cpp
- 程序的主入口点
- 初始化 GLFW 和 OpenGL 上下文
- 实现主循环

### include/
- **shader.h**: 着色器程序的管理类
- **renderer.h**: 渲染逻辑的抽象类
- **window.h**: 窗口管理类

### shaders/
- **vertex.glsl**: 顶点着色器示例
- **fragment.glsl**: 片段着色器示例

## 快速开始

1. 实现 `Shader` 类的编译方法
2. 实现 `Renderer` 类的渲染逻辑
3. 在 `main.cpp` 的主循环中添加渲染调用
4. 编写你的 OpenGL 图形代码

## 后续改进建议

- [ ] 实现完整的 Shader 类
- [ ] 实现 Renderer 类
- [ ] 添加模型加载 (Assimp)
- [ ] 添加纹理支持
- [ ] 实现相机系统
- [ ] 添加光照系统
- [ ] 性能优化

## 按键控制

- **ESC**: 关闭窗口

cmake_minimum_required(VERSION 3.16)
project(CooperEditor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# macOS 特定配置：添加 homebrew 的 pkg-config 路径
if(APPLE)
    # 尝试找到 homebrew 安装目录
    execute_process(COMMAND brew --prefix glfw3 OUTPUT_VARIABLE GLFW3_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT GLFW3_DIR)
        # 尝试默认的 homebrew 路径
        set(GLFW3_DIR "/opt/homebrew/opt/glfw3")
    endif()
    
    # 设置 CMake 找到 glfw3 的路径
    list(APPEND CMAKE_PREFIX_PATH "${GLFW3_DIR}")
endif()

# 添加源文件
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h" "include/*.hpp")

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE include)

# OpenGL
find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)

# GLFW - 需要通过 homebrew 或源码安装
find_package(glfw3 REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

# GLAD - 如果使用 glad 库（可选）
# 可以从 https://glad.dav1d.de/ 生成

# 输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

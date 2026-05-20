cmake_minimum_required(VERSION 3.20)

project(Engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED COMPONENTS glslangValidator)
find_package(glfw3 CONFIG REQUIRED)

set(ENGINE_SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(ENGINE_SHADER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${ENGINE_SHADER_BINARY_DIR})

set(ENGINE_SHADER_SOURCES
    ${ENGINE_SHADER_SOURCE_DIR}/triangle.vert
    ${ENGINE_SHADER_SOURCE_DIR}/triangle.frag
)

set(ENGINE_SHADER_OUTPUTS)
foreach(shader_source IN LISTS ENGINE_SHADER_SOURCES)
    get_filename_component(shader_name ${shader_source} NAME)
    set(shader_output ${ENGINE_SHADER_BINARY_DIR}/${shader_name}.spv)
    add_custom_command(
        OUTPUT ${shader_output}
        COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V ${shader_source} -o ${shader_output}
        DEPENDS ${shader_source}
        COMMENT "Compiling shader ${shader_name}"
        VERBATIM
    )
    list(APPEND ENGINE_SHADER_OUTPUTS ${shader_output})
endforeach()

add_custom_target(EngineShaders ALL DEPENDS ${ENGINE_SHADER_OUTPUTS})

add_executable(
    Engine
    main.cpp
    src/engine/Application.cpp
    src/engine/camera/CameraController.cpp
    src/engine/camera/OrthographicCamera.cpp
    src/engine/camera/PerspectiveCamera.cpp
    src/engine/config/AppConfig.cpp
    src/engine/config/AppConfigValidation.cpp
    src/engine/render/Scene.cpp
    src/engine/render/Transform.cpp
    src/engine/vulkan/VulkanBuffer.cpp
    src/engine/vulkan/VulkanContext.cpp
    src/engine/vulkan/VulkanBootstrap.cpp
    src/engine/vulkan/VulkanDevice.cpp
    src/engine/vulkan/VulkanPipeline.cpp
    src/engine/vulkan/VulkanRender.cpp
    src/engine/vulkan/VulkanReporter.cpp
    src/engine/vulkan/VulkanShader.cpp
    src/engine/vulkan/VulkanSwapchain.cpp
    src/engine/window/Window.cpp
)

target_include_directories(Engine PRIVATE include src)
target_link_libraries(Engine PRIVATE Vulkan::Vulkan glfw)
target_compile_definitions(Engine PRIVATE ENGINE_SHADER_DIR="${ENGINE_SHADER_BINARY_DIR}")
add_dependencies(Engine EngineShaders)

#include "engine/window/Window.hpp"

#include <string>
#include <stdexcept>

#include <GLFW/glfw3.h>

#include "engine/config/AppConfig.hpp"

namespace engine::window {

namespace {

Window* userWindow(GLFWwindow* window) {
    return static_cast<Window*>(glfwGetWindowUserPointer(window));
}

}  // namespace

void Window::framebufferResizeCallback(GLFWwindow* window,
                                       int,
                                       int) {
    auto* self = userWindow(window);
    if (self != nullptr) {
        self->framebufferResized_ = true;
    }
}

void Window::keyCallback(GLFWwindow* window,
                         int key,
                         int,
                         int action,
                         int) {
    auto* self = userWindow(window);
    if (self == nullptr || key < 0 ||
        key >= engine::input::InputState::kKeyCount) {
        return;
    }

    if (action == GLFW_PRESS) {
        self->inputState_.keysDown[static_cast<size_t>(key)] = true;
    } else if (action == GLFW_RELEASE) {
        self->inputState_.keysDown[static_cast<size_t>(key)] = false;
    }
}

void Window::cursorPositionCallback(GLFWwindow* window,
                                    double xPosition,
                                    double yPosition) {
    auto* self = userWindow(window);
    if (self == nullptr) {
        return;
    }

    if (self->inputState_.mousePositionValid) {
        self->inputState_.mouseDeltaX += xPosition - self->inputState_.mouseX;
        self->inputState_.mouseDeltaY += yPosition - self->inputState_.mouseY;
    }

    self->inputState_.mouseX = xPosition;
    self->inputState_.mouseY = yPosition;
    self->inputState_.mousePositionValid = true;
}

void Window::mouseButtonCallback(GLFWwindow* window,
                                 int button,
                                 int action,
                                 int) {
    auto* self = userWindow(window);
    if (self == nullptr || button < 0 ||
        button >= engine::input::InputState::kMouseButtonCount) {
        return;
    }

    if (action == GLFW_PRESS) {
        self->inputState_.mouseButtonsDown[static_cast<size_t>(button)] = true;
    } else if (action == GLFW_RELEASE) {
        self->inputState_.mouseButtonsDown[static_cast<size_t>(button)] = false;
    }
}

void Window::scrollCallback(GLFWwindow* window,
                            double xOffset,
                            double yOffset) {
    auto* self = userWindow(window);
    if (self == nullptr) {
        return;
    }

    self->inputState_.scrollDeltaX += xOffset;
    self->inputState_.scrollDeltaY += yOffset;
}

Window::~Window() {
    shutdown();
}

void Window::InitGlfwWindow() {
    //获得glfw配置
    const auto& config = engine::config::appConfig().window; //获取窗口配置

    //初始化glfw
    if (glfwInit() != GLFW_TRUE) {
        throw std::runtime_error("glfwInit failed");
    }
    //是否使用opengl 是否允许重制大小
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //客户端不使用openglapi 因为之后用vulkan接管
    glfwWindowHint(GLFW_RESIZABLE,
                   config.resizable ? GLFW_TRUE : GLFW_FALSE);     //客户端不允许拖动设置大小
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    //创建glfw，传入宽度，高度，标题，是否全屏，是否上下屏
    handle_ = glfwCreateWindow(config.width,
                               config.height,
                               config.title.c_str(),
                               nullptr,
                               nullptr);
    //如果glfw创建失败了就释放资源
    if (handle_ == nullptr) {
        glfwTerminate();
        throw std::runtime_error("glfwCreateWindow failed");
    }
    framebufferResized_ = false;
}

void Window::InitCallBacks() {
    inputState_ = {};
    glfwSetWindowUserPointer(handle_, this);
    glfwSetFramebufferSizeCallback(handle_, framebufferResizeCallback);
    glfwSetKeyCallback(handle_, keyCallback);
    glfwSetCursorPosCallback(handle_, cursorPositionCallback);
    glfwSetMouseButtonCallback(handle_, mouseButtonCallback);
    glfwSetScrollCallback(handle_, scrollCallback);

}

void Window::Initialize() {
    shutdown();     //清理环境
    InitGlfwWindow();
    InitCallBacks();
}

void Window::shutdown() {
    //确保窗口没被创建，否则全部清理
    if (handle_ != nullptr) {
        glfwDestroyWindow(handle_);
        handle_ = nullptr;
    }

    glfwTerminate(); //关闭glfw窗口 释放资源
}

void Window::pollEvents() {
    inputState_.mouseDeltaX = 0.0;
    inputState_.mouseDeltaY = 0.0;
    inputState_.scrollDeltaX = 0.0;
    inputState_.scrollDeltaY = 0.0;
    glfwPollEvents();
}

void Window::waitEvents() const {
    glfwWaitEvents();
}

bool Window::shouldClose() const {
    return handle_ == nullptr || glfwWindowShouldClose(handle_) == GLFW_TRUE;
}

bool Window::consumeFramebufferResized() {
    const bool resized = framebufferResized_;
    framebufferResized_ = false;
    return resized;
}

std::pair<int, int> Window::framebufferSize() const {
    int width = 0;
    int height = 0;
    if (handle_ != nullptr) {
        glfwGetFramebufferSize(handle_, &width, &height);
    }
    return {width, height};
}

const engine::input::InputState& Window::inputState() const noexcept {
    return inputState_;
}

void Window::setTitle(std::string_view title) const {
    if (handle_ == nullptr) {
        return;
    }

    const std::string titleCopy(title);
    glfwSetWindowTitle(handle_, titleCopy.c_str());
}

GLFWwindow* Window::nativeHandle() const noexcept {
    return handle_;
}

}  // namespace engine::window

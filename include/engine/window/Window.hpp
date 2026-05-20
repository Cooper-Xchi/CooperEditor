#pragma once

#include <utility>

#include "engine/input/InputState.hpp"

struct GLFWwindow;

namespace engine::window {

class Window {
public:
    Window() = default;
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void initialize();
    void shutdown();
    void pollEvents();
    void waitEvents() const;
    bool shouldClose() const;
    bool consumeFramebufferResized();
    std::pair<int, int> framebufferSize() const;
    const engine::input::InputState& inputState() const noexcept;

    GLFWwindow* nativeHandle() const noexcept;

private:
    static void framebufferResizeCallback(GLFWwindow* window,
                                          int width,
                                          int height);
    static void keyCallback(GLFWwindow* window,
                            int key,
                            int scancode,
                            int action,
                            int mods);
    static void cursorPositionCallback(GLFWwindow* window,
                                       double xPosition,
                                       double yPosition);
    static void mouseButtonCallback(GLFWwindow* window,
                                    int button,
                                    int action,
                                    int mods);
    static void scrollCallback(GLFWwindow* window,
                               double xOffset,
                               double yOffset);

    GLFWwindow* handle_ = nullptr;
    bool framebufferResized_ = false;
    engine::input::InputState inputState_{};
};

}  // namespace engine::window

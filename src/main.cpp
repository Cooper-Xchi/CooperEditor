#include <iostream>
#include <GLFW/glfw3.h>

// 窗口尺寸
const unsigned int WINDOW_WIDTH = 800;
const unsigned int WINDOW_HEIGHT = 600;

// 回调函数：窗口尺寸改变
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// 处理输入
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main()
{
    // 初始化 GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // 配置 GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // macOS 特定配置
    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // 创建窗口
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "CooperEditor", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // 将当前上下文设置为此窗口
    glfwMakeContextCurrent(window);
    
    // 启用垂直同步
    glfwSwapInterval(1);

    // 设置回调函数
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 初始化视口
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    std::cout << "OpenGL Context Created Successfully!" << std::endl;
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    // 主循环
    while (!glfwWindowShouldClose(window))
    {
        // 处理输入
        processInput(window);

        // 清空颜色缓冲
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // TODO: 在这里添加渲染代码

        // 交换缓冲并检查事件
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 清理资源
    glfwTerminate();
    return 0;
}

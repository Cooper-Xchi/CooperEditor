#ifndef WINDOW_H
#define WINDOW_H

#include <GLFW/glfw3.h>
#include <string>

class Window
{
public:
    Window(unsigned int width, unsigned int height, const std::string& title);
    ~Window();
    
    bool isOpen() const;
    void update();
    void close();
    
    GLFWwindow* getHandle() const { return m_window; }
    unsigned int getWidth() const { return m_width; }
    unsigned int getHeight() const { return m_height; }
    
private:
    GLFWwindow* m_window;
    unsigned int m_width;
    unsigned int m_height;
    
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
};

#endif

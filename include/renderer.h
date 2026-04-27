#ifndef RENDERER_H
#define RENDERER_H

#include <glm/glm.hpp>

class Renderer
{
public:
    Renderer();
    ~Renderer();
    
    void render();
    void update(float deltaTime);
    
private:
    unsigned int VAO, VBO, EBO;
    
    void setupGeometry();
};

#endif

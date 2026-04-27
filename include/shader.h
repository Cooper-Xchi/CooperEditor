#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <glm/glm.hpp>

class Shader
{
public:
    unsigned int ID;
    
    // 构造函数
    Shader() = default;
    
    // 从文件编译着色器程序
    void compile(const char* vertexPath, const char* fragmentPath);
    
    // 使用着色器程序
    void use() const;
    
    // 设置uniform变量
    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
    void setMat4(const std::string &name, const glm::mat4 &mat) const;
    
private:
    // 检查编译/链接错误
    void checkCompileErrors(unsigned int shader, const std::string &type);
};

#endif

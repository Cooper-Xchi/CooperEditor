#pragma once

#include <string>

namespace engine::render {

struct Material {
    std::string vertexShaderFile;
    std::string fragmentShaderFile;
};

}  // namespace engine::render

#include <cstdlib>
#include <exception>
#include <iostream>

#include "engine/Application.hpp"

int main() {
    try {
        engine::Application application; //程序实例
        application.run();  //执行程序
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "Startup failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}

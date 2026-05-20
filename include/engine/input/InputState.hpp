#pragma once

#include <array>

namespace engine::input {

struct InputState {
    static constexpr int kKeyCount = 512;
    static constexpr int kMouseButtonCount = 8;

    std::array<bool, kKeyCount> keysDown{};
    std::array<bool, kMouseButtonCount> mouseButtonsDown{};
    double mouseX = 0.0;
    double mouseY = 0.0;
    double mouseDeltaX = 0.0;
    double mouseDeltaY = 0.0;
    double scrollDeltaX = 0.0;
    double scrollDeltaY = 0.0;
    bool mousePositionValid = false;
};

}  // namespace engine::input

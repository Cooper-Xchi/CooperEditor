#pragma once

#include <string>
#include <vector>

namespace engine::editor {

class ConsolePanel {
public:
    std::vector<std::string> buildLines(
        const std::vector<std::string>& messages) const;
};

}  // namespace engine::editor

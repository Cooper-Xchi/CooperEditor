#include "engine/editor/panels/ConsolePanel.hpp"

namespace engine::editor {

std::vector<std::string> ConsolePanel::buildLines(
    const std::vector<std::string>& messages) const {
    if (messages.empty()) {
        return {"Console is empty"};
    }
    return messages;
}

}  // namespace engine::editor

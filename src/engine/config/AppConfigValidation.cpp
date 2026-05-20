#include "engine/config/AppConfigValidation.hpp"

#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace engine::config {

namespace {

void validateDirectory(const std::string& label,
                       const std::string& path,
                       std::vector<std::string>& errors) {
    const std::filesystem::path directory(path);
    if (!std::filesystem::exists(directory)) {
        errors.push_back(label + " does not exist: " + path);
        return;
    }

    if (!std::filesystem::is_directory(directory)) {
        errors.push_back(label + " is not a directory: " + path);
    }
}

void ensureDirectory(const std::string& label,
                     const std::string& path,
                     std::vector<std::string>& errors) {
    const std::filesystem::path directory(path);
    std::error_code error;

    if (std::filesystem::exists(directory, error)) {
        if (error) {
            errors.push_back(label + " could not be checked: " + path);
            return;
        }

        if (!std::filesystem::is_directory(directory, error)) {
            errors.push_back(label + " is not a directory: " + path);
        } else if (error) {
            errors.push_back(label + " could not be checked: " + path);
        }
        return;
    }

    if (!std::filesystem::create_directories(directory, error) || error) {
        errors.push_back(label + " could not be created: " + path);
    }
}

}  // namespace

void validateAppConfigOrThrow(const AppConfig& config) {
    std::vector<std::string> errors;
    validateDirectory("Shader root", config.paths.shaderRoot, errors);
    validateDirectory("Asset root", config.paths.assetRoot, errors);
    ensureDirectory("Cache root", config.paths.cacheRoot, errors);

    if (errors.empty()) {
        return;
    }

    std::ostringstream builder;
    builder << "App config validation failed:\n";
    for (const auto& error : errors) {
        builder << " - " << error << '\n';
    }

    throw std::runtime_error(builder.str());
}

}  // namespace engine::config

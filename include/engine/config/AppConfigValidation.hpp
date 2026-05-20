#pragma once

#include "engine/config/AppConfig.hpp"

namespace engine::config {

void validateAppConfigOrThrow(const AppConfig& config);

}  // namespace engine::config

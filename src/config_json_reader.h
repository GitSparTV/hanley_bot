#pragma once

#include <filesystem>

#include "config.h"

namespace hanley_bot::config {

Config FromJSONFile(std::filesystem::path path);

} // namespace hanley_bot::config
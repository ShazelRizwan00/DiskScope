#pragma once

#include <filesystem>
#include <optional>

namespace diskscope::platform {

std::optional<std::filesystem::path> pickFolder(const std::filesystem::path& initialDirectory = {});

}  // namespace diskscope::platform

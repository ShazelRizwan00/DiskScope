//Shazel

#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>

namespace diskscope {

// On-disk (allocated) size of a regular file, as opposed to its logical size.
//
// Windows: queried through GetFileInformationByHandleEx(FileStandardInfo) so
// cluster round-up, NTFS compression and sparse files are reflected. Other
// platforms fall back to the logical size.
//
// Returns std::nullopt when the value cannot be determined. That is distinct
// from a genuine 0 (an empty file, or a fully sparse / cloud placeholder file),
// so callers can fall back to the logical size instead of inventing a number.
std::optional<std::uintmax_t> allocatedSizeOf(const std::filesystem::path& file,
                                              std::uintmax_t logicalSize);

}  // namespace diskscope

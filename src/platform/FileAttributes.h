#pragma once

#include <filesystem>

namespace diskscope {

// Windows filesystem attributes that std::filesystem does not expose reliably
// through the MinGW/libstdc++ implementation:
//
//  * NTFS junctions and mount points are reparse points. libstdc++ may report
//    is_symlink() == false for them, which would let the scanner walk into a
//    cycle or into another volume through a junction;
//  * the FILE_ATTRIBUTE_HIDDEN bit is not part of file_status.
//
// Both helpers return false when the information is unavailable (non-Windows
// builds or a stat failure), which makes the scanner's behaviour conservative:
// it only skips what it can positively identify.

// True for any Windows reparse point (symlink, junction, mount point, OneDrive
// placeholder). Reparse points must never be traversed by the scanner.
bool isReparsePoint(const std::filesystem::path& path);

// True when the entry carries FILE_ATTRIBUTE_HIDDEN.
bool isHiddenEntry(const std::filesystem::path& path);

}  // namespace diskscope

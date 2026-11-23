#include "platform/FileAttributes.h"

#if defined(_WIN32)
#include <windows.h>
#endif

namespace diskscope {

bool isReparsePoint(const std::filesystem::path& path) {
#if defined(_WIN32)
    const DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        return false; // unknown: treat as an ordinary entry
    }
    return (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
#else
    (void)path;
    return false;
#endif
}

bool isHiddenEntry(const std::filesystem::path& path) {
#if defined(_WIN32)
    const DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        return false;
    }
    return (attributes & FILE_ATTRIBUTE_HIDDEN) != 0;
#else
    (void)path;
    return false;
#endif
}

}  // namespace diskscope

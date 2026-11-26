//Shazel 

#include "platform/AllocatedSize.h"

#if defined(_WIN32)
// windows.h must come before any other Windows header; NOMINMAX is set by the
// diskscope_core target so std::min/std::max stay usable.
#include <windows.h>
#endif

namespace diskscope {

    std::optional<std::uintmax_t> allocatedSizeOf(const std::filesystem::path& file,
                                                  std::uintmax_t logicalSize) {
#if defined(_WIN32)
    (void)logicalSize; // Windows reports the real allocation below
    // FILE_READ_ATTRIBUTES is the smallest right that still exposes
    // FileStandardInfo, so scanning a file does not need read access to it.
    const HANDLE handle = CreateFileW(file.c_str(), FILE_READ_ATTRIBUTES,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                      nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        return std::nullopt;
    }

    FILE_STANDARD_INFO info{};
    const BOOL ok = GetFileInformationByHandleEx(handle, FileStandardInfo, &info, sizeof(info));
    CloseHandle(handle);

    if (ok == 0) {
        return std::nullopt;
    }
    // AllocationSize reflects cluster round-up, NTFS compression and sparse
    // files. It can legitimately be 0 (empty or fully sparse file), which is why
    // the return type is optional rather than a sentinel.
    return static_cast<std::uintmax_t>(info.AllocationSize.QuadPart);
#else
    (void)file;
    // No portable way to ask for allocated size; the logical size is the honest
    // answer on platforms without it.
    return logicalSize;
#endif
}

}  // namespace diskscope

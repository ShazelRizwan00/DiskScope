#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "core/FolderNode.h"

namespace diskscope {

// A directory or file that could not be read during the scan. Inaccessible
// directories do not abort the scan; they are reported so the UI can state
// honestly that the totals are incomplete.
struct ScanError {
    std::filesystem::path path;
    std::string message;
};

// Live counters published by the scanner while it walks the tree.
struct ScanProgress {
    std::uintmax_t folders = 0;
    std::uintmax_t files = 0;
    std::uintmax_t bytes = 0;
    std::filesystem::path current;
};

struct ScanOptions {
    bool includeHidden = true;        // dotfiles / hidden directories
    bool measureAllocatedSize = true; // second stat per file for on-disk size
    // Publish progress and poll for cancellation every N visited entries.
    // 0 is treated as 1 (never divide by zero).
    std::uint32_t progressInterval = 256;
    // Hard cap on recursion depth. The root has depth 0; a directory is scanned
    // only when its depth is below maxDepth, so maxDepth = 1 scans the root's
    // children but not their children. Deeper branches are truncated and
    // reported as a ScanError.
    std::size_t maxDepth = 256;
    // Cap on collected ScanErrors; further problems only increment
    // ScanResult::omittedErrors so a scan of C:\ cannot exhaust memory.
    std::size_t maxErrors = 500;
};

// Immutable outcome of one scan. The root owns the whole N-ary tree; every
// other structure (index, heap) only keeps non-owning pointers into it.
struct ScanResult {
    std::unique_ptr<FolderNode> root;
    std::vector<ScanError> errors;
    std::uintmax_t omittedErrors = 0;     // problems not stored, see ScanOptions
    std::uintmax_t skippedSymlinks = 0;   // symlinks / junctions, never followed
    std::uintmax_t unreadableFiles = 0;   // size could not be read, not counted
    bool cancelled = false;               // scan stopped early on request

    bool empty() const { return root == nullptr; }
};

using ProgressFn = std::function<void(const ScanProgress&)>;
using CancelFn = std::function<bool()>;

// Recursive depth-first scan of `rootPath`.
//
// Rules enforced here (see docs/architecture.md):
//  * inclusive sizes are computed post-order, once per directory;
//  * symlinks / junctions are never followed and are counted in skippedSymlinks;
//  * permission-denied entries are recorded in `errors` and the walk continues:
//    the scanner uses the error_code overloads of std::filesystem instead of the
//    silent directory_options::skip_permission_denied, which would turn an
//    unreadable directory into an empty one without reporting anything;
//  * `isCancelled` is polled every `options.progressInterval` entries and, when
//    it returns true, a partial tree is returned with `cancelled == true`;
//    every node still equals own bytes + children totals (lower bounds only);
//  * nodes deeper than `options.maxDepth` are truncated with an error;
//  * if the chosen root itself is a symlink/junction it is resolved once for the
//    scan; links below the root are never followed;
//  * the scan root keeps its own inclusive totals (rankings exclude it later).
//
// The function is synchronous and thread-agnostic; ScanController runs it on a
// worker thread.
ScanResult scanDirectory(const std::filesystem::path& rootPath,
                         const ScanOptions& options = {},
                         ProgressFn onProgress = {},
                         CancelFn isCancelled = {});

}  // namespace diskscope

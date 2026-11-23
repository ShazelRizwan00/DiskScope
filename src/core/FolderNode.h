#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace diskscope {

// Stable identifier assigned during a scan, in depth-first discovery order.
// Node 1 is always the scan root. 0 is reserved for "no node".
using NodeId = std::uint64_t;
inline constexpr NodeId kInvalidNodeId = 0;

// One directory of the scanned hierarchy.
//
// This is the N-ary tree node required by the assignment: a directory may own
// any number of subdirectories. Ownership is strictly parent -> children via
// std::unique_ptr, so a FolderNode is destroyed exactly once, together with
// everything below it.
//
// Sizes are *inclusive*: totalBytes already contains every descendant file.
// Never sum a node and its descendants to obtain a storage total.
struct FolderNode {
    // Identity
    NodeId id = kInvalidNodeId;
    // UTF-8 display name (filename().u8string()). std::filesystem may hand back
    // a narrow string in the local ANSI code page, which garbles non-ASCII names
    // under MinGW, so the scanner must store UTF-8 explicitly.
    std::string name;
    std::filesystem::path path;    // normalized absolute path

    // Storage accounting (bytes stay integral; formatting happens in the UI)
    std::uintmax_t ownFileBytes = 0;      // regular files directly inside this directory
    std::uintmax_t ownAllocatedBytes = 0; // on-disk size of those same files
    std::uintmax_t ownFileCount = 0;      // regular files directly inside this directory
    std::uintmax_t totalBytes = 0;        // inclusive logical size of this subtree
    std::uintmax_t allocatedBytes = 0;    // inclusive on-disk size of this subtree

    // Counts
    std::uintmax_t fileCount = 0;    // inclusive: regular files in this subtree
    std::uintmax_t folderCount = 0;  // descendants only, this node excluded

    // False when this directory itself, or a file directly inside it, could not
    // be read. The flag is local: ancestors are not marked, because the UI
    // reports scan-wide problems separately (errors, unreadableFiles).
    bool accessible = true;

    std::vector<std::unique_ptr<FolderNode>> children;

    FolderNode() = default;
    FolderNode(const FolderNode&) = delete;
    FolderNode& operator=(const FolderNode&) = delete;

    bool leaf() const { return children.empty(); }
    std::size_t childCount() const { return children.size(); }
};

}  // namespace diskscope

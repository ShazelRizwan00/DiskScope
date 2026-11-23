#pragma once

// Shared fixtures for the DiskScope tests.
//
// Fixture trees are created at runtime in the system temp directory, so the
// suite never depends on files present on the machine and leaves nothing behind.

#include <algorithm>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

#include "core/FolderNode.h"
#include "structures/MaxHeap.h"

namespace dstest {

// Unique temporary directory, removed on destruction.
class TempDir {
public:
    explicit TempDir(const std::string& label) {
        static int counter = 0;
        std::error_code ec;
        path_ = std::filesystem::temp_directory_path(ec) /
                ("diskscope_test_" + label + "_" + std::to_string(++counter));
        std::filesystem::remove_all(path_, ec);
        std::filesystem::create_directories(path_, ec);
    }

    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }

    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

// Writes a file of exactly `bytes` bytes, creating parent directories.
inline void writeFile(const std::filesystem::path& path, std::size_t bytes) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    const std::string payload(bytes, 'x');
    out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
}

// Stable FolderNode storage for heap tests (a deque never moves its elements).
class NodeStore {
public:
    diskscope::FolderNode* make(std::uintmax_t bytes, const std::string& path) {
        nodes_.emplace_back();
        diskscope::FolderNode& node = nodes_.back();
        node.id = nextId_++;
        node.totalBytes = bytes;
        node.allocatedBytes = bytes;
        node.path = path;
        node.name = std::filesystem::path(path).filename().string();
        return &node;
    }

private:
    std::deque<diskscope::FolderNode> nodes_;
    diskscope::NodeId nextId_ = 1;
};

// The ranking order the assignment specifies, written independently of MaxHeap:
// larger total first, ties broken by path ascending.
inline bool referenceBefore(const diskscope::FolderNode* lhs, const diskscope::FolderNode* rhs) {
    if (lhs->totalBytes != rhs->totalBytes) {
        return lhs->totalBytes > rhs->totalBytes;
    }
    return lhs->path < rhs->path;
}

inline std::vector<const diskscope::FolderNode*> referenceRanking(
    std::vector<const diskscope::FolderNode*> items) {
    std::sort(items.begin(), items.end(), referenceBefore);
    return items;
}

// Inclusive-size identity that every tree must satisfy, used by scanner and
// analysis tests: total = own files + children totals, counts included.
inline bool treeIsConsistent(const diskscope::FolderNode& node, std::string& problem) {
    std::uintmax_t bytes = node.ownFileBytes;
    std::uintmax_t allocated = node.ownAllocatedBytes;
    std::uintmax_t files = node.ownFileCount;
    std::uintmax_t folders = 0;

    for (const auto& child : node.children) {
        if (!child) {
            problem = "null child under " + node.path.string();
            return false;
        }
        bytes += child->totalBytes;
        allocated += child->allocatedBytes;
        files += child->fileCount;
        folders += child->folderCount + 1;
        if (!treeIsConsistent(*child, problem)) {
            return false;
        }
    }

    if (node.totalBytes != bytes) {
        problem = "totalBytes mismatch at " + node.path.string();
        return false;
    }
    if (node.fileCount != files) {
        problem = "fileCount mismatch at " + node.path.string();
        return false;
    }
    if (node.folderCount != folders) {
        problem = "folderCount mismatch at " + node.path.string();
        return false;
    }
    if (node.allocatedBytes != allocated) {
        problem = "allocatedBytes mismatch at " + node.path.string();
        return false;
    }
    return true;
}

inline const diskscope::FolderNode* childNamed(const diskscope::FolderNode& node,
                                               const std::string& name) {
    for (const auto& child : node.children) {
        if (child->name == name) {
            return child.get();
        }
    }
    return nullptr;
}

// A heap is valid when no child outranks its parent.
inline bool heapPropertyHolds(const std::vector<const diskscope::FolderNode*>& array) {
    for (std::size_t i = 1; i < array.size(); ++i) {
        if (array[i] == nullptr ||
            diskscope::MaxHeap::higherPriority(array[i], array[(i - 1) / 2])) {
            return false;
        }
    }
    return true;
}

}  // namespace dstest

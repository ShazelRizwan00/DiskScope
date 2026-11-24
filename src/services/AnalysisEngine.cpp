#include "services/AnalysisEngine.h"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <utility>

#include "utils/Formatting.h"

namespace diskscope {
namespace {

// Component-wise prefix test: "E:/foo" must not match "E:/foobar", which a
// plain string prefix comparison would.
bool isPathPrefix(const std::filesystem::path& prefix, const std::filesystem::path& full) {
    auto prefixIt = prefix.begin();
    auto fullIt = full.begin();
    for (; prefixIt != prefix.end(); ++prefixIt, ++fullIt) {
        if (fullIt == full.end() || *prefixIt != *fullIt) {
            return false;
        }
    }
    return true;
}

}  // namespace

void AnalysisEngine::indexSubtree(FolderNode& node) {
    index_[node.id] = &node;
    discoveryOrder_.push_back(&node);
    for (auto& child : node.children) {
        if (child) {
            indexSubtree(*child);
        }
    }
}

void AnalysisEngine::setResult(ScanResult result) {
    // Dangling-pointer safety: the ranking heap holds non-owning pointers, so it
    // must be emptied before the previous tree is destroyed. Views watch
    // generation() to drop their own cached pointers.
    heap_.clear();
    index_.clear();
    discoveryOrder_.clear();

    result_ = std::move(result);
    if (result_.root) {
        indexSubtree(*result_.root);
    }
    statsCacheValid_ = false;
    ++generation_;
}

void AnalysisEngine::clear() {
    heap_.clear();
    index_.clear();
    discoveryOrder_.clear();
    result_ = ScanResult{};
    includeRoot_ = false;
    statsCacheValid_ = false;
    ++generation_;
}

void AnalysisEngine::rebuildRankings(bool includeRoot) {
    includeRoot_ = includeRoot;
    statsCacheValid_ = false;

    std::vector<const FolderNode*> items;
    items.reserve(discoveryOrder_.size());

    const FolderNode* root = result_.root.get();
    for (FolderNode* node : discoveryOrder_) {
        if (node == nullptr) {
            continue;
        }
        // The scan root is trivially its own largest folder, so it is excluded
        // from rankings unless the user asks for it.
        if (!includeRoot && node == root) {
            continue;
        }
        items.push_back(node);
    }

    heap_.buildHeap(std::move(items));
}

std::vector<const FolderNode*> AnalysisEngine::topK(std::size_t k) const {
    return heap_.getTopK(k);
}

std::vector<const FolderNode*> AnalysisEngine::search(const std::string& term,
                                                      std::size_t limit) const {
    std::vector<const FolderNode*> matches;
    if (term.empty() || limit == 0) {
        return matches;
    }

    const std::string needle = format::toLower(term);
    const FolderNode* root = result_.root.get();

    for (FolderNode* node : discoveryOrder_) {
        if (node == nullptr || node == root) {
            continue;
        }
        if (format::toLower(node->name).find(needle) == std::string::npos) {
            continue;
        }
        matches.push_back(node);
    }

    // Reuse the heap comparator so search results are ordered by exactly the
    // same rule as the rankings, including the tie-break.
    MaxHeap ordered;
    ordered.buildHeap(std::move(matches));

    std::vector<const FolderNode*> results;
    const std::size_t take = std::min(limit, ordered.size());
    results.reserve(take);
    for (std::size_t i = 0; i < take; ++i) {
        results.push_back(ordered.extractMax());
    }
    return results;
}

const FolderNode* AnalysisEngine::find(NodeId id) const {
    const auto it = index_.find(id);
    return it == index_.end() ? nullptr : it->second;
}

AnalysisStats AnalysisEngine::stats() const {
    if (!statsCacheValid_) {
        statsCache_ = computeStats();
        statsCacheValid_ = true;
    }
    return statsCache_;
}

AnalysisStats AnalysisEngine::computeStats() const {
    AnalysisStats stats;
    stats.rootExcludedFromRankings = !includeRoot_;
    stats.indexedFolders = heap_.size();

    const FolderNode* root = result_.root.get();
    if (root == nullptr) {
        return stats;
    }

    stats.hasScan = true;
    stats.totalBytes = root->totalBytes;
    stats.totalAllocatedBytes = root->allocatedBytes;
    stats.fileCount = root->fileCount;
    stats.folderCount = root->folderCount;
    stats.cancelled = result_.cancelled;
    stats.errorCount = static_cast<std::uintmax_t>(result_.errors.size()) + result_.omittedErrors;
    stats.skippedSymlinks = result_.skippedSymlinks;
    stats.unreadableFiles = result_.unreadableFiles;

    // "Largest folder" means the largest folder inside the scan, so it is the
    // best direct child of the root even when the root itself is ranked.
    for (const auto& child : root->children) {
        if (!child) {
            continue;
        }
        if (stats.largestFolder == nullptr || MaxHeap::higherPriority(child.get(), stats.largestFolder)) {
            stats.largestFolder = child.get();
        }
    }

    return stats;
}

std::vector<const FolderNode*> AnalysisEngine::pathTo(const FolderNode* node) const {
    std::vector<const FolderNode*> path;
    const FolderNode* root = result_.root.get();
    if (node == nullptr || root == nullptr) {
        return path;
    }

    path.push_back(root);
    const FolderNode* current = root;

    while (current != node) {
        const FolderNode* next = nullptr;
        for (const auto& child : current->children) {
            if (child && isPathPrefix(child->path, node->path)) {
                next = child.get();
                break;
            }
        }
        if (next == nullptr) {
            path.clear(); // node is not part of this tree
            return path;
        }
        path.push_back(next);
        current = next;
    }
    return path;
}

}  // namespace diskscope

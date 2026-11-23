#include "core/FileSystemScanner.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

#include "core/SizeCalculator.h"
#include "platform/AllocatedSize.h"
#include "platform/FileAttributes.h"

namespace diskscope {
namespace {

// Dotted names are the POSIX spelling of "hidden"; Windows uses an attribute bit
// that libstdc++ does not expose through file_status.
bool hasDotName(const std::filesystem::path& path) {
    const std::string name = path.filename().u8string();
    return !name.empty() && name.front() == '.';
}

// FolderNode::name is UTF-8 by contract. filename() is empty for drive roots
// ("E:/"), so those fall back to the full path text.
std::string utf8NameOf(const std::filesystem::path& path) {
    std::string name = path.filename().u8string();
    if (name.empty()) {
        name = path.u8string();
    }
    return name;
}

// Recursive walker. One instance per scan, used only by that scan's thread.
struct Walker {
    Walker(const ScanOptions& scanOptions, const ProgressFn& progress, const CancelFn& cancel)
        : options(scanOptions),
          onProgress(progress),
          isCancelled(cancel),
          interval(options.progressInterval == 0 ? 1 : options.progressInterval) {
        result.errors.reserve(options.maxErrors);
    }

    const ScanOptions& options;
    const ProgressFn& onProgress;
    const CancelFn& isCancelled;
    const std::uint32_t interval;

    ScanResult result;
    NodeId nextId = 2; // id 1 is reserved for the scan root
    std::uint32_t sinceCheckpoint = 0;
    std::uintmax_t foldersSeen = 1; // the root counts as visited
    std::uintmax_t filesSeen = 0;
    std::uintmax_t bytesSeen = 0;
    std::unordered_set<std::string> visited;
    bool cancelled = false;

    void addError(const std::filesystem::path& path, const std::string& message) {
        if (result.errors.size() < options.maxErrors) {
            result.errors.push_back(ScanError{path, message});
        } else {
            ++result.omittedErrors;
        }
    }

    // Progress and cancellation share one checkpoint so a scan of millions of
    // entries does not pay for a callback per entry.
    void checkpoint(const std::filesystem::path& current) {
        if (++sinceCheckpoint < interval) {
            return;
        }
        sinceCheckpoint = 0;

        if (onProgress) {
            ScanProgress snapshot;
            snapshot.folders = foldersSeen;
            snapshot.files = filesSeen;
            snapshot.bytes = bytesSeen;
            snapshot.current = current;
            onProgress(snapshot);
        }
        if (isCancelled && isCancelled()) {
            cancelled = true;
        }
    }

    // Identity of a directory for the cycle guard: the resolved path when it can
    // be resolved, the absolute path otherwise. Links are never followed, so
    // this only catches configurations where link detection fails.
    std::string realPathKey(const std::filesystem::path& path) const {
        std::error_code ec;
        std::filesystem::path resolved = std::filesystem::weakly_canonical(path, ec);
        if (ec || resolved.empty()) {
            resolved = std::filesystem::absolute(path, ec);
        }
        if (ec) {
            return path.u8string();
        }
        return resolved.lexically_normal().u8string();
    }

    void visit(FolderNode& node, std::size_t depth);
};

void Walker::visit(FolderNode& node, std::size_t depth) {
    if (cancelled) {
        return;
    }
    if (depth >= options.maxDepth) {
        addError(node.path, "maximum scan depth reached; this branch was not scanned");
        node.accessible = false;
        return;
    }
    if (!visited.insert(realPathKey(node.path)).second) {
        addError(node.path, "directory already visited (link cycle)");
        node.accessible = false;
        return;
    }

    std::error_code ec;
    std::filesystem::directory_iterator it(node.path, ec);
    if (ec) {
        // Permission denied or vanished directory: report, count as empty, go on.
        addError(node.path, ec.message());
        node.accessible = false;
        return;
    }

    const std::filesystem::directory_iterator end;
    while (it != end && !cancelled) {
        const std::filesystem::directory_entry entry = *it;
        checkpoint(entry.path());

        std::error_code entryEc;
        const std::filesystem::file_status status = entry.symlink_status(entryEc);
        if (entryEc) {
            addError(entry.path(), entryEc.message());
            node.accessible = false;
        } else if (std::filesystem::is_symlink(status)) {
            // Never followed, never counted: following a link could double count
            // a directory or leave the scanned volume.
            ++result.skippedSymlinks;
        } else {
            // The hidden attribute costs one extra query per entry, so it is only
            // consulted when hidden entries are actually being excluded.
            const bool excludedHidden =
                !options.includeHidden && (hasDotName(entry.path()) || isHiddenEntry(entry.path()));

            if (std::filesystem::is_directory(status)) {
                if (excludedHidden) {
                    // intentionally empty: excluded from the tree and from totals
                } else if (isReparsePoint(entry.path())) {
                    // NTFS junction / mount point: libstdc++ may not call this a
                    // symlink, but traversing it risks a cycle or another volume.
                    ++result.skippedSymlinks;
                } else {
                    auto child = std::make_unique<FolderNode>();
                    child->id = nextId++;
                    child->name = utf8NameOf(entry.path());
                    child->path = entry.path().lexically_normal();

                    FolderNode& childRef = *child;
                    node.children.push_back(std::move(child));
                    ++foldersSeen;
                    visit(childRef, depth + 1);
                }
            } else if (std::filesystem::is_regular_file(status) && !excludedHidden) {
                const std::uintmax_t size = entry.file_size(entryEc);
                if (entryEc) {
                    // Size unreadable: count the problem, never estimate a size.
                    ++result.unreadableFiles;
                    addError(entry.path(), entryEc.message());
                    node.accessible = false;
                } else {
                    node.ownFileBytes += size;
                    ++node.ownFileCount;

                    std::uintmax_t allocated = size;
                    if (options.measureAllocatedSize) {
                        if (const auto measured = allocatedSizeOf(entry.path(), size)) {
                            allocated = *measured;
                        }
                    }
                    node.ownAllocatedBytes += allocated;

                    ++filesSeen;
                    bytesSeen += size;
                }
            }
            // Other file kinds (devices, sockets, FIFOs) hold no storage and are
            // outside the assignment's scope.
        }

        it.increment(ec);
        if (ec) {
            addError(node.path, ec.message());
            node.accessible = false;
            break;
        }
    }
}

std::filesystem::path normalizedRoot(const std::filesystem::path& rootPath, std::error_code& ec) {
    std::filesystem::path root = std::filesystem::absolute(rootPath, ec);
    if (ec) {
        return rootPath;
    }
    return root.lexically_normal();
}

}  // namespace

ScanResult scanDirectory(const std::filesystem::path& rootPath,
                         const ScanOptions& options,
                         ProgressFn onProgress,
                         CancelFn isCancelled) {
    ScanResult result;

    if (rootPath.empty()) {
        result.errors.push_back(ScanError{rootPath, "no directory selected"});
        return result;
    }

    std::error_code ec;
    std::filesystem::path rootDir = normalizedRoot(rootPath, ec);
    if (ec) {
        result.errors.push_back(ScanError{rootPath, ec.message()});
        return result;
    }

    const std::filesystem::file_status status = std::filesystem::symlink_status(rootDir, ec);
    if (ec) {
        result.errors.push_back(ScanError{rootDir, ec.message()});
        return result;
    }

    if (std::filesystem::is_symlink(status) || isReparsePoint(rootDir)) {
        // The user picked this path explicitly, so resolving the root itself is
        // not "following links during traversal"; links below it are still never
        // followed.
        std::filesystem::path resolved = std::filesystem::weakly_canonical(rootDir, ec);
        if (ec || resolved.empty()) {
            result.errors.push_back(ScanError{rootDir, "cannot resolve symbolic link root"});
            return result;
        }
        rootDir = resolved.lexically_normal();
    }

    if (!std::filesystem::is_directory(std::filesystem::symlink_status(rootDir, ec)) || ec) {
        result.errors.push_back(ScanError{rootDir, "not an accessible directory"});
        return result;
    }

    auto root = std::make_unique<FolderNode>();
    root->id = 1;
    root->name = utf8NameOf(rootDir);
    root->path = rootDir;

    Walker walker{options, onProgress, isCancelled};
    walker.visit(*root, 0);

    result = std::move(walker.result);
    result.cancelled = walker.cancelled;
    result.root = std::move(root);

    // One post-order pass turns the collected own-file totals into inclusive
    // totals. A cancelled scan produces a smaller but still consistent tree.
    accumulateSizes(*result.root);

    // Final progress snapshot so the UI can show completed counts instead of
    // stopping at the last periodic checkpoint.
    if (onProgress) {
        ScanProgress finalSnapshot;
        finalSnapshot.folders = walker.foldersSeen;
        finalSnapshot.files = walker.filesSeen;
        finalSnapshot.bytes = walker.bytesSeen;
        finalSnapshot.current = rootDir;
        onProgress(finalSnapshot);
    }

    return result;
}

}  // namespace diskscope

// Filesystem scanner tests (cases 1-4, 13, 14, 16, 17 of the required table).
//
// Fixtures are generated at runtime under the system temp directory. The
// permission-denied and symlink cases use OS facilities (ACL deny / junction) and
// report SKIP instead of passing quietly when the machine refuses to cooperate.

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include "core/FileSystemScanner.h"
#include "core/SizeCalculator.h"
#include "fixtures.h"
#include "test_framework.h"

namespace {

namespace fs = std::filesystem;
using diskscope::ScanOptions;
using diskscope::ScanProgress;
using diskscope::ScanResult;

#if defined(_WIN32)
std::string currentUser() {
    const char* user = std::getenv("USERNAME");
    return user == nullptr ? std::string() : std::string(user);
}

// Denies "read data / list directory" on a directory for the current user, so
// the scanner meets a real permission error. Restores the ACL on scope exit.
class AclDeny {
public:
    explicit AclDeny(fs::path path) : path_(std::move(path)) {}

    ~AclDeny() {
        if (applied_) {
            const std::string cmd = "icacls \"" + path_.string() + "\" /remove:d \"" +
                                    currentUser() + "\" >NUL 2>&1";
            std::system(cmd.c_str());
        }
    }

    bool apply() {
        const std::string user = currentUser();
        if (user.empty()) {
            return false;
        }
        const std::string cmd = "icacls \"" + path_.string() + "\" /deny \"" + user +
                                ":(RD)\" >NUL 2>&1";
        applied_ = std::system(cmd.c_str()) == 0;
        return applied_;
    }

private:
    fs::path path_;
    bool applied_ = false;
};
#endif

// Case 1: a normal directory tree with known sizes.
TEST_CASE(scanner_normal_tree_sizes) {
    dstest::TempDir dir("normal");
    const fs::path root = dir.path() / "root";
    dstest::writeFile(root / "top.bin", 100);
    dstest::writeFile(root / "alpha" / "a1.bin", 1000);
    dstest::writeFile(root / "alpha" / "a2.bin", 2000);
    dstest::writeFile(root / "beta" / "b1.bin", 500);

    const ScanResult result = diskscope::scanDirectory(root);
    CHECK(!result.empty());
    if (result.empty()) {
        return;
    }

    CHECK_EQ(result.root->id, 1u);
    CHECK(result.root->path.is_absolute());
    CHECK_EQ(result.root->totalBytes, 3600u);
    CHECK_EQ(result.root->ownFileBytes, 100u);
    CHECK_EQ(result.root->fileCount, 4u);
    CHECK_EQ(result.root->folderCount, 2u);
    CHECK_EQ(result.root->children.size(), 2u);
    CHECK(result.errors.empty());
    CHECK_EQ(result.skippedSymlinks, 0u);

    const diskscope::FolderNode* alpha = dstest::childNamed(*result.root, "alpha");
    CHECK(alpha != nullptr);
    if (alpha != nullptr) {
        CHECK_EQ(alpha->totalBytes, 3000u);
        CHECK_EQ(alpha->fileCount, 2u);
        CHECK(alpha->id >= 2);
    }
}

// Case 2: an empty directory is a valid scan result, not an error.
TEST_CASE(scanner_empty_directory) {
    dstest::TempDir dir("empty");
    const fs::path root = dir.path() / "empty_root";
    fs::create_directories(root);

    const ScanResult result = diskscope::scanDirectory(root);
    CHECK(!result.empty());
    if (result.empty()) {
        return;
    }
    CHECK_EQ(result.root->totalBytes, 0u);
    CHECK_EQ(result.root->fileCount, 0u);
    CHECK_EQ(result.root->folderCount, 0u);
    CHECK(result.root->children.empty());
    CHECK(result.errors.empty());
}

// Case 3 + 4: nesting and recursive (inclusive) size aggregation.
TEST_CASE(scanner_nested_inclusive_sizes) {
    dstest::TempDir dir("nested");
    const fs::path root = dir.path() / "root";
    dstest::writeFile(root / "a" / "f.bin", 100);
    dstest::writeFile(root / "b" / "c" / "g.bin", 200);
    dstest::writeFile(root / "b" / "c" / "d" / "h.bin", 300);
    fs::create_directories(root / "b" / "c" / "d" / "empty_below");

    const ScanResult result = diskscope::scanDirectory(root);
    CHECK(!result.empty());
    if (result.empty()) {
        return;
    }

    CHECK_EQ(result.root->totalBytes, 600u); // parent is never added to its children
    CHECK_EQ(result.root->folderCount, 5u);

    const diskscope::FolderNode* b = dstest::childNamed(*result.root, "b");
    CHECK(b != nullptr);
    if (b != nullptr) {
        CHECK_EQ(b->totalBytes, 500u);
        const diskscope::FolderNode* c = dstest::childNamed(*b, "c");
        CHECK(c != nullptr);
        if (c != nullptr) {
            CHECK_EQ(c->totalBytes, 500u);
            CHECK_EQ(c->ownFileBytes, 200u); // g.bin sits directly in c
            const diskscope::FolderNode* d = dstest::childNamed(*c, "d");
            CHECK(d != nullptr);
            if (d != nullptr) {
                CHECK_EQ(d->totalBytes, 300u);
                CHECK_EQ(d->folderCount, 1u); // empty_below
            }
        }
    }

    std::string problem;
    CHECK(dstest::treeIsConsistent(*result.root, problem));
    if (!problem.empty()) {
        std::printf("      consistency problem: %s\n", problem.c_str());
    }

    // accumulateSizes must reproduce the same numbers from the own-file totals.
    diskscope::SubtreeTotals totals = diskscope::accumulateSizes(*result.root);
    CHECK_EQ(totals.bytes, result.root->totalBytes);
    CHECK_EQ(totals.files, result.root->fileCount);
    CHECK_EQ(totals.folders, result.root->folderCount + 1);
}

// Hidden entries are excluded from totals when the option is off.
TEST_CASE(scanner_hidden_entries_filter) {
    dstest::TempDir dir("hidden");
    const fs::path root = dir.path() / "root";
    dstest::writeFile(root / "visible.bin", 100);
    dstest::writeFile(root / ".hidden_file", 200);
    dstest::writeFile(root / ".hidden_dir" / "inside.bin", 400);

    ScanOptions options;
    options.includeHidden = false;
    const ScanResult filtered = diskscope::scanDirectory(root, options);
    CHECK(!filtered.empty());
    if (!filtered.empty()) {
        CHECK_EQ(filtered.root->totalBytes, 100u);
        CHECK_EQ(filtered.root->fileCount, 1u);
        CHECK_EQ(filtered.root->folderCount, 0u);
    }

    const ScanResult everything = diskscope::scanDirectory(root);
    CHECK(!everything.empty());
    if (!everything.empty()) {
        CHECK_EQ(everything.root->totalBytes, 700u);
        CHECK_EQ(everything.root->fileCount, 3u);
    }
}

// Case 13: permission denied is reported, not silently swallowed.
TEST_CASE(scanner_permission_denied_is_reported) {
#if defined(_WIN32)
    dstest::TempDir dir("acl");
    const fs::path root = dir.path() / "root";
    dstest::writeFile(root / "open" / "ok.bin", 128);
    dstest::writeFile(root / "locked" / "secret.bin", 4096);

    AclDeny deny(root / "locked");
    if (!deny.apply()) {
        SKIP_TEST("could not apply an ACL deny on this machine");
    }

    const ScanResult result = diskscope::scanDirectory(root);

    CHECK(!result.empty());
    if (result.empty()) {
        return;
    }
    CHECK(!result.errors.empty());          // the failure is recorded
    CHECK_EQ(result.root->totalBytes, 128u); // inaccessible content contributes 0
    const diskscope::FolderNode* locked = dstest::childNamed(*result.root, "locked");
    CHECK(locked != nullptr);
    if (locked != nullptr) {
        CHECK(!locked->accessible);
        CHECK_EQ(locked->totalBytes, 0u);
    }
#else
    SKIP_TEST("ACL deny is Windows specific");
#endif
}

// Case 14: symlinks/junctions are never followed and are counted.
TEST_CASE(scanner_symlinks_are_not_followed) {
    dstest::TempDir dir("links");
    const fs::path root = dir.path() / "root";
    dstest::writeFile(root / "real" / "file.bin", 1000);
    dstest::writeFile(root / "real" / "nested" / "file.bin", 250);

    std::error_code ec;
    fs::create_directory_symlink(root / "real", root / "link_symlink", ec);
    bool linkCreated = !ec;

    if (!linkCreated) {
        // std::filesystem cannot create symlinks on this machine; junctions need
        // no privilege and exercise the same reparse-point code path.
        const std::string cmd = "cmd /c mklink /J \"" + (root / "link_junction").string() +
                                "\" \"" + (root / "real").string() + "\" >NUL 2>&1";
        linkCreated = std::system(cmd.c_str()) == 0;
    }
    if (!linkCreated) {
        SKIP_TEST("neither a symlink nor a junction could be created here");
    }

    const ScanResult result = diskscope::scanDirectory(root);
    CHECK(!result.empty());
    if (result.empty()) {
        return;
    }
    CHECK_EQ(result.skippedSymlinks, 1u);       // the link was skipped and counted
    CHECK_EQ(result.root->totalBytes, 1250u);   // and its target was not counted twice
    CHECK_EQ(result.root->folderCount, 2u);     // real + nested only
    CHECK(dstest::childNamed(*result.root, "link_symlink") == nullptr);
    CHECK(dstest::childNamed(*result.root, "link_junction") == nullptr);

    // A link chosen *as the root* is resolved once (the user picked it explicitly).
    const fs::path linkRoot = linkCreated && !ec ? root / "link_symlink" : root / "link_junction";
    const ScanResult viaLink = diskscope::scanDirectory(linkRoot);
    CHECK(!viaLink.empty());
    if (!viaLink.empty()) {
        CHECK_EQ(viaLink.root->totalBytes, 1250u);
    }
}

// Case 16: cancellation produces a partial but consistent tree.
TEST_CASE(scanner_cancellation_returns_partial_tree) {
    dstest::TempDir dir("cancel");
    const fs::path root = dir.path() / "root";
    for (int i = 0; i < 60; ++i) {
        dstest::writeFile(root / ("d" + std::to_string(i)) / "f.bin", 100);
    }

    const ScanResult full = diskscope::scanDirectory(root);
    CHECK(!full.empty());
    if (full.empty()) {
        return;
    }
    CHECK_EQ(full.root->totalBytes, 6000u);

    ScanOptions options;
    options.progressInterval = 1;
    int polls = 0;
    const ScanResult partial = diskscope::scanDirectory(
        root, options, [](const ScanProgress&) {}, [&polls]() { return ++polls > 4; });

    CHECK(partial.cancelled);
    CHECK(!partial.empty());
    if (partial.empty()) {
        return;
    }
    CHECK(partial.root->totalBytes < full.root->totalBytes); // lower bound, not equal
    std::string problem;
    CHECK(dstest::treeIsConsistent(*partial.root, problem));
    if (!problem.empty()) {
        std::printf("      consistency problem: %s\n", problem.c_str());
    }
}

// Case 17: a larger generated structure still aggregates correctly.
TEST_CASE(scanner_large_structure) {
    dstest::TempDir dir("large");
    const fs::path root = dir.path() / "root";
    constexpr int kFolders = 200;
    constexpr int kFilesPerFolder = 4;
    constexpr std::uintmax_t kFileBytes = 100;

    for (int i = 0; i < kFolders; ++i) {
        const fs::path folder = root / ("folder_" + std::to_string(i));
        for (int f = 0; f < kFilesPerFolder; ++f) {
            dstest::writeFile(folder / ("file_" + std::to_string(f) + ".bin"), kFileBytes);
        }
    }

    const ScanResult result = diskscope::scanDirectory(root);
    CHECK(!result.empty());
    if (result.empty()) {
        return;
    }
    CHECK_EQ(result.root->folderCount, static_cast<std::uintmax_t>(kFolders));
    CHECK_EQ(result.root->fileCount, static_cast<std::uintmax_t>(kFolders * kFilesPerFolder));
    CHECK_EQ(result.root->totalBytes, static_cast<std::uintmax_t>(kFolders * kFilesPerFolder) * kFileBytes);
    CHECK_EQ(result.root->children.size(), static_cast<std::size_t>(kFolders));

    std::string problem;
    CHECK(dstest::treeIsConsistent(*result.root, problem));

    // Every folder has a unique id, which the analysis index depends on.
    std::vector<diskscope::NodeId> ids;
    ids.reserve(result.root->children.size());
    for (const auto& child : result.root->children) {
        ids.push_back(child->id);
    }
    std::sort(ids.begin(), ids.end());
    CHECK(std::adjacent_find(ids.begin(), ids.end()) == ids.end());
}

// Rejected inputs: missing path and a file used as a root.
TEST_CASE(scanner_rejects_invalid_roots) {
    dstest::TempDir dir("invalid");

    const ScanResult missing = diskscope::scanDirectory(dir.path() / "does_not_exist");
    CHECK(missing.empty());
    CHECK(!missing.errors.empty());

    dstest::writeFile(dir.path() / "file.bin", 10);
    const ScanResult fileRoot = diskscope::scanDirectory(dir.path() / "file.bin");
    CHECK(fileRoot.empty());
    CHECK(!fileRoot.errors.empty());

    const ScanResult emptyPath = diskscope::scanDirectory(fs::path());
    CHECK(emptyPath.empty());
    CHECK(!emptyPath.errors.empty());
}

}  // namespace

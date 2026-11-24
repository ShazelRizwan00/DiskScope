// Analysis engine, ranking cross-check and heap-demo tests.
//
// Case 15 (rescanning after filesystem changes) lives here, together with the
// ranking cross-check against a std::sort reference that the assignment asks for.

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

#include "core/FileSystemScanner.h"
#include "fixtures.h"
#include "services/AnalysisEngine.h"
#include "services/HeapDemoEngine.h"
#include "test_framework.h"

namespace {

namespace fs = std::filesystem;
using diskscope::AnalysisEngine;
using diskscope::FolderNode;
using diskscope::HeapDemoEngine;
using diskscope::HeapEvent;
using diskscope::HeapEventKind;

// Collects every non-root folder, the way the engine indexes them.
void collectDescendants(const FolderNode& node, const FolderNode* root,
                        std::vector<const FolderNode*>& out) {
    for (const auto& child : node.children) {
        if (!child) {
            continue;
        }
        if (child.get() != root) {
            out.push_back(child.get());
        }
        collectDescendants(*child, root, out);
    }
}

fs::path buildFixture(const fs::path& sandbox) {
    const fs::path root = sandbox / "fixture";
    dstest::writeFile(root / "top.bin", 100);
    dstest::writeFile(root / "alpha" / "a1.bin", 1000);
    dstest::writeFile(root / "alpha" / "a2.bin", 2000);
    dstest::writeFile(root / "beta" / "b1.bin", 900);
    dstest::writeFile(root / "beta" / "b2.bin", 900); // equal-sized sibling pair
    dstest::writeFile(root / "gamma" / "deep" / "g.bin", 500);
    dstest::writeFile(root / "delta" / "d.bin", 500); // equal to gamma subtree
    return root;
}

// Rankings must be exactly the std::sort-ordered descendant list.
TEST_CASE(analysis_rankings_match_sort_reference) {
    dstest::TempDir dir("rank");
    const fs::path root = buildFixture(dir.path());

    AnalysisEngine engine;
    engine.setResult(diskscope::scanDirectory(root));
    engine.rebuildRankings(false);

    std::vector<const FolderNode*> reference;
    collectDescendants(*engine.result().root, engine.result().root.get(), reference);
    reference = dstest::referenceRanking(reference);

    CHECK_EQ(engine.heap().size(), reference.size());
    CHECK(engine.topK(reference.size()) == reference);

    const std::vector<const FolderNode*> top3 = engine.topK(3);
    CHECK_EQ(top3.size(), 3u);
    if (top3.size() == 3) {
        CHECK(top3[0] == reference[0]);
        CHECK(top3[1] == reference[1]);
        CHECK(top3[2] == reference[2]);
        CHECK_EQ(top3[0]->totalBytes, 3000u); // alpha contains a1 + a2
    }

    // The equal-sized pairs must come out in a stable, path-defined order.
    const std::vector<const FolderNode*> topAll = engine.topK(reference.size());
    for (std::size_t i = 1; i < topAll.size(); ++i) {
        CHECK(!dstest::referenceBefore(topAll[i], topAll[i - 1]));
    }
}

// The scan root is excluded by default and included on request.
TEST_CASE(analysis_root_exclusion_toggle) {
    dstest::TempDir dir("rootexcl");
    const fs::path root = buildFixture(dir.path());

    AnalysisEngine engine;
    engine.setResult(diskscope::scanDirectory(root));

    engine.rebuildRankings(false);
    const std::vector<const FolderNode*> excluded = engine.heap().data();
    CHECK(std::find(excluded.begin(), excluded.end(), engine.result().root.get()) == excluded.end());
    CHECK(engine.stats().rootExcludedFromRankings);

    engine.rebuildRankings(true);
    CHECK(!engine.heap().data().empty());
    CHECK(engine.heap().data().front() == engine.result().root.get());
    CHECK(!engine.stats().rootExcludedFromRankings);
}

TEST_CASE(analysis_stats_lookup_and_generation) {
    dstest::TempDir dir("stats");
    const fs::path root = buildFixture(dir.path());

    AnalysisEngine engine;
    CHECK(!engine.hasResult());
    CHECK_EQ(engine.generation(), 0u);

    engine.setResult(diskscope::scanDirectory(root));
    const std::uint64_t generation = engine.generation();
    CHECK(generation > 0);

    const diskscope::AnalysisStats stats = engine.stats();
    CHECK(stats.hasScan);
    CHECK_EQ(stats.totalBytes, 100u + 3000u + 1800u + 500u + 500u);
    CHECK_EQ(stats.fileCount, 7u);
    CHECK_EQ(stats.folderCount, 5u); // alpha, beta, gamma, gamma/deep, delta
    CHECK(stats.largestFolder != nullptr);
    if (stats.largestFolder != nullptr) {
        CHECK_EQ(stats.largestFolder->name, std::string("alpha"));
    }
    CHECK(engine.find(1) == engine.result().root.get());
    CHECK(engine.find(424242) == nullptr);
    CHECK(engine.result().errors.empty());

    engine.rebuildRankings(false);
    CHECK_EQ(engine.stats().indexedFolders, engine.heap().size());

    const std::uint64_t beforeClear = engine.generation();
    engine.clear();
    CHECK(!engine.hasResult());
    CHECK(engine.heap().empty());
    CHECK(engine.generation() > beforeClear);

    // stats() is cached per mutation, so a cleared engine must report an empty
    // scan rather than the figures of the tree that was just dropped.
    const diskscope::AnalysisStats cleared = engine.stats();
    CHECK(!cleared.hasScan);
    CHECK_EQ(cleared.totalBytes, 0u);
    CHECK_EQ(cleared.indexedFolders, 0u);
}

TEST_CASE(analysis_search_and_path) {
    dstest::TempDir dir("search");
    const fs::path root = buildFixture(dir.path());

    AnalysisEngine engine;
    engine.setResult(diskscope::scanDirectory(root));
    engine.rebuildRankings(false);

    const std::vector<const FolderNode*> matches = engine.search("A");
    CHECK(!matches.empty());
    for (const FolderNode* node : matches) {
        CHECK(node->name.find('a') != std::string::npos ||
              node->name.find('A') != std::string::npos);
    }
    // Results are ordered by the heap comparator, i.e. descending by size.
    for (std::size_t i = 1; i < matches.size(); ++i) {
        CHECK(!dstest::referenceBefore(matches[i], matches[i - 1]));
    }
    CHECK_EQ(engine.search("alpha").size(), 1u);
    CHECK(engine.search("zzz-nothing").empty());
    CHECK(engine.search("").empty());

    const FolderNode* deep = nullptr;
    for (const auto& top : engine.result().root->children) {
        if (top->name != "gamma") {
            continue;
        }
        for (const auto& mid : top->children) {
            if (mid->name == "deep") {
                deep = mid.get();
            }
        }
    }
    CHECK(deep != nullptr);
    if (deep != nullptr) {
        const std::vector<const FolderNode*> chain = engine.pathTo(deep);
        CHECK_EQ(chain.size(), 3u);
        if (chain.size() == 3) {
            CHECK(chain.front() == engine.result().root.get());
            CHECK_EQ(chain[1]->name, std::string("gamma"));
            CHECK(chain.back() == deep);
        }
    }
    CHECK(engine.pathTo(nullptr).empty());
}

// Case 15: rescanning after the filesystem changed replaces tree and rankings.
TEST_CASE(analysis_rescan_after_filesystem_change) {
    dstest::TempDir dir("rescan");
    const fs::path root = buildFixture(dir.path());

    AnalysisEngine engine;
    engine.setResult(diskscope::scanDirectory(root));
    engine.rebuildRankings(false);
    const std::uintmax_t firstTotal = engine.stats().totalBytes;
    const std::uint64_t firstGeneration = engine.generation();
    const std::size_t firstHeapSize = engine.heap().size();
    const FolderNode* alphaBefore = engine.result().root->children.front().get();

    // Grow the largest folder, then rescan.
    dstest::writeFile(root / "alpha" / "a3.bin", 5000);
    engine.setResult(diskscope::scanDirectory(root));
    engine.rebuildRankings(false);

    CHECK(engine.generation() > firstGeneration);
    CHECK_EQ(engine.stats().totalBytes, firstTotal + 5000u);
    CHECK(engine.heap().size() > 0);
    CHECK(engine.topK(1).size() == 1);

    const FolderNode* alphaAfter = nullptr;
    for (const auto& child : engine.result().root->children) {
        if (child->name == "alpha") {
            alphaAfter = child.get();
        }
    }
    CHECK(alphaAfter != nullptr);
    if (alphaAfter != nullptr) {
        CHECK_EQ(alphaAfter->totalBytes, 8000u);
        CHECK(alphaAfter != alphaBefore); // a fresh tree, not a mutated one
    }
    CHECK_EQ(engine.heap().size(), firstHeapSize);
}

// Demo frames must come from the real heap, on a copy, and never touch it.
TEST_CASE(analysis_heap_demo_frames) {
    dstest::TempDir dir("demo");
    const fs::path root = buildFixture(dir.path());

    AnalysisEngine engine;
    engine.setResult(diskscope::scanDirectory(root));
    engine.rebuildRankings(false);
    const std::vector<const FolderNode*> liveBefore = engine.heap().data();

    const std::vector<const FolderNode*> demoItems =
        HeapDemoEngine::selectDemoItems(engine.heap().data(), 5);
    CHECK_EQ(demoItems.size(), 5u);
    CHECK(dstest::heapPropertyHolds(demoItems)); // a heap prefix is still a heap
    CHECK(HeapDemoEngine::selectDemoItems(engine.heap().data(), 0).empty());

    HeapDemoEngine demos;
    const std::vector<HeapEvent> buildFrames = demos.recordBuild(demoItems);
    CHECK(!buildFrames.empty());
    CHECK(buildFrames.front().kind == HeapEventKind::Build);
    CHECK(buildFrames.back().kind == HeapEventKind::Done);
    CHECK(dstest::heapPropertyHolds(buildFrames.back().array));

    const std::vector<HeapEvent> insertFrames =
        demos.recordInsert(demoItems, engine.heap().data().front());
    CHECK(!insertFrames.empty());
    CHECK(insertFrames.front().kind == HeapEventKind::Insert);
    CHECK_EQ(insertFrames.back().array.size(), demoItems.size() + 1);
    CHECK(dstest::heapPropertyHolds(insertFrames.back().array));

    const std::vector<HeapEvent> extractFrames = demos.recordExtract(demoItems);
    CHECK(!extractFrames.empty());
    CHECK(extractFrames.front().kind == HeapEventKind::Extract);
    CHECK_EQ(extractFrames.back().array.size(), demoItems.size() - 1);
    CHECK(dstest::heapPropertyHolds(extractFrames.back().array));

    CHECK(engine.heap().data() == liveBefore); // the live heap was never mutated
    CHECK(engine.heap().validate());
}

}  // namespace

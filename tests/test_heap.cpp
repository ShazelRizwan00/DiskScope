// Max binary heap tests (cases 5-12 of the required test table, plus the
// supporting checks that make those cases meaningful).

#include <algorithm>
#include <random>
#include <string>
#include <vector>

#include "fixtures.h"
#include "structures/MaxHeap.h"
#include "test_framework.h"

namespace {

using diskscope::FolderNode;
using diskscope::HeapEvent;
using diskscope::HeapEventKind;
using diskscope::IHeapObserver;
using diskscope::MaxHeap;

// Case 6: an empty heap must answer safely and stay consistent.
TEST_CASE(heap_empty_behaviour) {
    MaxHeap heap;
    CHECK(heap.empty());
    CHECK_EQ(heap.size(), 0u);
    CHECK(heap.getMax() == nullptr);
    CHECK(heap.extractMax() == nullptr);
    CHECK(heap.validate());
    CHECK(heap.getTopK(10).empty());
}

// Case 5: the smallest non-trivial heap.
TEST_CASE(heap_single_element) {
    dstest::NodeStore store;
    const FolderNode* only = store.make(4096, "E:/only");

    MaxHeap heap;
    heap.insert(only);
    CHECK_EQ(heap.size(), 1u);
    CHECK(heap.getMax() == only);
    CHECK(heap.validate());
    CHECK(heap.extractMax() == only);
    CHECK(heap.empty());
    CHECK(heap.validate());
}

// Case 7: equal sizes must rank deterministically, by path.
TEST_CASE(heap_duplicate_sizes_tie_break) {
    dstest::NodeStore store;
    const FolderNode* b = store.make(7777, "E:/tie/b");
    const FolderNode* a = store.make(7777, "E:/tie/a");
    const FolderNode* c = store.make(7777, "E:/tie/c");

    CHECK(MaxHeap::higherPriority(a, b));
    CHECK(!MaxHeap::higherPriority(b, a));

    MaxHeap heap;
    heap.buildHeap({b, c, a});
    const std::vector<const FolderNode*> order = heap.getTopK(3);
    CHECK_EQ(order.size(), 3u);
    CHECK(order[0] == a);
    CHECK(order[1] == b);
    CHECK(order[2] == c);
}

// Case 9: the heap property must survive every insertion, including duplicates.
TEST_CASE(heap_property_after_every_insert) {
    dstest::NodeStore store;
    std::mt19937 rng(20261123);
    std::uniform_int_distribution<int> sizes(0, 9);

    MaxHeap heap;
    bool invariantHeld = true;
    bool rootTracked = true;
    const FolderNode* expectedMax = nullptr;

    for (int i = 0; i < 200; ++i) {
        const FolderNode* node = store.make(static_cast<std::uintmax_t>(sizes(rng)) * 512,
                                            "E:/d" + std::to_string(i % 37));
        heap.insert(node);
        invariantHeld = invariantHeld && heap.validate();
        if (expectedMax == nullptr || dstest::referenceBefore(node, expectedMax)) {
            expectedMax = node;
        }
        rootTracked = rootTracked && heap.getMax() == expectedMax;
    }

    CHECK(invariantHeld);
    CHECK(rootTracked);
    CHECK_EQ(heap.size(), 200u);
}

// Case 10 + 8: extraction order and invariant after every extraction.
TEST_CASE(heap_extraction_order_and_invariant) {
    dstest::NodeStore store;
    std::mt19937 rng(4242);
    std::vector<const FolderNode*> items;
    for (int i = 0; i < 150; ++i) {
        items.push_back(store.make(static_cast<std::uintmax_t>(rng() % 3000) * 10,
                                   "E:/x/" + std::to_string(i)));
    }

    MaxHeap heap;
    heap.buildHeap(items);
    CHECK(heap.validate());

    std::vector<const FolderNode*> extracted;
    bool invariantHeld = true;
    while (!heap.empty()) {
        extracted.push_back(heap.extractMax());
        invariantHeld = invariantHeld && heap.validate();
    }

    CHECK(invariantHeld);
    CHECK_EQ(extracted.size(), items.size());
    const std::vector<const FolderNode*> expected = dstest::referenceRanking(items);
    CHECK(extracted == expected);
}

// Case 11: bottom-up construction from an arbitrary order.
TEST_CASE(heap_build_is_bottom_up_and_correct) {
    dstest::NodeStore store;
    std::mt19937 rng(7);
    std::vector<const FolderNode*> items;
    for (int i = 0; i < 63; ++i) {
        items.push_back(store.make(static_cast<std::uintmax_t>(rng() % 4000),
                                   "E:/build/" + std::to_string(i)));
    }
    std::shuffle(items.begin(), items.end(), rng);

    MaxHeap heap;
    heap.buildHeap(items);
    CHECK(heap.validate());
    CHECK_EQ(heap.size(), items.size());
    CHECK(heap.getMax() == dstest::referenceRanking(items).front());

    std::vector<const FolderNode*> stored = heap.data();
    std::sort(stored.begin(), stored.end());
    std::vector<const FolderNode*> sortedInput = items;
    std::sort(sortedInput.begin(), sortedInput.end());
    CHECK(stored == sortedInput);
}

// Case 12: K larger than the number of folders, and K == 0.
TEST_CASE(heap_top_k_beyond_size) {
    dstest::NodeStore store;
    std::vector<const FolderNode*> items;
    for (int i = 0; i < 20; ++i) {
        items.push_back(store.make(static_cast<std::uintmax_t>(100 - i),
                                   "E:/k/" + std::to_string(i)));
    }

    MaxHeap heap;
    heap.buildHeap(items);
    const std::vector<const FolderNode*> before = heap.data();

    CHECK_EQ(heap.getTopK(50).size(), 20u);
    CHECK(heap.getTopK(50) == dstest::referenceRanking(items));
    CHECK(heap.getTopK(0).empty());
    CHECK(heap.data() == before); // top-k works on a copy
    CHECK(heap.validate());
}

// Supporting check: heapify and the recorded frames of the real algorithm.
TEST_CASE(heap_heapify_and_recording) {
    dstest::NodeStore store;
    std::vector<const FolderNode*> items;
    for (int i = 0; i < 9; ++i) {
        items.push_back(store.make(static_cast<std::uintmax_t>(50 + i * 13),
                                   "E:/r/" + std::to_string(i)));
    }

    MaxHeap plain;
    plain.buildHeap(items);
    const std::vector<const FolderNode*> before = plain.data();
    plain.heapify(0);
    CHECK(plain.data() == before);
    plain.heapify(9999); // out of range: no-op
    CHECK(plain.data() == before);

    struct Recorder : IHeapObserver {
        std::vector<HeapEvent> frames;
        void onHeapEvent(const HeapEvent& event) override { frames.push_back(event); }
    };

    Recorder recorder;
    MaxHeap recorded(&recorder);
    recorded.buildHeap(items);

    CHECK(!recorder.frames.empty());
    CHECK(recorder.frames.front().kind == HeapEventKind::Build);
    CHECK(recorder.frames.back().kind == HeapEventKind::Done);
    CHECK(recorder.frames.back().array == recorded.data());
    CHECK(recorded.data() == plain.data());

    bool sameItems = true;
    bool doneFramesValid = true;
    std::vector<const FolderNode*> sortedItems = items;
    std::sort(sortedItems.begin(), sortedItems.end());
    for (const HeapEvent& frame : recorder.frames) {
        std::vector<const FolderNode*> snapshot = frame.array;
        std::sort(snapshot.begin(), snapshot.end());
        sameItems = sameItems && snapshot == sortedItems;
        if (frame.kind == HeapEventKind::Done) {
            doneFramesValid = doneFramesValid && dstest::heapPropertyHolds(frame.array);
        }
    }
    CHECK(sameItems);       // every snapshot holds the same folders
    CHECK(doneFramesValid); // every settled state is a valid heap
}

}  // namespace

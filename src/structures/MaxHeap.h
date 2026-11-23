#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/FolderNode.h"
#include "structures/HeapEvent.h"

namespace diskscope {

// Custom max binary heap over folders, ordered by inclusive folder size.
//
// This is the assessed data structure of the assignment: it is written by hand
// on top of a std::vector and never delegates to std::priority_queue,
// std::push_heap, std::pop_heap, std::make_heap or any other library heap.
//
// Layout invariants (0-based level order):
//   parent(i)      = (i - 1) / 2
//   leftChild(i)   = 2 * i + 1
//   rightChild(i)  = 2 * i + 2
//   items_[parent] >= items_[child]  for every non-root slot
//
// Ordering key:
//   1. larger totalBytes wins;
//   2. on equal sizes the lexicographically smaller path wins, which makes the
//      ranking deterministic across runs and machines.
//
// The heap stores *non-owning* pointers. The caller guarantees that the
// FolderNodes outlive the heap (they are owned by ScanResult::root).
//
// Complexity (D = number of indexed folders):
//   insert        O(log D)
//   getMax        O(1)
//   extractMax    O(log D)
//   heapify(i)    O(log D)
//   buildHeap     O(D)   bottom-up construction
//   getTopK(k)    O(D + k log D)  on a copy, the live heap is not modified
class MaxHeap {
public:
    // `observer`, when non-null, receives one HeapEvent per algorithm step.
    // Pass nullptr (the default) for the production heap.
    explicit MaxHeap(IHeapObserver* observer = nullptr);

    // O(D) bottom-up construction (heapify from the last parent down to the
    // root) out of an arbitrary level-order item array.
    void buildHeap(std::vector<const FolderNode*> items);

    // O(log D) insertion at the next free slot followed by sift-up.
    void insert(const FolderNode* node);

    // O(1). Returns nullptr when the heap is empty.
    const FolderNode* getMax() const;

    // O(log D). Removes and returns the largest folder, or nullptr if empty.
    const FolderNode* extractMax();

    // O(log D). Restores the heap property for the subtree rooted at `index`
    // (sift-down); exposed because the assignment requires heapify() as an
    // operation in its own right.
    void heapify(std::size_t index);

    // O(D + k log D). Descending top-k without modifying this heap: a copy is
    // built bottom-up and the maximum is extracted k times. Returns at most
    // size() items, so k > D is legal.
    std::vector<const FolderNode*> getTopK(std::size_t k) const;

    std::size_t size() const { return items_.size(); }
    bool empty() const { return items_.empty(); }
    const std::vector<const FolderNode*>& data() const { return items_; }
    void clear();

    // Test-only invariant check: parent/child ordering plus pointer sanity.
    bool validate() const;

    // The comparison used by every sift operation, public so tests can assert
    // the tie-breaking rule directly.
    static bool higherPriority(const FolderNode* lhs, const FolderNode* rhs);

private:
    void siftUp(std::size_t index);
    void siftDownFrom(std::size_t index);
    void swapSlots(std::size_t a, std::size_t b);
    void record(HeapEventKind kind, std::size_t a, std::size_t b, std::string note);

    static std::size_t parentOf(std::size_t index) { return (index - 1) / 2; }
    static std::size_t leftOf(std::size_t index) { return 2 * index + 1; }
    static std::size_t rightOf(std::size_t index) { return 2 * index + 2; }

    std::vector<const FolderNode*> items_;
    IHeapObserver* observer_ = nullptr;
};

}  // namespace diskscope

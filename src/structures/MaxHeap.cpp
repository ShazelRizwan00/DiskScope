#include "structures/MaxHeap.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>

namespace diskscope {
namespace {

// Short, allocation-free notes: they only exist for the visualizer, and only
// MaxHeap::record() ever stores them.
std::string slotNote(const char* verb, std::size_t a, std::size_t b) {
    return std::string(verb) + " " + std::to_string(a) + " vs " + std::to_string(b);
}

}  // namespace

MaxHeap::MaxHeap(IHeapObserver* observer) : observer_(observer) {}

bool MaxHeap::higherPriority(const FolderNode* lhs, const FolderNode* rhs) {
    if (lhs == rhs) {
        return false;
    }
    if (lhs->totalBytes != rhs->totalBytes) {
        return lhs->totalBytes > rhs->totalBytes; // larger folder wins
    }
    // Equal sizes: the lexicographically smaller path wins, so rankings are
    // deterministic across runs and machines. Paths are unique within one scan,
    // so this is a strict total order; the id check only guards against two
    // nodes built from the same path.
    const int cmp = lhs->path.compare(rhs->path);
    if (cmp != 0) {
        return cmp < 0;
    }
    return lhs->id < rhs->id;
}

void MaxHeap::record(HeapEventKind kind, std::size_t a, std::size_t b, std::string note) {
    if (observer_ == nullptr) {
        return;
    }
    HeapEvent event;
    event.kind = kind;
    event.a = a;
    event.b = b;
    event.array = items_; // full snapshot after the step, for playback
    event.note = std::move(note);
    observer_->onHeapEvent(event);
}

void MaxHeap::swapSlots(std::size_t a, std::size_t b) {
    std::swap(items_[a], items_[b]);
    record(HeapEventKind::Swap, a, b, slotNote("swap", a, b));
}

void MaxHeap::siftUp(std::size_t index) {
    std::size_t current = index;
    while (current > 0) {
        const std::size_t parent = parentOf(current);
        record(HeapEventKind::Compare, current, parent, slotNote("compare", current, parent));
        if (!higherPriority(items_[current], items_[parent])) {
            return; // parent already dominates: heap property restored
        }
        swapSlots(current, parent);
        current = parent;
    }
}

void MaxHeap::siftDownFrom(std::size_t index) {
    const std::size_t n = items_.size();
    std::size_t current = index;

    for (;;) {
        const std::size_t left = leftOf(current);
        const std::size_t right = rightOf(current);
        std::size_t best = current;

        if (left < n) {
            record(HeapEventKind::Compare, current, left, slotNote("compare", current, left));
            if (higherPriority(items_[left], items_[best])) {
                best = left;
            }
        }
        if (right < n) {
            record(HeapEventKind::Compare, current, right, slotNote("compare", current, right));
            if (higherPriority(items_[right], items_[best])) {
                best = right;
            }
        }
        if (best == current) {
            return; // this slot dominates both children
        }
        swapSlots(current, best);
        current = best;
    }
}

void MaxHeap::heapify(std::size_t index) {
    if (index >= items_.size()) {
        return;
    }
    record(HeapEventKind::SiftDown, index, kNoIndex, "heapify " + std::to_string(index));
    siftDownFrom(index);
}

void MaxHeap::buildHeap(std::vector<const FolderNode*> items) {
    items_ = std::move(items);
    record(HeapEventKind::Build, kNoIndex, kNoIndex,
           "build from " + std::to_string(items_.size()) + " folders");

    // Bottom-up construction: every slot from the last parent down to the root.
    // Slots n/2 .. n-1 are leaves, so they are already valid one-node heaps.
    for (std::size_t i = items_.size() / 2; i-- > 0;) {
        siftDownFrom(i);
    }
    record(HeapEventKind::Done, kNoIndex, kNoIndex, "heap built");
}

void MaxHeap::insert(const FolderNode* node) {
    if (node == nullptr) {
        return; // never store a null folder; it would break every comparison
    }
    items_.push_back(node);
    const std::size_t index = items_.size() - 1;
    record(HeapEventKind::Insert, index, kNoIndex, "append at slot " + std::to_string(index));
    if (index > 0) {
        record(HeapEventKind::SiftUp, index, parentOf(index),
               slotNote("sift up from", index, parentOf(index)));
        siftUp(index);
    }
    record(HeapEventKind::Done, kNoIndex, kNoIndex, "inserted");
}

const FolderNode* MaxHeap::getMax() const {
    return items_.empty() ? nullptr : items_.front();
}

const FolderNode* MaxHeap::extractMax() {
    if (items_.empty()) {
        record(HeapEventKind::Done, kNoIndex, kNoIndex, "extract from empty heap");
        return nullptr;
    }

    const FolderNode* maximum = items_.front();
    const std::size_t lastSlot = items_.size() - 1;

    // The last leaf takes the root slot, then sift-down restores the order.
    // Moving the element is deliberate: nothing is compared against a stale root.
    items_[0] = items_[lastSlot];
    items_.pop_back();
    record(HeapEventKind::Extract, 0, lastSlot, "root removed, slot " + std::to_string(lastSlot) + " moved to 0");

    if (!items_.empty()) {
        record(HeapEventKind::SiftDown, 0, kNoIndex, "sift down from root");
        siftDownFrom(0);
    }
    record(HeapEventKind::Done, kNoIndex, kNoIndex, "extracted max");
    return maximum;
}

std::vector<const FolderNode*> MaxHeap::getTopK(std::size_t k) const {
    std::vector<const FolderNode*> result;
    if (k == 0 || items_.empty()) {
        return result;
    }

    // Work on a copy with no observer: the live heap keeps its exact state and
    // the copy does not waste time recording frames nobody will show.
    MaxHeap working;
    working.buildHeap(items_);

    const std::size_t take = std::min(k, working.size());
    result.reserve(take);
    for (std::size_t i = 0; i < take; ++i) {
        result.push_back(working.extractMax());
    }
    return result;
}

void MaxHeap::clear() {
    items_.clear();
}

bool MaxHeap::validate() const {
    for (std::size_t i = 0; i < items_.size(); ++i) {
        if (items_[i] == nullptr) {
            return false;
        }
        if (i > 0 && higherPriority(items_[i], items_[parentOf(i)])) {
            return false; // child outranks its parent
        }
    }
    return true;
}

}  // namespace diskscope

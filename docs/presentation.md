# DiskScope — Presentation Outline

## Slide 1: Title & Overview
- **Title:** DiskScope: High-Performance Disk Analysis with Custom Binary Heap
- **Course & Context:** Data Structures and Algorithms Capstone Project
- **Core Technology:** C++17, Dear ImGui, Custom Max Binary Heap, N-ary Directory Tree
- **Key Objective:** Real-time inclusive storage analysis and honest algorithm visualization without standard library heap utilities.

## Slide 2: The Problem: Filesystem Blindness
- Standard OS directory listings do not aggregate recursive contents; folder sizes appear as zero or entry metadata sizes.
- Calculating storage requires traversing deep hierarchical directories containing millions of entries.
- Naive re-sorting across $D$ folders on every user interaction costs $O(D \log D)$ and discards priority relationships.
- Goal: Immediate peak identification ($O(1)$) and efficient Top-$K$ ranking with dynamic maintenance.

## Slide 3: Application Architecture & Five Core Views
- Multi-view graphical interface built on Dear ImGui v1.91.9b and ImPlot v0.17 running on GLFW 3.4.
- **Dashboard:** Storage breakdown, donut charts, and integrity status banners.
- **Directory Explorer:** Interactive tree view reflecting the recursive N-ary hierarchy.
- **Largest Folders:** Descending priority list driven directly by the max heap.
- **Heap Visualizer:** Playback engine rendering complete binary tree levels and backing array.
- **Settings:** Configurable traversal options and root inclusion toggles.

## Slide 4: System Architecture & Thread Separation
- Strict separation between headless core library (`diskscope_core`) and graphical shell (`diskscope_ui`).
- Background worker thread (`ScanController`) performs non-blocking recursive traversal.
- Main UI thread handles rendering, user inputs, and analytical queries (`AnalysisEngine`).
- Safe atomic handoff: worker builds isolated tree; main thread adopts ownership and rebuilds rankings in a single atomic frame.

## Slide 5: N-ary Directory Tree & Post-Order Aggregation
- Modeled via `FolderNode` struct where parent uniquely owns child subtrees via `std::vector<std::unique_ptr<FolderNode>>`.
- Stable raw pointers (`const FolderNode*`) stored in downstream indices without danger of pointer invalidation.
- Inclusive sizes computed in a single post-order pass:
  $$\text{totalBytes}(\text{node}) = \text{ownFileBytes} + \sum_{c \in \text{children}} \text{totalBytes}(c)$$
- Strict $O(N)$ runtime complexity where $N = D + F$.

## Slide 6: Custom Max Binary Heap Design
- Complete binary tree embedded in a contiguous 0-indexed vector:
  $$\text{parent}(i) = \lfloor (i - 1)/2 \rfloor, \quad \text{left}(i) = 2i + 1, \quad \text{right}(i) = 2i + 2$$
- Strictly written from scratch; no `std::priority_queue`, `std::make_heap`, or `std::push_heap`.
- Bottom-up construction (`buildHeap`) achieves optimal $O(D)$ runtime versus $O(D \log D)$ iterative insertion.
- Enforces strict invariants: shape completeness, parent dominance ($A[p] \ge A[c]$), and pointer validity.

## Slide 7: Deterministic Priority & Top-K Extraction
- Primary priority key: inclusive byte size (`totalBytes`).
- Secondary tie-breaker: lexicographical path comparison (`lhs->path < rhs->path`).
- Eliminates non-determinism across platforms, file systems, and runs.
- Top-$K$ extraction operates on an isolated heap clone:
  - Preserves live ranking heap integrity.
  - Runtime: $O(D + K \log D)$ time, returning empty when $K=0$ and at most $D$ elements when $K > D$.

## Slide 8: The Honest Algorithm Visualizer
- Visualizer replays real algorithm steps; zero synthetic or hardcoded animations.
- `IHeapObserver` interface receives discrete `HeapEvent` records during sift operations.
- Full array snapshots taken after each step (`Compare`, `Swap`, `SiftUp`, `SiftDown`).
- Operations recorded on an isolated demo clone capped at 63 nodes (6 levels) to guarantee visual legibility and bounded memory.

## Slide 9: Storage Accounting & Filesystem Robustness
- Distinguishes between logical file size (`file_size`) and on-disk allocated cluster size (`AllocatedSize`).
- Symlinks and NTFS junctions are strictly not followed, preventing traversal loops.
- Explicit error handling: permission-denied directories return `accessible = false` and log errors without crashing.
- Cancellation checkpoints yield consistent partial subtrees (valid lower bounds on storage).

## Slide 10: Empirical Verification & Testing Rigor
- Unit test suite: 23 cases, 192 individual checks, 0 failures, 0 skips.
- CTest suite passing with 100% success rate.
- Deterministic smoke fixture validated off-screen via automated BMP pixel sampling.
- Benchmark: 401 directories / 1,200 files scanned and captured in 1.14 s; full 9,000-file project repository in 4.21 s.

## Slide 11: Limitations & Future Improvement
- Current limitations: hard links accounted per directory entry; folder dialog Windows-specific.
- Proposed enhancement: **Incremental Rescan via Filesystem Journaling**.
- Monitor change notifications (`ReadDirectoryChangesW` / USN Journal).
- Re-traverse only modified subtrees ($M$ nodes) and bubble updates up ancestor chains in $O(M + \text{depth} \cdot \log D)$.

## Slide 12: Summary & Viva Q&A Readiness
- Demonstrates rigorous data structure design: N-ary tree and custom max binary heap in modern C++17.
- Complete algorithmic proofs, index arithmetic, and dry traces documented.
- Verified test suite and automated smoke run provide concrete empirical evidence of correctness.
- Open for technical questions and live code walkthrough.

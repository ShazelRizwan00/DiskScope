# Team Contributions and Attribution Matrix

## Project Information

- **Course Title:** Data Structures and Algorithms
- **Project Title:** DiskScope: Disk Space Analyzer with Custom Max Binary Heap
- **Submission Date:** `[Insert Date: YYYY-MM-DD]`

---

## 1. Division of Responsibilities

The workload for the DiskScope project was structured into five distinct engineering areas. The table below documents the assignment of components, affected source files, and concrete verification evidence:

| Student Name & ID | Project Component | Key Assigned Files | Tangible Verification Evidence |
|---|---|---|---|
| `[Student Name 1]`<br>`[ID 1]` | **Core Scanner & Filesystem Engine** | `src/core/FileSystemScanner.cpp`<br>`src/core/FileSystemScanner.h`<br>`src/core/SizeCalculator.cpp`<br>`src/core/SizeCalculator.h`<br>`src/platform/AllocatedSize.cpp`<br>`src/platform/FileAttributes.cpp` | Implemented recursive DFS traversal and post-order aggregation; passed 9 scanner test cases in `test_scanner.cpp`. |
| `[Student Name 2]`<br>`[ID 2]` | **Custom Max Binary Heap & Demo Engine** | `src/structures/MaxHeap.cpp`<br>`src/structures/MaxHeap.h`<br>`src/structures/HeapEvent.h`<br>`src/services/HeapDemoEngine.cpp`<br>`src/services/HeapDemoEngine.h` | Authored custom heap from scratch without STL heap utilities; passed 8 heap unit tests in `test_heap.cpp` and tie-break validation. |
| `[Student Name 3]`<br>`[ID 3]` | **UI Application, Views & Visualizers** | `src/ui/Application.cpp`<br>`src/ui/MainWindow.cpp`<br>`src/ui/Dashboard.cpp`<br>`src/ui/DirectoryExplorer.cpp`<br>`src/ui/LargestFoldersView.cpp`<br>`src/ui/HeapVisualizerView.cpp`<br>`src/visualization/HeapRenderer.cpp`<br>`src/visualization/ChartRenderer.cpp` | Built five Dear ImGui / ImPlot views, complete binary tree renderer, and offline smoke capture mechanism in `main.cpp`. |
| `[All Team Members]` | **Services, Analysis & Concurrency** | `src/services/AnalysisEngine.cpp`<br>`src/services/AnalysisEngine.h`<br>`src/services/ScanController.cpp`<br>`src/services/ScanController.h`<br>`src/main.cpp` | Integrated worker thread concurrency, mutex locks, cancellation polling, and analytical query caches. |
| `[All Team Members]` | **Testing, Documentation & Toolchain** | `tests/test_*.cpp`<br>`docs/*.md`<br>`scripts/build.*`<br>`scripts/setup_toolchain.*`<br>`CMakeLists.txt` | Authored 23 unit test cases, manual dry trace, verification reports, portable toolchain scripts, and CMake build configuration. |

---

## 2. Component Contribution Summary

### Component A: Core Filesystem Scanner & Storage Accounting
- Implemented depth-first directory traversal using standard C++17 `std::filesystem`.
- Implemented single-pass post-order size aggregation in `accumulateSizes()`.
- Implemented error handling for locked directories (`icacls /deny`) and junction skip logic.
- Maintained thread-safe progress callbacks and cancellation checks.

### Component B: Custom Max Binary Heap
- Implemented complete binary tree indexing using dynamic array backing (`std::vector<const FolderNode*>`).
- Designed $O(D)$ bottom-up `buildHeap` algorithm and logarithmic `siftUp` / `siftDown`.
- Enforced deterministic path-based tie-breaking for identical byte sizes.
- Developed non-mutating `getTopK` queries operating on isolated copies.

### Component C: Graphical User Interface & Visualization
- Created the Dear ImGui / ImPlot application shell and five distinct navigation views.
- Implemented step-by-step playback renderer with node-level coordinate positioning.
- Engineered offline headless smoke test with automated 24-bit BMP image capture.

### Component D: Verification and Cross-Testing
- Conducted internal peer code reviews across all modules before final integration.
- Designed adversarial test cases verifying tree invariance under random insertions.
- Validated build toolchain scripts across both Git Bash and Windows PowerShell environments.

---

## 3. Academic Integrity Statement

We, the undersigned students, certify that:
1. The division of work detailed above accurately reflects each member's contributions.
2. All members contributed actively to the design, implementation, debugging, and verification of DiskScope.
3. Every team member has reviewed and understands the complete codebase and is prepared to defend it during the viva examination.

| Student Name | Student ID | Signature | Date |
|---|---|---|---|
| `[Student Name 1]` | `[Student ID 1]` | `____________________` | `[YYYY-MM-DD]` |
| `[Student Name 2]` | `[Student ID 2]` | `____________________` | `[YYYY-MM-DD]` |
| `[Student Name 3]` | `[Student ID 3]` | `____________________` | `[YYYY-MM-DD]` |

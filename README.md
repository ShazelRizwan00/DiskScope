# DiskScope

DiskScope is a high-performance C++17 disk-space analyzer for Windows featuring custom data structures: an N-ary directory tree with post-order inclusive size aggregation and a hand-written max binary heap for ranking folders. It includes an interactive GUI built with Dear ImGui and an algorithm visualizer that replays recorded frames of the real heap operations.

## Features

- Non-blocking recursive directory scanner running on a dedicated worker thread.
- Inclusive size accounting tracking both logical file lengths and on-disk allocated sizes.
- Custom max binary heap with deterministic tie-breaking (no STL heap algorithms).
- Top-K directory ranking and real-time substring search.
- Honest algorithm visualizer replaying frame-by-frame execution snapshots on heap copies.
- Headless automated smoke test rendering off-screen BMP captures for verification.

## Application Views

1. **Dashboard (Overview):** Storage summary metrics, status banners, and visual distribution charts.
2. **Directory Explorer:** Interactive drill-down view of the N-ary directory tree and child table.
3. **Largest Folders:** Descending ranking of largest folders via heap extraction, with path filtering.
4. **Heap Visualizer:** Step-by-step playback of build, insert, and extract operations with tree and array views.
5. **Settings:** Scan options (hidden files, allocated sizes, depth limits), ranking root exclusion, and visualizer speed.

## Prerequisites and Toolchain

- **Compiler:** GCC 15.2.0 (MSYS2 MinGW-w64). `E:/Compilers/mingw64/bin` must be on `PATH`.
- **Build Tools:** Portable CMake 4.5.1 and Ninja 1.13.2 provisioned under `.tools/`.
- **Dependencies:** Pinned via CMake FetchContent: Dear ImGui v1.91.9b, ImPlot v0.17, GLFW 3.4.

## Build and Run

### Git Bash

```bash
export PATH="/e/Compilers/mingw64/bin:$PATH"
./scripts/setup_toolchain.sh
./scripts/build.sh --test --smoke
```

### PowerShell

```powershell
$env:PATH = "E:\Compilers\mingw64\bin;$env:PATH"
powershell -ExecutionPolicy Bypass -File scripts\setup_toolchain.ps1
powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -Test -Smoke
```

### Command-Line Arguments

The executable accepts the following options:

```bash
build/DiskScope.exe --root "E:/Projects"          # Scan a specific directory on startup
build/DiskScope.exe --smoke                      # Run automated headless verification
build/DiskScope.exe --smoke --capture docs/captures # Headless run saving 6 view captures
build/DiskScope.exe --frames 4                   # Render N frames per view before capture
build/DiskScope.exe --help                       # Display usage information
```

Captured screenshots are saved to `docs/captures/`:
- `docs/captures/00-heap-playback.bmp`: Mid-playback frame of heap visualizer.
- `docs/captures/01-overview.bmp`: Dashboard overview and storage summary.
- `docs/captures/02-directory-explorer.bmp`: N-ary tree drill-down view.
- `docs/captures/03-largest-folders.bmp`: Heap-driven top-K folder ranking.
- `docs/captures/04-heap-visualizer.bmp`: Complete binary heap inspection view.
- `docs/captures/05-settings.bmp`: Application settings and scan configuration.

## Project Structure

```
DiskScope/
├── .tools/                 # Portable CMake 4.5.1 and Ninja 1.13.2 binaries
├── docs/                   # Assignment documentation, captures, and slides
│   ├── captures/           # Automated smoke test output BMPs (1440x900)
│   ├── dsa.md              # Core DSA assignment specification (15 deliverables)
│   ├── test-results.md     # Verification test matrix and reproducibility steps
│   ├── presentation.md     # Presentation slide deck outline
│   ├── ai-disclosure.md    # Transparent AI assistance disclosure
│   └── contributions.md    # Team contributions statement
├── scripts/                # Toolchain setup and build scripts (Bash + PowerShell)
├── src/
│   ├── core/               # FolderNode (N-ary tree), FileSystemScanner, SizeCalculator
│   ├── structures/         # MaxHeap (custom binary heap) and HeapEvent observer
│   ├── services/           # AnalysisEngine, ScanController, HeapDemoEngine
│   ├── platform/           # Windows file attributes, allocated sizes, folder picker
│   ├── ui/                 # Application shell, MainWindow, Theme, and 5 ImGui views
│   ├── utils/              # Formatting utilities for bytes and strings
│   └── visualization/      # HeapRenderer and ChartRenderer
└── tests/                  # Unit tests (test_heap.cpp, test_scanner.cpp, test_analysis.cpp)
```

## Data Structures and Algorithms

- **N-ary Directory Tree (`FolderNode`):** Hierarchical tree mirroring the filesystem. Inclusive sizes are computed via post-order depth-first traversal in $O(N)$ time and space, guaranteeing each directory's total includes all descendants.
- **Custom Max Binary Heap (`MaxHeap`):** Level-order array layout over a contiguous `std::vector<const FolderNode*>`. The heap forbids STL heap algorithms (`std::priority_queue`, `make_heap`, `push_heap`, `pop_heap`). Priority is determined by inclusive byte size, breaking ties lexicographically by path.
  - `getMax()`: $O(1)$ time, $O(1)$ space.
  - `insert()`: $O(\log D)$ time via `siftUp`, amortized $O(1)$ space.
  - `extractMax()`: $O(\log D)$ time via `siftDown`, $O(1)$ space.
  - `buildHeap()`: $O(D)$ time via bottom-up sift-down sequence, $O(1)$ auxiliary space.
  - `getTopK(k)`: $O(D + k \log D)$ time operating on an isolated heap copy, $O(D)$ space.

## Verification and Test Summary

- **Unit Test Suite (`build/diskscope_tests.exe`):** 23 cases, 192 checks, 0 skipped, 0 failed cases, 0 failed checks.
- **CTest Integration:** `1/1 Test #1: diskscope_tests ... Passed`.
- **Automated Smoke Run:** Generated fixture of 401 directories and 1,200 files scanned and captured in 1.14 seconds; project repository (≈9,000 files) scanned and captured in 4.21 seconds.
- **Off-Screen Pixel Sampling:** Validated all six 1440x900 24-bit BMP captures contain non-black rendered pixels.

## Limitations

- Symlinks and NTFS junctions are not followed to prevent cycles; link targets are skipped and counted.
- Hard links are accounted per directory entry rather than per inode/MFT record.
- Native folder dialog uses Windows COM `IFileDialog` with fallback to text input.
- Traversal depth is capped at 256; recorded errors are capped at 500 to bound memory.
- Cancelled scans return consistent lower-bound partial trees.
- Visualizer replay is capped at 63 nodes (6 levels) to ensure visual legibility and bounded frame memory.

## Documentation Links

- [DSA Specification and Deliverables](docs/dsa.md)
- [Test Suite Results and Matrix](docs/test-results.md)
- [Presentation Slide Outline](docs/presentation.md)
- [AI Assistance Disclosure](docs/ai-disclosure.md)
- [Team Contributions Matrix](docs/contributions.md)
- [Technical Architecture Specification](docs/architecture.md)

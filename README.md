# DiskScope

DiskScope is a high-performance C++17 disk-space analyzer for Windows featuring custom data structures: an N-ary directory tree with post-order inclusive size aggregation and a hand-written max binary heap for ranking folders. It includes an interactive GUI built with Dear ImGui and an algorithm visualizer that replays recorded frames of the real heap operations.

## Features

- Non-blocking recursive directory scanner running on a dedicated worker thread.
- Inclusive size accounting tracking both logical file lengths and on-disk allocated sizes.
- Custom max binary heap with deterministic tie-breaking (no STL heap algorithms).
- Top-K directory ranking and real-time substring search.
- Honest algorithm visualizer replaying frame-by-frame execution snapshots on heap copies.
- Desktop UI built on one design system: palette/typography tokens, a vector icon set, and a shared component layer (cards, buttons, switches, steppers, tables, dialogs, empty states) with hover, focus, disabled and loading states.
- Headless automated smoke test rendering off-screen BMP captures, with per-capture assertions that a frame actually rendered.

## Application Views

1. **Overview:** Storage summary cards, scan progress, a ranked bar chart, and a storage-share ring that partitions the scan root's children rather than overlapping nested folders.
2. **Directory Explorer:** N-ary tree drill-down with a filter, share meters per row, a breadcrumb trail, and a details pane covering sizes, counts and subfolders.
3. **Largest Folders:** Heap-driven ranking with top-K switch, name filter, rank badges, usage meters and per-row navigation into the Explorer.
4. **Heap Visualizer:** Playback of build, insert and extract operations with a fitted-label tree, level-order array strip, operation legend, node details and tree/array selection sync.
5. **Settings:** Scan options, ranking root exclusion, visualizer speed, the live palette reference (role, hex, usage) and known limitations.

## Design System

One palette drives every surface, chart and visualizer state, so the app never falls back to framework defaults:

| Role | Colour | Used for |
|---|---|---|
| Background | `#090B0A` | application canvas |
| Sidebar | `#0D100F` | navigation rail |
| Panel | `#111513` | cards, tables |
| Elevated | `#171C19` | inputs, meters, hover surfaces |
| Border | `#252D28` | hairlines and outlines |
| Primary green | `#16C76A` | actions, selection, healthy states |
| Green hover | `#20E879` | hover and comparison highlights |
| Dark green | `#103D27` | selected surfaces, heap root |
| Primary red | `#E5484D` | destructive actions, errors, swap |
| Dark red | `#471B1D` | destructive surfaces, extract |
| Text | `#F1F5F2` | primary copy |
| Secondary text | `#929B95` | captions and metadata |

There is deliberately no amber: in-progress states read as green and problem states as red. Icons are drawn as vector primitives (`ImDrawList`), so no icon font or asset dependency was added. Heap tree states are palette-only: root = dark green with a green ring, compare = double ring, swap = red fill, insert = green fill, extract = dark red with a red ring.

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
build/DiskScope.exe --size 1280x800              # Window / capture size (default 1440x900)
build/DiskScope.exe --dpi 1.5                    # UI scale override (0 = auto from the monitor)
build/DiskScope.exe --help                       # Display usage information
```

Captured screenshots are saved to `docs/captures/`:
- `docs/captures/00-heap-playback.bmp`: Mid-playback frame showing a real compare/swap step, with the matching slots tinted in tree and array.
- `docs/captures/01-overview.bmp`: Overview cards, ranked bar chart and storage-share ring.
- `docs/captures/02-directory-explorer.bmp`: N-ary tree drill-down with the details pane open on the largest folder.
- `docs/captures/03-largest-folders.bmp`: Heap-driven top-K ranking with usage meters.
- `docs/captures/04-heap-visualizer.bmp`: Complete binary heap with a selected node and synced array slot.
- `docs/captures/05-settings.bmp`: Settings sections and the palette reference.

Batch captures are deterministic: they always render at DPI scale 1.0 so the images are comparable across machines, while interactive runs follow the monitor's content scale. The window is clamped by the OS to the monitor work area, so an oversized `--size` reports the actual framebuffer rather than silently passing.

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
│   ├── ui/                 # Application shell, Theme tokens, Icons, Components, views
│   ├── utils/              # Formatting utilities for bytes and strings
│   └── visualization/      # HeapRenderer (tree + array strip) and ChartRenderer (bars, ring)
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

- **Unit Test Suite (`build/diskscope_tests.exe`):** 23 cases, 195 checks, 0 skipped, 0 failed cases, 0 failed checks.
- **CTest Integration:** `1/1 Test #1: diskscope_tests ... Passed`.
- **Automated Smoke Run:** Generated fixture of 401 directories and 1,200 files scanned and captured in 1.14 seconds; project repository (≈9,000 files) scanned and captured in 4.21 seconds.
- **Off-Screen Pixel Sampling:** every capture is analysed before it is accepted: distinct quantised colours, non-background pixel ratio and a toolbar-band colour count, so a blank or half-rendered frame fails the run instead of passing silently. Verified at 1440x900, 1280x800 and 1920x1080 (clamped to the work area, reported) — see `docs/test-results.md`.

## Limitations

- Symlinks and NTFS junctions are not followed to prevent cycles; link targets are skipped and counted.
- Hard links are accounted per directory entry rather than per inode/MFT record.
- Native folder dialog uses Windows COM `IFileDialog` with fallback to text input.
- Traversal depth is capped at 256; recorded errors are capped at 500 to bound memory.
- Cancelled scans return consistent lower-bound partial trees.
- Visualizer replay is capped at 63 nodes (6 levels) to ensure visual legibility and bounded frame memory.
- The UI scale is sampled at startup and re-applied when the monitor content scale changes during an interactive session; per-monitor DPI changes made while the window is hidden are not tracked.
- Capture runs force DPI scale 1.0 so the committed screenshots stay comparable; they document layout, not the user's monitor scaling.

## Documentation Links

- [Beginner Build Guide (Windows)](BUILD_GUIDE.md)

- [DSA Specification and Deliverables](docs/dsa.md)
- [Test Suite Results and Matrix](docs/test-results.md)
- [Presentation Slide Outline](docs/presentation.md)
- [AI Assistance Disclosure](docs/ai-disclosure.md)
- [Team Contributions Matrix](docs/contributions.md)
- [Technical Architecture Specification](docs/architecture.md)
- [Refactoring Report and Ledger](docs/refactoring-report.md)

# DiskScope — Architecture

> Status: describes the frozen interface contracts in `src/**.h` and `CMakeLists.txt`.
> The review findings raised against the first draft are recorded, with their resolution, in **§13 Resolved decisions**.

---

## 1. Purpose and scope

DiskScope is a C++17 Windows desktop app that scans a directory, shows where the storage goes, and ranks the largest folders.
It is a university DSA assignment, so the GUI (Dear ImGui v1.91.9b + ImPlot v0.17 on GLFW 3.4 / OpenGL 3) is the delivery vehicle, **not** the assessed artefact.

**Why the DSA is the point.** The assessed work is two hand-built structures and the algorithms over them: an **N-ary tree** (`FolderNode`) that mirrors the directory hierarchy and holds inclusive sizes computed by post-order traversal, and a **custom max binary heap** (`MaxHeap`) that ranks folders by inclusive size with a deterministic tie-break, built bottom-up in O(D). Everything else exists to feed these structures (the scanner), query them (`AnalysisEngine`), or *show them honestly* (the heap visualizer replays frames recorded from the real heap code). No library heap is used anywhere (`std::priority_queue`, `std::make_heap`/`push_heap`/`pop_heap` are banned).

In scope: one scan root at a time, logical + allocated size, top-k rankings, name search, tree exploration, heap playback.
Out of scope: see §12.

---

## 2. Layered architecture

```
 ┌──────────────────────────── DiskScope.exe (src/main.cpp) ───────────────────────────┐
 │  diskscope_ui  (links imgui, implot, glfw, OpenGL; Win32: ole32 uuid shell32)        │
 │   Design system: ui/Theme (palette + metric/typography tokens), ui/Icons (vector     │
 │   icon set), ui/Components (cards, buttons, switches, steppers, tables, dialogs)     │
 │   Shell: ui/Application, MainWindow (sidebar, toolbar, status row, view host)        │
 │   Views: Dashboard | DirectoryExplorer | LargestFoldersView | HeapVisualizerView |   │
 │          SettingsView          visualization/HeapRenderer, ChartRenderer             │
 │   platform/NativeFolderDialog (IFileDialog, guarded fallback)                        │
 └───────────────▲──────────────────────────────────────────────▲──────────────────────┘
                 │ const FolderNode*, AnalysisStats,            │ vector<HeapEvent>
                 │ ScanProgress, optional<ScanResult>           │ (playback frames)
 ┌───────────────┴──────────────────── diskscope_core (NO GUI) ─┴──────────────────────┐
 │ services/ScanController ──(worker thread)──► core/scanDirectory                      │
 │        │ takeResult() (UI thread)                │ std::filesystem DFS               │
 │        ▼                                         ▼                                   │
 │ services/AnalysisEngine ◄── ScanResult{ unique_ptr<FolderNode> root, errors, ... }   │
 │        │ owns tree + index + MaxHeap             core/FolderNode  (N-ary tree)       │
 │        ▼                                         core/accumulateSizes (post-order)   │
 │ structures/MaxHeap  ◄──── IHeapObserver ──── services/HeapDemoEngine (copy + record) │
 │ utils/format::*   platform/allocatedSizeOf                                           │
 └──────────────────────────────────────────────────────────────────────────────────────┘
```

Data flow: `std::filesystem` scan → N-ary tree (inclusive sizes) → `MaxHeap` (bottom-up build over non-root folders) → `AnalysisEngine` queries → `ScanController` handoff → ImGui views → `HeapDemoEngine` records → `HeapVisualizerView` playback.

**Dependency rule.** Nothing in `src/core`, `src/structures`, `src/services`, `src/utils` or `src/platform/AllocatedSize.*` includes an ImGui, ImPlot, GLFW or OpenGL header. `diskscope_core` links no GUI library. The build enforces this: `diskscope_tests` links **only** `diskscope_core`, so any GUI symbol leaking into core fails the test link. Dependencies point downward only. `ui` may include core headers; core never includes `ui/`, `visualization/` or `platform/NativeFolderDialog.h`.

---

## 3. Memory ownership model

| Object | Owner | Held by others as | Notes |
|---|---|---|---|
| `FolderNode` (root) | `ScanResult::root` (`unique_ptr`) | — | moved worker → `ScanController::pending_` → `AnalysisEngine::result_` |
| `FolderNode` (child) | parent's `children` (`vector<unique_ptr>`) | `const FolderNode*` | destroyed exactly once, with its parent |
| id index | `AnalysisEngine::index_` (`unordered_map<NodeId, FolderNode*>`) + `discoveryOrder_` | non-owning | rebuilt by `setResult` |
| ranking heap | `AnalysisEngine::heap_` (`vector<const FolderNode*>`) | non-owning | rebuilt by `rebuildRankings` |
| heap frames | `HeapVisualizerView` (`vector<HeapEvent>`, each with a `vector<const FolderNode*>` snapshot) | non-owning | must be discarded on rescan |
| view selections | views (`const FolderNode*` / `NodeId`) | non-owning | must be reset on rescan |

- **Why addresses are stable:** each node is a separate heap allocation reached through `unique_ptr`. Growing `children` moves the `unique_ptr`s, not the nodes. `FolderNode` is non-copyable, and the tree is never changed after `scanDirectory` returns. Raw pointers stay valid for as long as the tree lives.
- **Lifetime guarantee:** the tree outlives every structure that points into it. `AnalysisEngine` declares `result_` before `heap_`, so members are destroyed in reverse order and the heap goes first. Views read only through `const AnalysisEngine&`.
- **Why no `shared_ptr`:** ownership is strictly hierarchical and single-threaded once handed over. `shared_ptr` would add refcount traffic for millions of nodes, hide the lifetime rule, and allow cycles. `unique_ptr` + documented non-owning observers is the honest model.
- **What invalidates what:** a rescan installs a whole new tree. On the UI thread, within **one frame**: (1) views drop cached pointers and heap frames; (2) `engine.setResult(std::move(*r))` clears the heap and index (see F2), destroys the old tree and indexes the new one; (3) `engine.rebuildRankings(includeRoot)`. No frame ever sees a new tree with an old heap. While a rescan runs, the **old** tree remains valid and displayed. The worker builds a separate tree that nobody else touches.

---

## 4. Filesystem scanning algorithm (`scanDirectory`)

```
scanDirectory(rootPath, options, onProgress, isCancelled):
  root = new FolderNode{id=1, path=absolute(rootPath).lexically_normal()}
  visit(root, depth=0)
  return ScanResult{root, errors, skippedSymlinks, unreadableFiles, cancelled}

visit(node, depth):
  if cancelled: return
  if depth > options.maxDepth: record {node.path, "maximum scan depth reached"}; node.accessible=false; return
  if !visited.insert(realPathOf(node.path)): record cycle error; return  // guard
  it = directory_iterator(node.path, ec)                          // error_code overload
  if ec: errors += {node.path, ec.message()}; node.accessible=false; return
  for entry in it (it.increment(ec) each step):
     if ++entries % N == 0: onProgress(snapshot); if isCancelled(): cancelled=true; break
     st = entry.symlink_status(ec)                               // NEVER status()
     if is_symlink(st) or isReparsePoint(entry): ++skippedSymlinks; continue
     if hidden(entry) and !options.includeHidden: continue
     if is_directory(st): child = new FolderNode{id=nextId++, ...}; visit(child, depth+1)
     else if is_regular_file(st):
        sz = entry.file_size(ec); if ec: ++unreadableFiles; errors+=...; node.accessible=false; continue
        node.ownFileBytes += sz; node.fileCount++ ; alloc += measureAllocated ? allocatedSizeOf(path, sz) : sz
     // other types (devices, sockets) ignored
  // post-order aggregation (same math as accumulateSizes)
  node.totalBytes     = ownFileBytes + Σ child.totalBytes
  node.allocatedBytes = ownAllocated + Σ child.allocatedBytes
  node.fileCount      = ownFiles     + Σ child.fileCount
  node.folderCount    = Σ (1 + child.folderCount)
```

- **DFS order / ids:** ids are assigned in pre-order discovery (root = 1, `kInvalidNodeId = 0`), so `discoveryOrder_[id-1]` is O(1).
- **Post-order aggregation:** each directory's inclusive totals are final only after all children return, so every node is summed exactly once (O(D + F)). `accumulateSizes(FolderNode&)` is the standalone, unit-tested version. Note: `SubtreeTotals::folders` **includes** the node, while `FolderNode::folderCount` counts **descendants only** (`folderCount = totals.folders - 1`).
- **Permission denied:** `directory_options::skip_permission_denied` exists to stop the walk throwing. But with that flag an unreadable directory looks empty and no error is reported (F3). The recorded design uses the `error_code` overloads, and every failure becomes a `ScanError` plus `accessible = false`. The walk never aborts.
- **Symlinks/junctions:** the type is read with `symlink_status`, and links are **never followed**. Each skipped link adds 1 to `skippedSymlinks`. NTFS junctions and mount points are reparse points that libstdc++ may not report as symlinks, so a Win32 attribute check is needed (F4).
- **Cycle guard:** a `std::unordered_set` of real directory paths visited during this scan. With links never followed, cycles shouldn't occur. The guard backs up the cases where link detection fails (e.g. the profile's `Application Data` loop) at the cost of one lookup per directory.
- **Hidden entries:** `ScanOptions::includeHidden` (default `true`). When false, hidden files and dot-files are excluded from all totals, and hidden directories are not entered. The Dashboard must say "hidden excluded".
- **Errors:** collected into `ScanResult::errors` (path + message). Unreadable file sizes are counted in `unreadableFiles` and never estimated. The list is capped at `ScanOptions::maxErrors` (500); further problems only increment `ScanResult::omittedErrors`, so scanning `C:\` cannot exhaust memory.
- **Progress:** every `progressInterval` (default 256) entries, `onProgress(ScanProgress{folders, files, bytes, current})`. The controller copies it under its mutex.
- **Cancellation:** checked at the same checkpoints. When set, each active `visit` breaks out of its loop, **still runs its post-order aggregation**, and returns. The result is a partial tree with `cancelled = true`.
- **Partial-result semantics:** the tree is internally consistent (each node's total = its own bytes + its children's totals), but the totals are **lower bounds** on the real usage. Directories not yet visited are missing entirely, and there is no per-node "complete" flag.
- **Logical vs allocated size:** `totalBytes` adds up `file_size` (logical length). `allocatedBytes` adds up `allocatedSizeOf(file, logical)`, which reads `GetFileInformationByHandleEx(FileStandardInfo).AllocationSize` on Windows. That figure reflects cluster round-up, NTFS compression and sparse files. If it returns 0 the logical size is used (F5). With `measureAllocatedSize = false`, allocated = logical. **Rankings use `totalBytes` only.**

---

## 5. Storage accounting rules

1. **Inclusive sizes:** `totalBytes(n) = ownFileBytes(n) + Σ totalBytes(child)`.
2. **Never add a parent and its descendant** to get a total. The scan total is `root->totalBytes`, full stop. A top-k list may contain nested folders (e.g. `Users` and `Users\Me`), and the UI must not sum them.
3. **The root is excluded from rankings** by default (`rebuildRankings(includeRoot=false)`, `AnalysisStats::rootExcludedFromRankings`). It is trivially the largest. Corollary: the largest non-root folder is always a **direct child** of the root.
4. **Bytes are `std::uintmax_t`** (64-bit), from the scanner to the heap key. No floats and no 32-bit counters.
5. **Formatting happens only at display time**, through `diskscope::format::*`. It uses 1024-based units with KB/MB/GB labels, matching Explorer (`byteSize(1536) == "1.50 KB"`).
6. **Each directory is counted once per scan**: post-order, one node per directory, cycle guard.
7. **Symlinks/junctions are not followed and contribute 0 bytes.** They are counted in `skippedSymlinks`. Mounted volumes behind junctions are therefore not included.
8. **Hard links are counted once per link name.** Multiply-linked files (e.g. WinSxS) are over-counted relative to real disk use. This is documented, not corrected.
9. **`subst` drives and alternate paths** to the same data are separate scans. Totals from two scans must not be added together. Alternate data streams are not included in the logical size.
10. **Unreadable content contributes 0 and is reported.** `accessible = false`, `errors`, `unreadableFiles`, and `cancelled` make the incompleteness visible. Sizes are never estimated.

---

## 6. Custom max binary heap (`structures/MaxHeap`)

**Layout:** `std::vector<const FolderNode*> items_`, a 0-based complete binary tree in level order:
`parentOf(i) = (i-1)/2`, `leftOf(i) = 2i+1`, `rightOf(i) = 2i+2`.

**Priority:** `MaxHeap::higherPriority(lhs, rhs)` is `lhs->totalBytes > rhs->totalBytes`, or equal sizes with `lhs->path < rhs->path`. Paths are unique within a scan, so this is a **strict total order** and rankings are fully deterministic. `path::compare` is component-wise, so an ancestor always beats a same-sized descendant (a parent folder holding only one subfolder ranks above it).

**Operations**
- `siftUp(i)`: while `i > 0` and `higherPriority(items[i], items[parentOf(i)])`, swap and set `i = parentOf(i)`.
- `siftDownFrom(i)` / `heapify(i)`: let `m` be the highest-priority of `{i, leftOf(i), rightOf(i)}` that are within bounds. If `m == i`, stop. Otherwise swap and set `i = m`.
- `buildHeap(items)`: take ownership of the array, then `heapify(i)` for `i = n/2 - 1` down to `0`.
- `insert(node)`: `push_back`, then `siftUp(n-1)`.
- `extractMax()`: save `items[0]`, move `items[n-1]` to slot 0, `pop_back`, `heapify(0)`, return the saved item (nullptr if the heap was empty).
- `getMax()`: `items[0]` or nullptr.
- `getTopK(k) const`: copy `items_` into a local heap **without an observer**, extract `min(k, size())` times, return them in descending order. The live heap is not modified, and `k > D` is legal.

**Why bottom-up build is O(D):** at most ⌈D/2^(h+1)⌉ nodes have height h, and sifting one costs O(h). The total is D·Σ h/2^(h+1) ≤ D, so O(D). By contrast, D inserts cost O(D log D).

**Invariants**

| # | Invariant | Checked by |
|---|---|---|
| I1 | Shape: `items_[0..n-1]` is contiguous, so it is a complete tree | by construction |
| I2 | Order: for all `i > 0`, `!higherPriority(items[i], items[parentOf(i)])` | `validate()` |
| I3 | No null entries | `validate()` |
| I4 | Each `FolderNode` appears at most once | tests (set of pointers) |
| I5 | Pointees outlive the heap | ownership model (§3) |
| I6 | The heap never owns or frees its pointees | type (`const FolderNode*`) |

**Complexity** (D = indexed folders)

| Op | Time | Extra space |
|---|---|---|
| `getMax` | O(1) | O(1) |
| `insert` | O(log D) | amortised O(1) |
| `extractMax`, `heapify` | O(log D) | O(1) |
| `buildHeap` | O(D) | O(1) beyond the input |
| `getTopK(k)` | O(D + k log D) | O(D) copy |

**Sift-down correctness (loop invariant).** Precondition: the subtrees under `leftOf(i)` and `rightOf(i)` are heaps. Invariant at the top of each iteration, with current slot `j`: (a) every slot in subtree(i) except `j` satisfies I2 with respect to its children; (b) if `j ≠ i`, `items[parentOf(j)]` beats both children of `j`.
*Initialisation:* `j = i`. (a) holds by the precondition, and (b) is vacuous.
*Maintenance:* let `m` win among `{j, l, r}`. If `m = j`, then `j` satisfies I2, and with (a) the whole subtree is a heap. Otherwise swap `j` and `m`. Slot `j` now holds the winner of three, so it satisfies I2. The old `items[m]` beat `m`'s children (they were in a heap), and it now sits at `parentOf(m)`, so (b) holds for `m`. Every other slot is unchanged, so (a) holds with `j := m`.
*Termination:* `j` at least doubles each step, so there are at most ⌊log₂ n⌋ iterations. On exit, I2 holds throughout subtree(i).

---

## 7. Event recording and the honest visualizer

- **Hook:** `MaxHeap(IHeapObserver* observer = nullptr)`. The private `record(kind, a, b, note)` builds a `HeapEvent{kind, a, b, array = items_ (full snapshot after the step), note}` and calls `observer->onHeapEvent`. With a null observer `record` returns immediately, and call sites build the note string only when `observer_` is set, so the production heap pays one predictable branch per step.
- **Event kinds** (`HeapEventKind`):
  - build: `Build` (initial array), `SiftDown`, `Compare`, `Swap`, …, `Done`
  - insert: `Insert` (a = new slot), `Compare`, `Swap`, `SiftUp`, …, `Done`
  - extract: `Extract` (a = 0 after the last item moves to the root), `SiftDown`, `Compare`, `Swap`, …, `Done` (frame showing the new root)
- **`HeapDemoEngine`** (all methods const): each `recordX` creates a local recorder (an `IHeapObserver` that appends to a `vector<HeapEvent>`) and a local `MaxHeap(&recorder)`, then runs the **real** operation on a **copy** of the items. `recordInsert`/`recordExtract` first load the copy through `buildHeap` (an already-valid heap causes no swaps) and **discard those pre-roll frames** (F12).
- **Real items:** `selectDemoItems(engine.heap().data(), 63)` takes the first 63 slots in level order of the live heap. That is a complete prefix, so it is still a valid heap of real folders. It is *not* necessarily the top 63. For `recordBuild`, the same nodes are put in a deliberately non-heap order (e.g. discovery order) so the build has work to show.
- **63-node cap** (`kDefaultDemoCap`): 63 = 2⁶−1 nodes, six full levels, readable on screen. It also bounds memory: each frame is an O(n) snapshot, so an uncapped build over D folders would need O(D²) memory.
- **Playback** (`HeapVisualizerView` + `HeapRenderer`): state is `frames`, `frameIndex`, `playing`, `fps` and an accumulator advanced by `ImGui::GetIO().DeltaTime`. Controls are build/insert/extract demos, play/pause, step +1, reset (`frameIndex = 0`) and an fps stepper. Frame `f` draws the tree and the level-order array strip from `frames[f].array`, tints slots `a`/`b` by kind, and shows `note`. The live heap is never read during playback.
- **Presentation over the frames:** the recording is the ground truth; the view only animates the transition between two frames (`animT`, ~180 ms). A `Swap` frame slides the two exchanged items between their slot centres, an `Insert` frame grows the new node, and every state is expressed with palette tones only: root = dark green fill + green ring, `Compare` = extra ring in the hover green, `Swap` = red fill, `Insert` = green fill, `Extract` = dark red fill + red ring. Clicking a node selects it, rings it and highlights the matching array cell (and vice versa); the detail line shows name, slot, parent slot, size and an entry point back to the Directory Explorer.
- **Offline capture hook:** `MainWindow::requestHeapDemo` forwards to `HeapVisualizerView::requestDemo(demo, pauseAtFirstHighlight)`, which queues a demo and stops on the first recorded operation frame, so `00-heap-playback.bmp` shows a real compare/swap step instead of a second copy of the static heap.
- **No second heap.** There is no second or "fake" heap implementation anywhere: no animation-only sift code and no scripted frames. Every frame is a by-product of `MaxHeap::siftUp` / `siftDownFrom` running on real `FolderNode*`s. A reviewer can check this by grepping for swap logic outside `MaxHeap.cpp`.

---

## 8. Threading model

| Thread | Owns | May call |
|---|---|---|
| UI (main) | GLFW/ImGui context, `AnalysisEngine` (tree + index + heap), views, demo frames | everything in `diskscope_ui`, `ScanController` public API |
| Worker (1, `ScanController::worker_`) | the tree under construction | `scanDirectory`, `ScanController` private sinks |

- **Atomics:** `cancelRequested_` (set by `requestCancel()`, read by the scanner's `CancelFn`) and `running_` (acquire/release).
- **Mutex `mutex_`** protects `progress_` (copied out by `progress()`), `pending_` (`optional<ScanResult>`), `error_` and `root_`.
- **Handoff order:** the worker stores `pending_`/`error_` under the lock, **then** does `running_.store(false, release)`. Each frame the UI calls `takeResult()`, which moves the result out exactly once and leaves `pending_` empty.
- **`start()`** returns false if a scan is running or the path isn't an existing directory (`lastError()` explains). Before launching it must `join()` a finished worker, because assigning to a joinable `std::thread` calls `std::terminate`. It clears `cancelRequested_`, sets `running_ = true` **before** spawning, and resets progress.
- **`run()`** catches every exception (`filesystem_error`, `bad_alloc`) and turns it into `error_`. No exception escapes the thread.
- **No GUI calls off the main thread.** The worker never touches ImGui, GLFW, OpenGL, the folder dialog or `AnalysisEngine`.
- **`~ScanController()`:** `requestCancel()`, then `join()` if joinable. Any pending result is destroyed with the controller. App shutdown order: stop the controller, destroy views, destroy the engine, shut down ImGui/GLFW.

---

## 9. Module / interface contracts

| Header | Responsibility | Thread | Consumers | Boundary signatures |
|---|---|---|---|---|
| `core/FolderNode.h` | N-ary tree node, inclusive sizes | built on worker, read on UI | all | `struct FolderNode{ NodeId id; std::string name; path path; uintmax_t ownFileBytes,totalBytes,allocatedBytes,fileCount,folderCount; bool accessible; vector<unique_ptr<FolderNode>> children; }` |
| `core/FileSystemScanner.h` | recursive DFS scan | worker (thread-agnostic) | `ScanController`, tests | `ScanResult scanDirectory(const path& rootPath, const ScanOptions& = {}, ProgressFn = {}, CancelFn = {});` `ProgressFn = function<void(const ScanProgress&)>`, `CancelFn = function<bool()>` |
| `core/SizeCalculator.h` | post-order aggregation | any | scanner logic, tests | `SubtreeTotals accumulateSizes(FolderNode& node);` |
| `structures/MaxHeap.h` | custom max heap | UI (production), demo copies | `AnalysisEngine`, `HeapDemoEngine`, tests | `buildHeap(vector<const FolderNode*>)`, `insert`, `getMax`, `extractMax`, `heapify(size_t)`, `getTopK(size_t) const`, `data()`, `validate()`, `static higherPriority(lhs, rhs)` |
| `structures/HeapEvent.h` | step record + observer | UI | `MaxHeap`, `HeapDemoEngine`, `HeapVisualizerView` | `struct HeapEvent{kind,a,b,array,note}`; `virtual void IHeapObserver::onHeapEvent(const HeapEvent&)` |
| `services/AnalysisEngine.h` | owns the result; top-k, search, lookup, stats | UI only | all views | `setResult(ScanResult)`, `clear()`, `rebuildRankings(bool includeRoot=false)`, `topK(size_t) const`, `search(const string&, size_t limit=20) const`, `find(NodeId) const`, `pathTo(const FolderNode*) const`, `stats() const`, `heap() const`, `result() const`, `hasResult() const`, `generation() const` |
| `services/ScanController.h` | worker lifecycle + handoff | API: UI; `run()`: worker | `Application`/`MainWindow`, `Dashboard` (progress) | `bool start(const path&, const ScanOptions& = {})`, `requestCancel()`, `running()`, `progress()`, `optional<ScanResult> takeResult()`, `lastError()` |
| `services/HeapDemoEngine.h` | record real ops on copies | UI | `HeapVisualizerView` | `static selectDemoItems(const vector<const FolderNode*>&, size_t cap=63)`, `recordBuild(items) const`, `recordInsert(existing, node) const`, `recordExtract(existing) const` → `vector<HeapEvent>` |
| `utils/Formatting.h` | the only bytes→text conversion | UI | views, charts | `format::byteSize`, `byteSizeCompact`, `count`, `percent(double,int=1)`, `relativePath(path, base)`, `ellipsize(string,size_t)`, `toLower(string)` |
| `platform/AllocatedSize.h` | on-disk size per file | worker | scanner | `std::optional<uintmax_t> allocatedSizeOf(const path& file, uintmax_t logicalSize);` |
| `platform/FileAttributes.h` | reparse-point / hidden attribute checks | worker | scanner | `bool isReparsePoint(const path&)`, `bool isHiddenEntry(const path&)` |

Engine query notes:
- `topK` delegates to `heap_.getTopK`. It copies O(D) per call, so views **cache** results and recompute only when the result or k changes, not every frame.
- `search` filters names in O(D), lower-cased ASCII via `format::toLower`, then orders matches largest-first with `MaxHeap` (`buildHeap` + `limit` extractions), so search and rankings share one comparator. Results are cached per search term.
- `pathTo` has no parent link to follow. It walks down from the root, choosing the child whose path is a prefix of the target: O(depth × fan-out) (F6).
- `stats().largestFolder` is the best direct child of the root, which holds whether or not the root is included in rankings.
- `stats()` is derived state (`result_`, `heap_.size()`, `includeRoot_`) and the views ask for it several times per frame, so it is computed once and invalidated by `setResult`, `clear` and `rebuildRankings`; callers are unchanged because the signature still returns by value.

UI views (five): **Dashboard** (stat cards, scan progress, warnings, ImPlot bar chart plus a custom storage-share ring via `ChartRenderer`), **DirectoryExplorer** (tree with filter and share meters, breadcrumb and subfolder table), **LargestFoldersView** (`topK`/`search` with rank badges and usage meters), **HeapVisualizerView** (`frames` playback with tree/array sync and node details), **SettingsView** (`ScanOptions`, includeRoot, demo fps, live palette reference).

UI shell and design system:
- **`ui/MainWindow`** owns the three chrome regions: the sidebar (brand mark, five icon nav rows with active indicator, footer status with the Clear-results action), the toolbar (inline path field with folder glyph and in-field clear, Select Folder / Scan / Cancel / Rescan, then a divider-separated status row with an indeterminate progress bar while scanning) and the view host. The host never scrolls; each view owns its own scrolling region.
- **`ui/Theme`** is the single source of truth: palette tokens (12 named roles), metric tokens (`sidebarWidth`, `cardPadding`, `cardRadius`, `controlRadius`, `gap`, `navRowHeight`, `tableRowHeight`, `iconSize`), a seven-step typography scale and the chart ramp. Views must not hardcode colours or pixel paddings.
- **`ui/Icons`** draws 32 outline icons as `ImDrawList` primitives. An icon font was rejected: Segoe Fluent glyph codepoints vary across Windows builds and would fail silently as tofu, while vector primitives are deterministic and themeable.
- **`ui/Components`** holds the state-complete widgets (cards with optional auto-height, stat cards, four button roles × three sizes with disabled/loading, nav items, search inputs, segmented control, progress bar, bar meter, tooltip, key-value rows, table wrappers plus right-aligned text/header helpers, switches with an eased knob, numeric steppers, confirm dialog, empty states; `ui/Icons` adds the icon button). Every interactive widget implements default, hover, active, focus, disabled and — where it applies — loading.
- **Accent semantics:** no amber exists in the palette. Positive/active/in-progress reads green, destructive/error/partial reads red, informational reads secondary text. `printf`-style state strings use ASCII dashes only, so no glyph resolves to `?`.
- **Breakpoints:** stat cards 4-across above 860 px and 2×2 below; Overview charts side-by-side above 880 px; Settings two columns above 880 px; heap controls on one row above 824 px (two rows below); the Directory Explorer split keeps a 280 px tree pane. The minimum window is 1024×640 (`glfwSetWindowSizeLimits`).

---

## 10. Error handling and edge cases

| Case | Behaviour | UI must show |
|---|---|---|
| Empty directory | root with all zeros, no children; heap empty when the root is excluded | "No subfolders to rank" |
| Empty scan (`ScanResult::empty()`, root null: start failed or exception) | engine not updated; `lastError()` set | error message; previous result kept |
| Permission denied (directory) | `ScanError` + `accessible=false`; directory counted as 0 | error count, ⚠ marker on the node, "totals incomplete" banner |
| Unreadable file size | `unreadableFiles++`, not counted, parent `accessible=false` | count in Dashboard |
| Very deep trees | recursion depth is capped by `ScanOptions::maxDepth` (default 256); deeper branches are truncated and recorded as a `ScanError`. `~FolderNode` and `accumulateSizes` recurse to the same depth | error entry for truncated branches |
| Path > 260 chars | without `\\?\` / longPathAware, the iterator returns an error, which is recorded and the walk continues | errors list |
| Non-ASCII names | `name` must be UTF-8 (`u8string()`) for ImGui; `toLower` affects ASCII only (F9) | — |
| Sizes > 2³² bytes | `uintmax_t` throughout; `format::byteSize` handles TB | — |
| Duplicate sizes | strict tie-break by path, so the order is deterministic | — |
| `k > D` | `getTopK` returns `D` items | "showing D of k" |
| `k == 0` | returns empty | — |
| Cancelled mid-walk | partial, consistent tree, `cancelled=true` | persistent "Scan cancelled — partial results (lower bounds)" banner |
| Rescan after changes | entirely new tree/index/heap (§3); old pointers, frames and selections dropped; selection restored by **path**, never by `NodeId` (ids are per scan) | — |
| Root is a symlink/junction | followed once because the user chose it explicitly; never followed below the root (F13) | — |
| Worker exception | caught in `run()`, `lastError()` set | error toast |

Any of `cancelled`, `errorCount > 0`, `unreadableFiles > 0` or `!includeHidden` means **incomplete**. The Dashboard must state this next to the total and never present the number as exact.

---

## 11. Verification strategy

- **Harness:** `diskscope_tests` (`tests/test_main.cpp` custom harness + `test_heap.cpp`, `test_scanner.cpp`, `test_analysis.cpp`) is registered as the single CTest test `diskscope_tests` (TIMEOUT 900 s). It links only `diskscope_core`, which also proves the dependency rule. Run it with `scripts/build.ps1 -Test`.
- **Heap tests:** `validate()` after every `insert`/`extractMax`/`buildHeap`, on random and adversarial inputs (sorted, reverse-sorted, all-equal sizes, D = 0/1/2). `heapify(i)` on a subtree with only the root out of place. Tie-break asserted directly with `higherPriority`. `getTopK` leaves `data()` unchanged. `k > D`. Pointer uniqueness (I4).
- **Ranking cross-check:** for random fixture sets, the output of `getTopK(k)` and repeated `extractMax` must exactly match `std::sort` with `higherPriority` followed by the first k. `std::sort` is used **only as a test oracle**.
- **Scanner tests:** fixture trees created **at runtime** in `std::filesystem::temp_directory_path()/diskscope_test_<unique>` and deleted afterwards. Known file sizes check own/inclusive totals, counts, ids (root = 1) and the parent = own + Σ children identity at every node. Empty directories. `includeHidden`. Cancellation through a `CancelFn` that returns true after N checks (expects `cancelled` and a consistent partial tree). Symlink fixtures are **skipped with a message** when `create_directory_symlink` fails (no Developer Mode or admin).
- **Analysis tests:** root excluded/included, `stats()` fields, `find`, `pathTo`, `search` ordering and case-insensitivity, `largestFolder` is a root child, `setResult` replacing a previous result.
- **Demo tests:** every `HeapEvent::array` from `recordBuild`/`recordExtract` ends in a frame that passes `validate()`-equivalent checks. Frame counts are greater than zero. The live heap is unchanged.
- **Visual verification:** `DiskScope.exe --smoke --capture <dir> [--frames N] [--size WxH] [--dpi S]` renders the five views off-screen and exits, so rendering can be checked without touching the desktop or the user's folders:
  - scan target: a **generated fixture tree** written under the temp directory (deterministic names and sizes: nested folders, a zero-byte file, a duplicate-size pair, an empty folder), unless `--root <path>` overrides it;
  - output: `01-overview.bmp` … `05-settings.bmp` plus `00-heap-playback.bmp` written as 24-bit BMP at the requested `--size`;
  - assertions: `MaxHeap::validate()` on the built heap, plus the scan summary (bytes, files, folders, errors) and the top five ranked folders printed to stdout for eyeball comparison;
  - **render assertions per capture:** quantised distinct-colour count, non-background pixel ratio and a toolbar-band colour count, so a frame that failed to render fails the run instead of producing a plausible-looking filename. Batch mode also prints `SMOKE FRAMEBUFFER` and a `SMOKE NOTE` when the OS clamps the requested size to the monitor work area;
  - **deterministic scale:** batch runs default to DPI 1.0 so captures are comparable across machines; `--dpi` overrides it and the interactive loop follows `glfwGetWindowContentScale`, rebuilding fonts when it changes;
  - **honest playback capture:** before the `00-heap-playback.bmp` frame the harness queues a real `Build heap` demo and pauses on the first recorded operation frame, and it selects a heap slot so `04-heap-visualizer.bmp` shows the node detail line and the tree/array selection sync;
  - **deterministic selections:** the Directory Explorer frame pre-selects the largest ranked folder, because a details pane cannot be clicked headlessly;
  - exit code 0 only when every assertion holds and every file was written; non-zero with the first failure printed to stderr otherwise;
  - `docs/captures/` is git-ignored; the images are for the developer and the grader, not committed artefacts.
- **Viewport coverage:** captures are verified at 1440×900 (canonical), 1280×800 and 1920×1080 so the responsive breakpoints are exercised, not just asserted.

| Requirement | Evidence |
|---|---|
| N-ary tree with inclusive sizes | `FolderNode`, `accumulateSizes`, scanner tests |
| Custom heap: insert / extractMax / heapify / buildHeap / getTopK | `MaxHeap`, heap tests, `std::sort` cross-check |
| No library heap | grep for `priority_queue|make_heap|push_heap|pop_heap` finds nothing |
| Honest visualization | §7, demo tests, Heap Visualizer capture |
| Robust scanning (permissions, links, cancel) | scanner tests, Dashboard banners |
| Responsive GUI | worker thread (§8), smoke captures at three viewport sizes |
| Single palette, no framework defaults | §9 design system; every `ImPlotCol_*` slot is assigned from the theme |

---

## 12. Risks, limitations, non-goals

**Risks/limitations**
- Hard links, `subst` drives and cloud placeholders distort totals (§5).
- libstdc++ (MinGW GCC) may not detect junctions or hidden attributes (F4). `std::thread` needs a MinGW build with working C++11 threads.
- `getTopK` copies O(D) per call, which is a performance risk if views call it every frame.
- The `errors` vector is capped at 500 entries; the overflow count is shown in the Dashboard.
- A slow network share can block inside one `directory_iterator` step, so cancellation only takes effect at the next checkpoint.
- Opening every file for `allocatedSizeOf` roughly doubles the per-file I/O cost. `measureAllocatedSize=false` is the escape hatch.
- UI scale is sampled at startup and re-applied when the interactive monitor content scale changes; a scale change while batch-capturing is not tracked (captures are pinned to 1.0 on purpose).
- Animated transitions are cosmetic only: they never advance frames, so a dropped frame cannot desynchronise the tree from the array.
- Recursion depth is bounded by `ScanOptions::maxDepth` (256), which a pathological hierarchy can hit; such branches are reported, not silently dropped.

**Non-goals:** no databases, servers, Qt or Electron. No `std::priority_queue` or library heap. No incremental or live re-scan. No file deletion or modification. No cross-platform GUI guarantee. The native folder picker (`IFileDialog`) is Windows-only, and on other platforms or COM failure it falls back to a typed path field.

---

## 13. Resolved decisions

The architecture review raised fifteen findings against the first draft of these contracts. Each one was either fixed in the header, specified here, or explicitly accepted; the list is kept so the reasoning behind the contracts is auditable.

| # | Finding | Resolution |
|---|---|---|
| D1 | `MaxHeap` held a `const IHeapObserver*` while `onHeapEvent` is non-const (would not compile) | Fixed in `structures/MaxHeap.h`: the constructor parameter and member are now `IHeapObserver*`. A recording sink is inherently mutable, so const-correctness lives on the heap, not the observer. |
| D2 | `setResult` must not leave the heap pointing at a destroyed tree | Specified in `services/AnalysisEngine.h`: `setResult` clears heap and index **before** replacing the tree, and `generation()` increments on every `setResult`/`clear` so views drop cached node pointers, selections and demo frames. |
| D3 | `skip_permission_denied` hides errors | Scanner uses the `error_code` overloads and reports every failure through `ScanError` + `accessible = false`; the silent option is not used. Documented in `core/FileSystemScanner.h` and §4. |
| D4 | Junctions/mount points and the hidden bit are unreliable through libstdc++ | Added `platform/FileAttributes.h` (`isReparsePoint`, `isHiddenEntry`), implemented in `FileAttributes.cpp` and added to `diskscope_core`. Both fail closed (return false when unknown). |
| D5 | `allocatedSizeOf` used 0 for unknown | Signature is now `std::optional<std::uintmax_t>`; `nullopt` means unknown and the caller falls back to the logical size, which is never confused with a genuine 0. |
| D6 | `pathTo` has no parent link | Accepted: it walks down from the root, O(depth × fan-out), and is called only for the selected node, never per frame. `discoveryOrder_` already gives O(1) lookup by id. |
| D7 | Note strings built with no observer | Accepted as an implementation rule: `MaxHeap::record` returns before building anything when `observer_ == nullptr`; call sites pass short literals. |
| D8 | No depth limit; `progressInterval == 0` divides by zero | `ScanOptions::maxDepth` (default 256) truncates and reports deep branches; `progressInterval` of 0 is treated as 1. |
| D9 | Non-ASCII names | `FolderNode::name` is documented as UTF-8 (`filename().u8string()`); `format::toLower` is ASCII-only and the search box states that. |
| D10 | `accessible` propagation undefined | Documented as local: a node is false when it or a file directly inside it failed. Scan-wide incompleteness is reported by `errors`, `unreadableFiles` and `cancelled`. |
| D11 | `errors` unbounded | `ScanOptions::maxErrors` (500) plus `ScanResult::omittedErrors`; the Dashboard shows both. |
| D12 | `buildHeap` always records; insert demo exceeds 63 nodes | `HeapDemoEngine` discards pre-roll frames from its internal `buildHeap`; callers use `selectDemoItems(items, cap - 1)` for the insert demo so the result stays within six levels. |
| D13 | Layer order: `structures` included `services` | `HeapEvent.h` moved to `structures/HeapEvent.h` (the assignment's suggested layout is refined, which the brief allows). Dependencies now point strictly downward. |
| D14 | Root-is-a-link behaviour and `--smoke` contract unspecified | A root that is itself a link is resolved once for the scan; links below it are never followed (§4). The smoke/capture contract is specified in §11 (generated fixture, `01-overview.bmp` … `05-settings.bmp` + `00-heap-playback.bmp`, assertions, exit codes). |
| D15 | Accounting rules reconstructed from the brief | Checked against the project brief: the ten rules in §5 match rules 1–10 of the assignment, including root
excluded
from
rankings and parent
and
descendant
sizes
are
never
summed. |

---

## 14. Open questions for the course assessor

None blocking development. The two items that belong to the assignment write-up rather than the code are:

1. Whether the ranking must include the scan root (currently excluded by default, behind a Settings toggle), because the brief says "by default". Both behaviours are implemented, so either reading is defensible.
2. Whether allocated size must be part of the assessed output or is optional extra credit. It is implemented and shown, but rankings deliberately use logical size only.

# Refactoring Report — Internal Cleanup

Behaviour- and pixel-preserving cleanup of DiskScope. No feature, layout, colour, typography, animation, CLI, algorithm or error-handling change. This document records what was removed or consolidated, how each change was validated, and what debt remains.

## Measured result

| Metric | Before | After | Delta |
|---|---|---|---|
| Lines under `src/` (`*.cpp` + `*.h`) | 8434 | 8081 | −353 (−4.2 %) |
| Source files under `src/` | 49 | 48 | −1 (`ui/Animation.h`) |
| Lines under `tests/` | 1118 | 1125 | +7 (3 new characterization checks) |
| Unit tests | 23 cases / 192 checks | 23 cases / 195 checks | +3 checks, 0 removed |
| Object files compiled into `libimplot.a` | 3 | 2 (`implot_demo.cpp` dropped) | −1 TU |
| Visual baseline | 6 captures @1440×900, 6 @1024×640 | 1440×900 compared after every step; 1024×640 compared at baseline and final | 0 differing bytes in either set |

Line counts come from `wc -l` on `find src -name '*.cpp' -o -name '*.h'`; the before figures are the snapshot taken in the baseline step. No performance improvement is claimed: the `stats()` memoisation removes up to nine recomputations per frame by construction, but no wall-clock measurement was taken.

## Refactoring ledger

| Area | Original problem | Change made | Validation |
|---|---|---|---|
| `ui/Theme` | 18 legacy colour aliases (`background`, `card`, `accent`, `good`, `warn`, `bad`, … plus `*V`) that only duplicated canonical tokens | Deleted fields and their initialisers; sole consumer migrated from `backgroundV` to `bgV` | Build + `ctest`; 6/6 captures byte-identical |
| `ui/Theme` | Dead `heading()` accessor, unused metrics `rowHeight`, `sectionGap`, `iconSizeSmall` | Deleted | Same |
| `ui/Animation.h` | Entire 91-line header unused (no includer), duplicating easing math already local to `HeapRenderer.cpp` | File deleted | Build unaffected; captures identical |
| `ui/Components` | Dead widgets `Badge`, `StatusPill`, `ErrorState`, `TableHeader`, plus `IconText`/`IconMeasure` in `ui/Icons` | Deleted declarations, bodies and comments; shared `ToneColors` helper kept (still used by `ProgressBar`/`BarMeter`/`StatCard`) | Build + `ctest`; captures identical |
| `ui/Icons` | 4 enum values (`SiteMap`, `Check`, `ListNumbered`, `Dot`) referenced only by their own draw case | Removed values and cases (36 → 32 drawable icons) | Build; captures identical |
| `viz/ChartRenderer` | `seriesColour` was a one-line pass-through to `theme::chartColour` with no callers | Deleted | Build; captures identical |
| `AppState` | `autoRebuildRankings` never read; `statusMessage` written in four places and never displayed (the shell derives its status row from the engine) | Deleted both, plus the four write sites — preserving `errorMessage` clearing and `scanRunning` maintenance | `ctest`; smoke run exercises both the interactive pump and the batch drain; captures identical |
| `services` | `Application::rebuildRankings()`, `ScanController::root()`, `MainWindow::sectionCount()` declared, defined, never called | Deleted | Build; captures identical |
| `AnalysisStats` | `hasScan` believed dead | **Kept** — `tests/test_analysis.cpp` asserts it; removing it broke the test TU, so the field and its assignment were restored rather than the check weakened | Test TU compiles; 195 checks pass |
| `ui/Theme` | `withAlpha` implemented identically in three files | One `theme::withAlpha`; 10 call sites repointed (1 explorer, 1 settings, 8 heap) | Build; captures identical (heap views exercise it most) |
| `ui/Components` | `rightAlignedText` + `headerCellRight` duplicated across `DirectoryExplorer.cpp` and `LargestFoldersView.cpp` | `ui::RightAlignedText` + `ui::TableHeaderCellRight`; both copies deleted; orphaned and misplaced comments reattached to the functions they describe | Build; captures identical |
| `ui/Application` | Scan-result installation repeated three times (interactive pump, batch loop, batch drain) | Single `adoptScanResult()` returning whether a result was adopted; timeout and drain order unchanged | `SMOKE SUMMARY` unchanged (`indexed=10` proves the adoption ran); captures identical |
| `ui/HeapVisualizerView` | `HeapDrawState` built twice and the "current frame" expression pasted three times | `currentFrame()` + `makeDrawState()`; struct forward-declared in the header | Captures identical, including the mid-playback frame that carries kind/`animT` into both tree and array strip |
| `ui/AppState`, `MainWindow`, `Application` | Section kept as `int currentSection` + `int requestedSection` with `-1` sentinel despite an existing `Section` enum | `Section currentSection` + `std::optional<Section> requestedSection`; typed `setSection`; capture table uses `Section` values | All five sections rendered correctly in captures; handoff ordering unchanged |
| `services/AnalysisEngine` | `stats()` recomputed up to nine times per frame, each call rescanning the root's children | Cached behind `statsCacheValid_`, invalidated by `setResult`/`clear`/`rebuildRankings`; derivation moved verbatim into `computeStats()` | Existing tests cover `setResult`/`rebuildRankings` invalidation; 3 checks added for `clear()`; captures identical |
| `ui/Components.cpp` | Namespace closed and reopened mid-file (merge artefact) between `NavItem` and `Switch` | Joined into one `namespace diskscope::ui` block | Build; captures identical |
| Includes | Redundantly included headers across 5 files | Audited by removing one include at a time and recompiling with `-fsyntax-only`, cross-checked against direct symbol use so transitive includes were not mistaken for unused | Build clean; captures identical |
| `CMakeLists.txt` | `implot_demo.cpp` compiled into the `implot` library although no code calls the demo | Removed from the target; stale object deleted | `nm -C build/libimplot.a \| grep -c ShowDemoWindow` → 0; captures identical |

## Validation method

1. **Baseline** (before any edit): clean build with zero warnings, `ctest` 1/1, pre-refactor binary saved as `build/DiskScope.baseline.exe`, six captures at 1440×900 and 1024×640 with `--frames 40`.
2. **Determinism gate**: each baseline set was generated twice and compared; all six BMPs were byte-identical at both viewports, so the visual gate for every step is strict `cmp` equality rather than a tolerance. (The long warm-up matters: with the default two frames the eased heap settle is timing-dependent.)
3. **Per step**: rebuild, `ctest`, regenerate captures, `cmp` all six against baseline, plus targeted greps for removed symbols.
4. **Core logic**: the existing 23-case suite is the oracle; the only test edit was adding three assertions for behaviour the change newly made cache-dependent (`stats()` after `clear()`).

## Remaining technical debt (not addressed, deliberately)

- **Three per-view ranking caches** (`Dashboard`, `LargestFoldersView`, `DirectoryExplorer` filter) share a pattern but not a type. Consolidating them would add an abstraction for three users with different invalidation keys; drift risk outweighs the gain. If a fourth consumer appears, extract a `RankingCache`.
- **`Application.cpp` is still 594 lines** and mixes the run loop, CLI-independent state and the capture harness (`saveBmp`, `captureFrame`, capture steps). Splitting a `ui/CaptureHarness` unit would improve cohesion but moves a large, verified block; worth doing when the harness next changes.
- **`ScanController`'s mutex + two atomics** could be unified, but the current split is correct and load-bearing for the handoff ordering; rewriting it would trade proof for elegance.
- **`ui::Tone` and `ToneColors`** carry both `ImU32` and `ImVec4` forms of each colour, mirroring the `ImGui::TextColored` vs draw-list split. A single canonical representation plus conversion at the boundary would shrink it further, but touches every widget.
- **Smoke fixture duplication**: `main.cpp`'s `buildSmokeFixture` and `tests/fixtures.h` build similar trees with their own `writeFile` helpers. Sharing them would couple test fixtures to application fixtures; kept independent on purpose.
- **`docs/captures/`** is git-ignored smoke output, so the committed documentation cannot show before/after images; the baseline/after BMP sets live under `build/` and are regenerated on demand.
- Interactive paths (clicks, hovers, scroll, the view-→shell section handoff, the 60 s scan timeout) remain covered by inspection only; headless captures cannot produce pointer input.

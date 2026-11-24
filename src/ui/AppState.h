#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

#include "core/FileSystemScanner.h"
#include "core/FolderNode.h"

namespace diskscope {
class AnalysisEngine;
class ScanController;
}  // namespace diskscope

// Sidebar destinations. The values are the navigation order, so they must stay
// contiguous and in sync with kSectionCount.
enum class Section : int {
    Overview = 0,
    DirectoryExplorer = 1,
    LargestFolders = 2,
    HeapVisualizer = 3,
    Settings = 4,
};

inline constexpr int kSectionCount = 5;
static_assert(static_cast<int>(Section::Settings) == kSectionCount - 1,
              "kSectionCount must match the last Section value");

struct AppState {
    std::filesystem::path scanRoot;                             // path shown/typed by the user
    bool includeRoot = false;                                   // rankings include the scan root
    bool includeHidden = true;                                  // ScanOptions.includeHidden
    bool measureAllocatedSize = true;                           // ScanOptions.measureAllocatedSize
    diskscope::NodeId selectedId = diskscope::kInvalidNodeId;   // selected folder
    int topKChoice = 10;                                        // 5 / 10 / 20
    std::string searchTerm;                                     // Largest Folders filter
    std::string explorerFilter;                                 // Directory Explorer filter
    float demoFramesPerSecond = 6.0f;                           // heap visualizer playback speed
    std::string errorMessage;                                   // last user-visible error
    diskscope::AnalysisEngine* engine = nullptr;                // non-owning, lifetime = Application
    diskscope::ScanController* controller = nullptr;            // non-owning, lifetime = Application
    bool scanRunning = false;
    // Shell navigation: the active section plus a one-frame switch request that a
    // view can post when it holds no MainWindow reference (e.g. a ranking row
    // jumping to the explorer). MainWindow consumes the request and clears it.
    Section currentSection = Section::Overview;   // active sidebar section
    std::optional<Section> requestedSection;

    // Heap Visualizer presentation state that survives a section switch: which
    // heap slot the user inspected, and whether the tree takes the full height.
    int heapSelectedSlot = -1;
    bool heapExpanded = false;

    // Shell-level dialog state; the confirmation itself is rendered by MainWindow.
    bool confirmClearResults = false;

    Section section() const { return currentSection; }

    // Requests a section switch for the next frame, for call sites that hold no
    // MainWindow reference (e.g. "open in directory explorer" buttons).
    void requestSection(Section target) { requestedSection = target; }

    // helper: ScanOptions scanOptions() const built from includeHidden/measureAllocatedSize
    diskscope::ScanOptions scanOptions() const {
        diskscope::ScanOptions opts;
        opts.includeHidden = includeHidden;
        opts.measureAllocatedSize = measureAllocatedSize;
        return opts;
    }
};


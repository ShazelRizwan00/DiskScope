#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include "core/FileSystemScanner.h"
#include "core/FolderNode.h"

namespace diskscope {
class AnalysisEngine;
class ScanController;
}  // namespace diskscope

// Sidebar destinations. The values are the navigation order and are also the
// section indices requested by views (see AppState::requestedSection), so they
// must stay contiguous and in sync with kSectionCount.
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
    bool autoRebuildRankings = true;
    float demoFramesPerSecond = 6.0f;                           // heap visualizer playback speed
    std::string statusMessage;                                  // transient success/notice text
    std::string errorMessage;                                   // last user-visible error
    diskscope::AnalysisEngine* engine = nullptr;                // non-owning, lifetime = Application
    diskscope::ScanController* controller = nullptr;            // non-owning, lifetime = Application
    bool scanRunning = false;
    // A view can ask the shell to switch sections (e.g. a ranking row jumping to
    // the explorer). MainWindow consumes it and resets it to -1.
    int currentSection = static_cast<int>(Section::Overview);   // active sidebar section
    int requestedSection = -1;

    // Heap Visualizer presentation state that survives a section switch: which
    // heap slot the user inspected, and whether the tree takes the full height.
    int heapSelectedSlot = -1;
    bool heapExpanded = false;

    // Shell-level dialog state; the confirmation itself is rendered by MainWindow.
    bool confirmClearResults = false;

    Section section() const { return static_cast<Section>(currentSection); }

    // Requests a section switch for the next frame, for call sites that hold no
    // MainWindow reference (e.g. "open in directory explorer" buttons).
    void requestSection(Section target) { requestedSection = static_cast<int>(target); }

    // helper: ScanOptions scanOptions() const built from includeHidden/measureAllocatedSize
    diskscope::ScanOptions scanOptions() const {
        diskscope::ScanOptions opts;
        opts.includeHidden = includeHidden;
        opts.measureAllocatedSize = measureAllocatedSize;
        return opts;
    }
};


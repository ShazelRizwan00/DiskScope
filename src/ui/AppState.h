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

struct AppState {
    std::filesystem::path scanRoot;                             // path shown/typed by the user
    bool includeRoot = false;                                   // rankings include the scan root
    bool includeHidden = true;                                  // ScanOptions.includeHidden
    bool measureAllocatedSize = true;                           // ScanOptions.measureAllocatedSize
    diskscope::NodeId selectedId = diskscope::kInvalidNodeId;   // selected folder
    int topKChoice = 10;                                        // 5 / 10 / 20
    std::string searchTerm;
    bool autoRebuildRankings = true;
    float demoFramesPerSecond = 6.0f;                           // heap visualizer playback speed
    std::string statusMessage;                                  // transient success/notice text
    std::string errorMessage;                                   // last user-visible error
    diskscope::AnalysisEngine* engine = nullptr;                // non-owning, lifetime = Application
    diskscope::ScanController* controller = nullptr;            // non-owning, lifetime = Application
    bool scanRunning = false;
    // A view can ask the shell to switch sections (e.g. a ranking row jumping to
    // the explorer). MainWindow consumes it and resets it to -1.
    int requestedSection = -1;

    // helper: ScanOptions scanOptions() const built from includeHidden/measureAllocatedSize
    diskscope::ScanOptions scanOptions() const {
        diskscope::ScanOptions opts;
        opts.includeHidden = includeHidden;
        opts.measureAllocatedSize = measureAllocatedSize;
        return opts;
    }
};


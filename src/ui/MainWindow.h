#pragma once

#include "ui/AppState.h"
#include "ui/Dashboard.h"
#include "ui/DirectoryExplorer.h"
#include "ui/LargestFoldersView.h"
#include "ui/HeapVisualizerView.h"
#include "ui/SettingsView.h"

namespace diskscope {

class Application;

class MainWindow {
public:
    explicit MainWindow(Application& app);
    void draw();
    void setSection(int section);
    int sectionCount() const;

private:
    Application& app_;
    int currentSection_ = 0;
    char pathBuffer_[1024] = {0};
    bool lastScanRunning_ = false;

    Dashboard dashboardView_;
    DirectoryExplorer directoryExplorerView_;
    LargestFoldersView largestFoldersView_;
    HeapVisualizerView heapVisualizerView_;
    SettingsView settingsView_;
};


}  // namespace diskscope


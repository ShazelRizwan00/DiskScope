#pragma once

#include "ui/AppState.h"
#include "ui/Dashboard.h"
#include "ui/DirectoryExplorer.h"
#include "ui/HeapVisualizerView.h"
#include "ui/LargestFoldersView.h"
#include "ui/SettingsView.h"

namespace diskscope {

class Application;

// Shell of the application: sidebar navigation, scan toolbar and the view host.
// The five views own their own layout and scrolling; the shell only decides
// which one is visible and how much space it gets.
class MainWindow {
public:
    explicit MainWindow(Application& app);
    void draw();
    void setSection(Section section);

    // Offline-capture hook: queues a real heap demo so a headless run can show
    // a mid-operation frame instead of the static heap.
    void requestHeapDemo(int demo, bool pauseAtFirstHighlight = true);

private:
    void drawSidebar(AppState& state, float height);
    void drawToolbar(AppState& state, float width);
    void drawStatusRow(const AppState& state, float width);
    void drawContent(AppState& state);
    void clearResults(AppState& state);

    Application& app_;
    char pathBuffer_[1024] = {0};

    Dashboard dashboardView_;
    DirectoryExplorer directoryExplorerView_;
    LargestFoldersView largestFoldersView_;
    HeapVisualizerView heapVisualizerView_;
    SettingsView settingsView_;
};

}  // namespace diskscope

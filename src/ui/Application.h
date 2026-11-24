#pragma once

#include <filesystem>
#include <memory>

#include "services/AnalysisEngine.h"
#include "services/ScanController.h"
#include "ui/AppState.h"

struct GLFWwindow;
struct ImPlotContext;

namespace diskscope {

class MainWindow;

struct AppOptions {
    std::filesystem::path root;
    bool smoke = false;
    bool capture = false;
    std::filesystem::path captureDir;
    int frames = 2;
    int width = 1440;
    int height = 900;
    float dpiScale = 0.0f;
};

struct CaptureAnalysis {
    bool ok = false;
    int width = 0;
    int height = 0;
    int distinctColors = 0;
    float nonBgRatio = 0.0f;
    int toolbarDistinctColors = 0;
};

class Application {
public:
    explicit Application(AppOptions options = {});
    ~Application();

    int run();

    diskscope::AnalysisEngine& engine();
    diskscope::ScanController& controller();
    AppState& state();

    bool startScan(const std::filesystem::path& path);
    void cancelScan();
    void rebuildRankings();

private:
    bool initGraphics();
    void shutdown();
    bool renderOneFrame();
    int runInteractiveLoop();
    int runBatchMode();
    void rebuildFonts(float scale);

    AppOptions options_;
    diskscope::AnalysisEngine engine_;
    diskscope::ScanController controller_;
    AppState state_;
    std::unique_ptr<MainWindow> mainWindow_;

    GLFWwindow* window_ = nullptr;
    ImPlotContext* plotContext_ = nullptr;
    bool glfwInitialized_ = false;
    bool imguiInitialized_ = false;

    // Capture is done inside the frame, before the buffer swap: reading the
    // back buffer after glfwSwapBuffers would return undefined pixels.
    std::filesystem::path pendingCapturePath_;
    bool lastCaptureOk_ = false;
    CaptureAnalysis lastCaptureAnalysis_;
    float currentDpiScale_ = 1.0f;
};


}  // namespace diskscope


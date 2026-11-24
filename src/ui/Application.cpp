#include "ui/Application.h"

#include <algorithm>
#include <bitset>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>

#include "ui/MainWindow.h"
#include "ui/Theme.h"
#include "utils/Formatting.h"

namespace {

static bool saveBmp(const std::filesystem::path& path, int width, int height, const std::vector<unsigned char>& rgba) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }

    int rowStride = ((width * 3 + 3) / 4) * 4;
    std::uint32_t imageSize = static_cast<std::uint32_t>(rowStride * height);
    std::uint32_t fileSize = 54 + imageSize;

    unsigned char fileHeader[14] = {
        'B', 'M',
        static_cast<unsigned char>(fileSize & 0xFF),
        static_cast<unsigned char>((fileSize >> 8) & 0xFF),
        static_cast<unsigned char>((fileSize >> 16) & 0xFF),
        static_cast<unsigned char>((fileSize >> 24) & 0xFF),
        0, 0, 0, 0,
        54, 0, 0, 0
    };

    unsigned char infoHeader[40] = {
        40, 0, 0, 0,
        static_cast<unsigned char>(width & 0xFF),
        static_cast<unsigned char>((width >> 8) & 0xFF),
        static_cast<unsigned char>((width >> 16) & 0xFF),
        static_cast<unsigned char>((width >> 24) & 0xFF),
        static_cast<unsigned char>(height & 0xFF),
        static_cast<unsigned char>((height >> 8) & 0xFF),
        static_cast<unsigned char>((height >> 16) & 0xFF),
        static_cast<unsigned char>((height >> 24) & 0xFF),
        1, 0,
        24, 0,
        0, 0, 0, 0,
        static_cast<unsigned char>(imageSize & 0xFF),
        static_cast<unsigned char>((imageSize >> 8) & 0xFF),
        static_cast<unsigned char>((imageSize >> 16) & 0xFF),
        static_cast<unsigned char>((imageSize >> 24) & 0xFF),
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    };

    out.write(reinterpret_cast<const char*>(fileHeader), 14);
    out.write(reinterpret_cast<const char*>(infoHeader), 40);

    std::vector<unsigned char> row(static_cast<std::size_t>(rowStride), 0);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            std::size_t srcIdx = static_cast<std::size_t>((y * width + x) * 4);
            row[static_cast<std::size_t>(x * 3 + 0)] = rgba[srcIdx + 2]; // B
            row[static_cast<std::size_t>(x * 3 + 1)] = rgba[srcIdx + 1]; // G
            row[static_cast<std::size_t>(x * 3 + 2)] = rgba[srcIdx + 0]; // R
        }
        out.write(reinterpret_cast<const char*>(row.data()), rowStride);
    }
    return out.good();
}

static diskscope::CaptureAnalysis captureFrame(GLFWwindow* window, const std::filesystem::path& path) {
    diskscope::CaptureAnalysis result;
    int w = 0;
    int h = 0;
    glfwGetFramebufferSize(window, &w, &h);
    if (w <= 0 || h <= 0) {
        return result;
    }
    result.width = w;
    result.height = h;

    std::vector<unsigned char> rgba(static_cast<std::size_t>(w * h * 4));
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

    if (!saveBmp(path, w, h, rgba)) {
        return result;
    }
    result.ok = true;

    // Theme background color converted to 0-255
    const ImVec4& bgV = diskscope::theme::colors().bgV;
    const int bgR = std::clamp(static_cast<int>(bgV.x * 255.0f + 0.5f), 0, 255);
    const int bgG = std::clamp(static_cast<int>(bgV.y * 255.0f + 0.5f), 0, 255);
    const int bgB = std::clamp(static_cast<int>(bgV.z * 255.0f + 0.5f), 0, 255);

    // 5 bits per channel (0..31) -> 15-bit color key [0..32767]
    std::bitset<32768> seenTotal;
    std::bitset<32768> seenToolbar;

    const int totalPixels = w * h;
    int nonBgCount = 0;
    int distinctCount = 0;
    int toolbarDistinctCount = 0;

    // Top 112 px toolbar band in OpenGL framebuffer coordinates:
    // OpenGL row y=0 is the bottom, y=h-1 is the top. Top 112 px band is [h - toolbarBand, h).
    const int toolbarBand = std::min(h, 112);
    const int toolbarMinY = h - toolbarBand;

    for (int y = 0; y < h; ++y) {
        const bool inToolbar = (y >= toolbarMinY);
        const std::size_t rowOffset = static_cast<std::size_t>(y * w * 4);
        for (int x = 0; x < w; ++x) {
            const std::size_t idx = rowOffset + static_cast<std::size_t>(x * 4);
            const int r = static_cast<int>(rgba[idx + 0]);
            const int g = static_cast<int>(rgba[idx + 1]);
            const int b = static_cast<int>(rgba[idx + 2]);

            if (std::abs(r - bgR) > 8 || std::abs(g - bgG) > 8 || std::abs(b - bgB) > 8) {
                ++nonBgCount;
            }

            const std::size_t key = static_cast<std::size_t>(((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3));
            if (distinctCount < 4096) {
                if (!seenTotal.test(key)) {
                    seenTotal.set(key);
                    ++distinctCount;
                }
            }

            if (inToolbar && toolbarDistinctCount < 4096) {
                if (!seenToolbar.test(key)) {
                    seenToolbar.set(key);
                    ++toolbarDistinctCount;
                }
            }
        }
    }

    result.distinctColors = distinctCount;
    result.nonBgRatio = totalPixels > 0 ? static_cast<float>(nonBgCount) / static_cast<float>(totalPixels) : 0.0f;
    result.toolbarDistinctColors = toolbarDistinctCount;
    return result;
}

}  // namespace

namespace diskscope {

Application::Application(AppOptions options)
    : options_(std::move(options)) {
    state_.engine = &engine_;
    state_.controller = &controller_;
    if (!options_.root.empty()) {
        state_.scanRoot = options_.root;
    }
}

Application::~Application() {
    shutdown();
}

diskscope::AnalysisEngine& Application::engine() {
    return engine_;
}

diskscope::ScanController& Application::controller() {
    return controller_;
}

AppState& Application::state() {
    return state_;
}

bool Application::startScan(const std::filesystem::path& path) {
    if (path.empty()) {
        return false;
    }
    state_.errorMessage.clear();
    state_.statusMessage = "Scanning " + path.string() + "...";
    state_.scanRoot = path;
    bool started = controller_.start(path, state_.scanOptions());
    if (started) {
        state_.scanRunning = true;
    } else {
        state_.scanRunning = false;
        state_.errorMessage = controller_.lastError();
        if (state_.errorMessage.empty()) {
            state_.errorMessage = "Failed to start scan (invalid directory or already running)";
        }
    }
    return started;
}

void Application::cancelScan() {
    controller_.requestCancel();
}

void Application::rebuildRankings() {
    if (engine_.hasResult()) {
        engine_.rebuildRankings(state_.includeRoot);
    }
}

bool Application::initGraphics() {
    if (!glfwInit()) {
        std::fprintf(stderr, "DiskScope: Failed to initialize GLFW\n");
        return false;
    }
    glfwInitialized_ = true;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    if (options_.smoke || options_.capture) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }

    window_ = glfwCreateWindow(options_.width, options_.height, "DiskScope", nullptr, nullptr);
    if (!window_) {
        std::fprintf(stderr, "DiskScope: Failed to create GLFW window\n");
        return false;
    }
    glfwSetWindowSizeLimits(window_, 1024, 640, GLFW_DONT_CARE, GLFW_DONT_CARE);

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    plotContext_ = ImPlot::CreateContext();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    const bool batchMode = options_.smoke || options_.capture;
    float xscale = 1.0f;
    float yscale = 1.0f;
    glfwGetWindowContentScale(window_, &xscale, &yscale);
    const float contentScale = std::max(xscale, yscale);
    const float dpiScale = options_.dpiScale > 0.0f ? options_.dpiScale : (batchMode ? 1.0f : contentScale);
    currentDpiScale_ = dpiScale;

    theme::apply(dpiScale);
    theme::loadFonts(dpiScale);
    io.FontDefault = theme::body();

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    imguiInitialized_ = true;

    mainWindow_ = std::make_unique<MainWindow>(*this);
    return true;
}

void Application::rebuildFonts(float scale) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplOpenGL3_DestroyFontsTexture();
    io.Fonts->Clear();
    io.FontDefault = nullptr;
    theme::apply(scale);
    theme::loadFonts(scale);
    io.Fonts->Build();
    ImGui_ImplOpenGL3_CreateFontsTexture();
    io.FontDefault = theme::body();
    currentDpiScale_ = scale;
}

void Application::shutdown() {
    if (controller_.running()) {
        controller_.requestCancel();
    }
    mainWindow_.reset();

    if (plotContext_) {
        ImPlot::DestroyContext(plotContext_);
        plotContext_ = nullptr;
    }

    if (imguiInitialized_) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        imguiInitialized_ = false;
    }

    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }

    if (glfwInitialized_) {
        glfwTerminate();
        glfwInitialized_ = false;
    }
}

bool Application::renderOneFrame() {
    glfwPollEvents();
    if (glfwWindowShouldClose(window_)) {
        return false;
    }

    int displayW = 0;
    int displayH = 0;
    glfwGetFramebufferSize(window_, &displayW, &displayH);

    // Skip rendering when iconified
    if (displayW <= 0 || displayH <= 0) {
        return true;
    }

    // Scan result pump
    if (auto result = controller_.takeResult()) {
        engine_.setResult(std::move(*result));
        engine_.rebuildRankings(state_.includeRoot);
        state_.scanRunning = false;

        const auto& res = engine_.result();
        if (res.cancelled) {
            state_.statusMessage = "Scan cancelled (partial results)";
        } else {
            auto stats = engine_.stats();
            state_.statusMessage = "Scan complete: " + format::byteSize(stats.totalBytes)
                + ", " + format::count(stats.fileCount) + " files, "
                + format::count(stats.folderCount) + " folders";
            if (stats.errorCount > 0) {
                state_.statusMessage += " (" + format::count(stats.errorCount) + " errors)";
            }
        }
    }
    state_.scanRunning = controller_.running();

    // A worker that threw publishes an error but no result; surface it instead of
    // leaving the user with a silently empty view.
    if (!state_.scanRunning && !engine_.hasResult() && state_.errorMessage.empty()) {
        const std::string workerError = controller_.lastError();
        if (!workerError.empty()) {
            state_.errorMessage = workerError;
        }
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    mainWindow_->draw();

    ImGui::Render();
    glViewport(0, 0, displayW, displayH);
    const auto& bg = theme::colors().backgroundV;
    glClearColor(bg.x, bg.y, bg.z, bg.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Capture before swapping buffers: after the swap the back buffer holds
    // undefined contents, so a post-swap glReadPixels would give black frames.
    if (!pendingCapturePath_.empty()) {
        lastCaptureAnalysis_ = captureFrame(window_, pendingCapturePath_);
        lastCaptureOk_ = lastCaptureAnalysis_.ok;
        pendingCapturePath_.clear();
    }

    glfwSwapBuffers(window_);
    return true;
}

int Application::runInteractiveLoop() {
    while (!glfwWindowShouldClose(window_)) {
        if (options_.dpiScale == 0.0f) {
            float xscale = 1.0f;
            float yscale = 1.0f;
            glfwGetWindowContentScale(window_, &xscale, &yscale);
            const float scale = std::max(xscale, yscale);
            if (std::abs(scale - currentDpiScale_) > 0.01f) {
                rebuildFonts(scale);
            }
        }
        if (!renderOneFrame()) {
            break;
        }
    }
    return 0;
}

int Application::runBatchMode() {
    if (options_.smoke) {
        std::cout << "SMOKE START: Initializing smoke tests\n";
    }

    if (!options_.root.empty()) {
        if (!startScan(options_.root)) {
            if (options_.smoke) {
                std::fprintf(stderr, "SMOKE FAIL: Failed to start scan for %s\n", options_.root.string().c_str());
            }
            return 1;
        }
        if (options_.smoke) {
            std::cout << "SMOKE PASS: Scan started for " << options_.root.string() << "\n";
        }

        // Bounded loop pumping until scan finishes
        auto startTime = std::chrono::steady_clock::now();
        while (controller_.running()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            if (auto res = controller_.takeResult()) {
                engine_.setResult(std::move(*res));
                engine_.rebuildRankings(state_.includeRoot);
                state_.scanRunning = false;
                break;
            }
            if (std::chrono::steady_clock::now() - startTime > std::chrono::seconds(60)) {
                if (options_.smoke) {
                    std::fprintf(stderr, "SMOKE FAIL: Scan timed out after 60 seconds\n");
                }
                controller_.requestCancel();
                return 1;
            }
        }

        // The worker may have finished between the last poll and running()
        // turning false, so always drain the handoff once more.
        if (!engine_.hasResult()) {
            if (auto res = controller_.takeResult()) {
                engine_.setResult(std::move(*res));
                engine_.rebuildRankings(state_.includeRoot);
                state_.scanRunning = false;
            }
        }

        if (engine_.hasResult()) {
            const AnalysisStats stats = engine_.stats();
            state_.statusMessage = "Scan complete: " + format::byteSize(stats.totalBytes) + ", " +
                                   format::count(stats.fileCount) + " files, " +
                                   format::count(stats.folderCount) + " folders";
            if (stats.errorCount > 0) {
                state_.statusMessage += " (" + format::count(stats.errorCount) + " errors)";
            }
            state_.scanRunning = false;
        }
        if (options_.smoke && engine_.hasResult()) {
            const AnalysisStats stats = engine_.stats();
            std::cout << "SMOKE SUMMARY: bytes=" << stats.totalBytes << " files=" << stats.fileCount
                      << " folders=" << stats.folderCount << " indexed=" << stats.indexedFolders
                      << " errors=" << stats.errorCount << "\n";
            const std::vector<const FolderNode*> top = engine_.topK(5);
            for (std::size_t i = 0; i < top.size(); ++i) {
                std::cout << "SMOKE TOP " << (i + 1) << ": bytes=" << top[i]->totalBytes
                          << " name=" << top[i]->name << "\n";
            }
        }
        if (options_.smoke) {
            std::cout << "SMOKE PASS: Scan completed\n";
        }
    }

    if (options_.smoke) {
        if (!engine_.heap().validate()) {
            std::fprintf(stderr, "SMOKE FAIL: Heap validation failed\n");
            return 1;
        }
        std::cout << "SMOKE PASS: Heap validated\n";
    }

    std::filesystem::path capDir = options_.captureDir;
    if (capDir.empty() && options_.smoke) {
        capDir = std::filesystem::temp_directory_path() / "diskscope_captures";
    }

    if (options_.capture || options_.smoke) {
        std::error_code ec;
        std::filesystem::create_directories(capDir, ec);

        int fbW = 0;
        int fbH = 0;
        glfwGetFramebufferSize(window_, &fbW, &fbH);
        char dpiBuf[32];
        if (currentDpiScale_ == static_cast<int>(currentDpiScale_)) {
            std::snprintf(dpiBuf, sizeof(dpiBuf), "%.1f", currentDpiScale_);
        } else {
            std::snprintf(dpiBuf, sizeof(dpiBuf), "%g", currentDpiScale_);
        }
        std::cout << "SMOKE FRAMEBUFFER: " << fbW << "x" << fbH
                  << " requested=" << options_.width << "x" << options_.height
                  << " dpi=" << dpiBuf << "\n";
        if (fbW != options_.width || fbH != options_.height) {
            std::cout << "SMOKE NOTE: Actual framebuffer " << fbW << "x" << fbH
                      << " differs from requested " << options_.width << "x" << options_.height << "\n";
        }

        struct CaptureStep {
            int section;
            const char* fileName;
        };

        const CaptureStep steps[] = {
            { 0, "01-overview.bmp" },
            { 1, "02-directory-explorer.bmp" },
            { 2, "03-largest-folders.bmp" },
            { 3, "04-heap-visualizer.bmp" },
            { 4, "05-settings.bmp" },
            { 3, "00-heap-playback.bmp" }
        };

        for (const auto& step : steps) {
            mainWindow_->setSection(step.section);
            if (std::strcmp(step.fileName, "00-heap-playback.bmp") == 0) {
                mainWindow_->requestHeapDemo(0, true);
            }
            // The directory view is only meaningful with a folder selected, so the
            // capture opens the largest ranked folder the way a user would.
            if (step.section == static_cast<int>(Section::DirectoryExplorer) &&
                state_.selectedId == kInvalidNodeId) {
                const std::vector<const FolderNode*> top = engine_.topK(1);
                if (!top.empty() && top.front() != nullptr) {
                    state_.selectedId = top.front()->id;
                }
            }
            // Selecting the root slot exercises the node detail panel and the
            // tree/array selection sync in the heap capture. It has to happen after
            // the warm-up frames: the view clears the selection when it notices the
            // new scan generation on its first draw.
            const int frameRenders = std::max(2, options_.frames);
            for (int f = 0; f < frameRenders; ++f) {
                renderOneFrame();
            }

            if (step.section == static_cast<int>(Section::HeapVisualizer) &&
                std::strcmp(step.fileName, "04-heap-visualizer.bmp") == 0) {
                state_.heapSelectedSlot = 0;
            }

            pendingCapturePath_ = capDir / step.fileName;
            lastCaptureOk_ = false;
            if (!renderOneFrame() || !lastCaptureOk_) {
                if (options_.smoke) {
                    std::fprintf(stderr, "SMOKE FAIL: Failed to write %s\n", step.fileName);
                }
                return 1;
            }

            const CaptureAnalysis& m = lastCaptureAnalysis_;
            char pctBuf[32];
            std::snprintf(pctBuf, sizeof(pctBuf), "%.1f%%", m.nonBgRatio * 100.0f);
            std::cout << "SMOKE CAPTURE: " << step.fileName << " "
                      << m.width << "x" << m.height
                      << " distinct=" << m.distinctColors
                      << " nonbg=" << pctBuf << "\n";

            // Distinct colours and the toolbar band carry the real signal: a frame
            // that never rendered reports 1-2 colours, while any real view reports
            // a hundred or more. The non-background floor is deliberately low (3%)
            // because a dark theme with a sparse table stays near-background at
            // large viewports without anything being wrong.
            if (m.distinctColors < 12 || m.nonBgRatio < 0.03f || m.toolbarDistinctColors < 6) {
                std::fprintf(stderr, "SMOKE FAIL: capture %s looks blank or uniform (distinct=%d, nonbg=%.1f%%)\n",
                             step.fileName, m.distinctColors, m.nonBgRatio * 100.0f);
                return 1;
            }
        }

        if (options_.smoke) {
            std::cout << "SMOKE PASS: All sections captured to " << capDir.string() << "\n";
        }
    }

    if (options_.smoke) {
        std::cout << "SMOKE PASS: All smoke checks passed\n";
    }
    return 0;
}

int Application::run() {
    try {
        if (!initGraphics()) {
            shutdown();
            return 1;
        }

        int exitCode = 0;
        if (options_.smoke || options_.capture) {
            exitCode = runBatchMode();
        } else {
            exitCode = runInteractiveLoop();
        }

        shutdown();
        return exitCode;
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "DiskScope error: %s\n", ex.what());
        shutdown();
        return 1;
    } catch (...) {
        std::fprintf(stderr, "DiskScope error: unknown exception\n");
        shutdown();
        return 1;
    }
}

}  // namespace diskscope

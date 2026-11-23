#include "ui/Application.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
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

static bool captureFrame(GLFWwindow* window, const std::filesystem::path& path) {
    int w = 0;
    int h = 0;
    glfwGetFramebufferSize(window, &w, &h);
    if (w <= 0 || h <= 0) {
        return false;
    }
    std::vector<unsigned char> rgba(static_cast<std::size_t>(w * h * 4));
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    return saveBmp(path, w, h, rgba);
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

    window_ = glfwCreateWindow(1440, 900, "DiskScope", nullptr, nullptr);
    if (!window_) {
        std::fprintf(stderr, "DiskScope: Failed to create GLFW window\n");
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    plotContext_ = ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    theme::apply(1.0f);
    theme::loadFonts(1.0f);

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    imguiInitialized_ = true;

    mainWindow_ = std::make_unique<MainWindow>(*this);
    return true;
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
        lastCaptureOk_ = captureFrame(window_, pendingCapturePath_);
        pendingCapturePath_.clear();
    }

    glfwSwapBuffers(window_);
    return true;
}

int Application::runInteractiveLoop() {
    while (!glfwWindowShouldClose(window_)) {
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
            const int frameRenders = std::max(2, options_.frames);
            for (int f = 0; f < frameRenders; ++f) {
                renderOneFrame();
            }

            pendingCapturePath_ = capDir / step.fileName;
            lastCaptureOk_ = false;
            if (!renderOneFrame() || !lastCaptureOk_) {
                if (options_.smoke) {
                    std::fprintf(stderr, "SMOKE FAIL: Failed to write %s\n", step.fileName);
                }
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

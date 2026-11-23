#include "ui/MainWindow.h"

#include <cstring>
#include <string>

#include <imgui.h>

#include "platform/NativeFolderDialog.h"
#include "ui/Application.h"
#include "ui/Theme.h"
#include "utils/Formatting.h"

namespace diskscope {

MainWindow::MainWindow(Application& app)
    : app_(app) {
}

void MainWindow::setSection(int section) {
    if (section >= 0 && section < 5) {
        currentSection_ = section;
    }
}

int MainWindow::sectionCount() const {
    return 5;
}

void MainWindow::draw() {
    AppState& state = app_.state();

    // Check if scan just finished
    if (lastScanRunning_ && !state.scanRunning && app_.engine().hasResult()) {
        const auto& res = app_.engine().result();
        if (res.cancelled) {
            state.statusMessage = "Scan cancelled (partial results)";
        } else {
            auto stats = app_.engine().stats();
            state.statusMessage = "Scan complete: " + format::byteSize(stats.totalBytes)
                + ", " + format::count(stats.fileCount) + " files, "
                + format::count(stats.folderCount) + " folders";
            if (stats.errorCount > 0) {
                state.statusMessage += " (" + format::count(stats.errorCount) + " errors)";
            }
        }
    }
    lastScanRunning_ = state.scanRunning;

    // Sync pathBuffer_ if it was empty but state.scanRoot is set
    if (pathBuffer_[0] == '\0' && !state.scanRoot.empty()) {
        std::string s = state.scanRoot.string();
        std::strncpy(pathBuffer_, s.c_str(), sizeof(pathBuffer_) - 1);
        pathBuffer_[sizeof(pathBuffer_) - 1] = '\0';
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGuiWindowFlags rootFlags = ImGuiWindowFlags_NoDecoration
                               | ImGuiWindowFlags_NoMove
                               | ImGuiWindowFlags_NoResize
                               | ImGuiWindowFlags_NoSavedSettings
                               | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("DiskScopeRoot", nullptr, rootFlags);

    // Left sidebar (~220px, full height, distinct background, subtle right border)
    float sidebarWidth = 220.0f;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, theme::colors().sidebarV);
    ImGui::PushStyleColor(ImGuiCol_Border, theme::colors().borderV);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));

    ImGui::BeginChild("Sidebar", ImVec2(sidebarWidth, 0.0f), true, ImGuiWindowFlags_NoScrollbar);

    // Product name "DiskScope" with small subtitle "Disk storage analyzer"
    ImGui::PushFont(theme::heading());
    ImGui::TextColored(theme::colors().accentV, "DiskScope");
    ImGui::PopFont();
    ImGui::TextColored(theme::colors().textMutedV, "Disk storage analyzer");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Five nav buttons
    const char* navTitles[5] = {
        "Overview",
        "Directory Explorer",
        "Largest Folders",
        "Heap Visualizer",
        "Settings"
    };

    for (int i = 0; i < 5; ++i) {
        bool active = (currentSection_ == i);
        if (active) {
            ImGui::PushStyleColor(ImGuiCol_Button, theme::colors().accentSoftV);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, theme::colors().accentSoftV);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, theme::colors().accentSoftV);
            ImGui::PushStyleColor(ImGuiCol_Text, theme::colors().accentV);
            ImGui::PushStyleColor(ImGuiCol_Border, theme::colors().accentV);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, theme::colors().cardHoverV);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, theme::colors().cardHoverV);
            ImGui::PushStyleColor(ImGuiCol_Text, theme::colors().textV);
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        }

        if (ImGui::Button(navTitles[i], ImVec2(-1.0f, 36.0f))) {
            setSection(i);
        }

        ImGui::PopStyleColor(5);
        ImGui::Spacing();
    }

    // Footer block with scan state using theme colours
    float footerHeight = 85.0f;
    float availY = ImGui::GetContentRegionAvail().y;
    if (availY > footerHeight) {
        ImGui::Dummy(ImVec2(0.0f, availY - footerHeight));
    }

    ImGui::Separator();
    ImGui::Spacing();

    if (state.scanRunning) {
        ScanProgress prog = app_.controller().progress();
        ImGui::TextColored(theme::colors().warnV, "SCANNING...");
        ImGui::TextColored(theme::colors().textMutedV, "%s files", format::count(prog.files).c_str());
        ImGui::TextColored(theme::colors().textMutedV, "%s", format::byteSize(prog.bytes).c_str());
    } else if (!state.errorMessage.empty()) {
        ImGui::TextColored(theme::colors().badV, "ERROR");
        ImGui::TextColored(theme::colors().textMutedV, "%s", format::ellipsize(state.errorMessage, 20).c_str());
    } else if (app_.engine().hasResult()) {
        if (app_.engine().result().cancelled) {
            ImGui::TextColored(theme::colors().warnV, "CANCELLED");
            ImGui::TextColored(theme::colors().textMutedV, "Partial results");
        } else {
            ImGui::TextColored(theme::colors().goodV, "READY");
            auto stats = app_.engine().stats();
            ImGui::TextColored(theme::colors().textMutedV, "%s folders", format::count(stats.indexedFolders).c_str());
        }
    } else {
        ImGui::TextColored(theme::colors().textMutedV, "IDLE");
        ImGui::TextColored(theme::colors().textMutedV, "No scan loaded");
    }

    ImGui::EndChild();  // Sidebar
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    // Right side content area
    ImGui::SameLine();
    ImGui::BeginGroup();

    // Top Bar
    float topBarHeight = 82.0f;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, theme::colors().cardV);
    ImGui::PushStyleColor(ImGuiCol_Border, theme::colors().borderV);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 10.0f));

    ImGui::BeginChild("TopBar", ImVec2(0.0f, topBarHeight), true);

    // Row 1: Path input and scan action buttons
    float buttonsWidth = 370.0f;
    float inputWidth = ImGui::GetContentRegionAvail().x - buttonsWidth;
    if (inputWidth < 180.0f) {
        inputWidth = 180.0f;
    }

    ImGui::SetNextItemWidth(inputWidth);
    if (ImGui::InputTextWithHint("##ScanPath", "Folder path to analyze...", pathBuffer_, sizeof(pathBuffer_))) {
        state.scanRoot = pathBuffer_;
    }

    ImGui::SameLine();
    if (ImGui::Button("Select folder...")) {
        auto chosen = diskscope::platform::pickFolder(pathBuffer_);
        if (chosen) {
            std::string s = chosen->string();
            std::strncpy(pathBuffer_, s.c_str(), sizeof(pathBuffer_) - 1);
            pathBuffer_[sizeof(pathBuffer_) - 1] = '\0';
            state.scanRoot = *chosen;
        }
    }

    ImGui::SameLine();
    bool canScan = !state.scanRunning && (pathBuffer_[0] != '\0');
    if (!canScan) {
        ImGui::BeginDisabled();
    }
    ImGui::PushStyleColor(ImGuiCol_Button, theme::colors().accentSoftV);
    ImGui::PushStyleColor(ImGuiCol_Text, theme::colors().accentV);
    if (ImGui::Button("Scan")) {
        state.scanRoot = pathBuffer_;
        app_.startScan(state.scanRoot);
    }
    ImGui::PopStyleColor(2);
    if (!canScan) {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    bool canCancel = state.scanRunning;
    if (!canCancel) {
        ImGui::BeginDisabled();
    }
    ImGui::PushStyleColor(ImGuiCol_Button, theme::colors().badV);
    if (ImGui::Button("Cancel")) {
        app_.cancelScan();
    }
    ImGui::PopStyleColor();
    if (!canCancel) {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    bool canRescan = !state.scanRunning && (pathBuffer_[0] != '\0');
    if (!canRescan) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Rescan")) {
        state.scanRoot = pathBuffer_;
        app_.startScan(state.scanRoot);
    }
    if (!canRescan) {
        ImGui::EndDisabled();
    }

    // Row 2: Progress line / status line
    ImGui::Spacing();
    if (state.scanRunning) {
        ScanProgress prog = app_.controller().progress();
        std::string progLine = "Scanning: " + format::count(prog.folders) + " folders, "
            + format::count(prog.files) + " files, "
            + format::byteSize(prog.bytes);
        if (!prog.current.empty()) {
            progLine += " | " + format::ellipsize(prog.current.string(), 55);
        }
        ImGui::TextColored(theme::colors().warnV, "%s", progLine.c_str());
    } else if (!state.errorMessage.empty()) {
        ImGui::TextColored(theme::colors().badV, "Error: %s", state.errorMessage.c_str());
    } else if (!state.statusMessage.empty()) {
        ImGui::TextColored(theme::colors().goodV, "%s", state.statusMessage.c_str());
    } else {
        ImGui::TextColored(theme::colors().textMutedV, "Select a directory and click Scan to analyze disk space.");
    }

    ImGui::EndChild();  // TopBar
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    // View content area (padded child region)
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, theme::colors().backgroundV);

    ImGui::BeginChild("ViewContainer", ImVec2(0.0f, 0.0f), false);

    switch (currentSection_) {
        case 0: dashboardView_.draw(state); break;
        case 1: directoryExplorerView_.draw(state); break;
        case 2: largestFoldersView_.draw(state); break;
        case 3: heapVisualizerView_.draw(state); break;
        case 4: settingsView_.draw(state); break;
        default: break;
    }

    ImGui::EndChild();  // ViewContainer
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    ImGui::EndGroup();

    ImGui::End();  // DiskScopeRoot
    ImGui::PopStyleVar(2);
}

}  // namespace diskscope

#include "ui/MainWindow.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

#include <imgui.h>

#include "platform/NativeFolderDialog.h"
#include "services/AnalysisEngine.h"
#include "services/ScanController.h"
#include "ui/Application.h"
#include "ui/Components.h"
#include "ui/Icons.h"
#include "ui/Theme.h"
#include "utils/Formatting.h"

namespace diskscope {
namespace {

// Small local helpers for the shell chrome. They exist so the toolbar and the
// sidebar footer can place an icon inline with text without every view needing
// to know about ImDrawList cursor math.
void inlineIcon(ui::Icon icon, ImU32 colour, float size, float gap = 6.0f) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float lineH = ImGui::GetTextLineHeight();
    const ImVec2 centre(p.x + size * 0.5f, p.y + lineH * 0.5f);
    ui::DrawIcon(ImGui::GetWindowDrawList(), icon, centre, size, colour, 1.6f);
    ImGui::Dummy(ImVec2(size, lineH));
    ImGui::SameLine(0.0f, gap);
}

// Thin vertical rule between status segments. Deliberately not ui::Divider(true):
// that one spans the remaining child height, which is wrong inside a text row.
void statusSeparator() {
    const float lineH = ImGui::GetTextLineHeight();
    ImGui::SameLine(0.0f, 10.0f);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x, p.y + 2.0f), ImVec2(p.x, p.y + lineH - 2.0f),
                                        theme::colors().border, 1.0f);
    ImGui::Dummy(ImVec2(1.0f, lineH));
    ImGui::SameLine(0.0f, 10.0f);
}

void hairline() {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddLine(p, ImVec2(p.x + ImGui::GetContentRegionAvail().x, p.y),
                                        theme::colors().border, 1.0f);
    ImGui::Dummy(ImVec2(0.0f, 1.0f));
}

}  // namespace

MainWindow::MainWindow(Application& app)
    : app_(app) {
}

void MainWindow::setSection(Section section) {
    app_.state().currentSection = section;
}

void MainWindow::requestHeapDemo(int demo, bool pauseAtFirstHighlight) {
    heapVisualizerView_.requestDemo(demo, pauseAtFirstHighlight);
}

void MainWindow::clearResults(AppState& state) {
    app_.engine().clear();
    state.selectedId = kInvalidNodeId;
    state.heapSelectedSlot = -1;
    state.errorMessage.clear();
}

void MainWindow::draw() {
    AppState& state = app_.state();

    if (state.requestedSection.has_value()) {
        setSection(*state.requestedSection);
        state.requestedSection.reset();
    }

    // The scan root can be set before the first frame (--root), and the path
    // field is the single source for what the user is about to scan.
    if (pathBuffer_[0] == '\0' && !state.scanRoot.empty()) {
        const std::string s = state.scanRoot.string();
        std::strncpy(pathBuffer_, s.c_str(), sizeof(pathBuffer_) - 1);
        pathBuffer_[sizeof(pathBuffer_) - 1] = '\0';
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    const ImGuiWindowFlags rootFlags = ImGuiWindowFlags_NoDecoration
                                     | ImGuiWindowFlags_NoMove
                                     | ImGuiWindowFlags_NoResize
                                     | ImGuiWindowFlags_NoSavedSettings
                                     | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("DiskScopeRoot", nullptr, rootFlags);

    drawSidebar(state, ImGui::GetContentRegionAvail().y);

    ImGui::SameLine(0.0f, 0.0f);

    // Content column: toolbar above, view host below. The column owns the outer
    // padding so the sidebar can run edge to edge like the reference.
    ImGui::PushStyleColor(ImGuiCol_ChildBg, theme::colors().bgV);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 10.0f));
    ImGui::BeginChild("ContentColumn", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // Indent instead of padding the column: the toolbar and the view host then
    // share one horizontal inset, so cards and controls line up on the same grid.
    ImGui::Indent(theme::metrics().cardPadding);
    drawToolbar(state, ImGui::GetContentRegionAvail().x);
    ImGui::Unindent(theme::metrics().cardPadding);
    drawContent(state);

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    // Rendered inside the root window so the modal is owned by a live window;
    // it still centres itself on the main viewport.
    const ui::ConfirmResult confirm = ui::ConfirmDialog(
        "ClearResultsDialog", &state.confirmClearResults, "Clear scan results?",
        "This discards the current scan tree and ranking heap. Files on disk are not touched.",
        "Clear results", "Keep results", true);
    if (confirm.confirmed) {
        clearResults(state);
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
}

void MainWindow::drawSidebar(AppState& state, float height) {
    const theme::Metrics& metrics = theme::metrics();
    const theme::Colors& colors = theme::colors();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, colors.sidebarV);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));
    ImGui::BeginChild("Sidebar", ImVec2(metrics.sidebarWidth, height), ImGuiChildFlags_AlwaysUseWindowPadding,
                      ImGuiWindowFlags_NoScrollbar);

    // Brand: mark + wordmark + descriptor.
    {
        const ImVec2 start = ImGui::GetCursorScreenPos();
        ImDrawList* list = ImGui::GetWindowDrawList();
        const float tile = 38.0f;
        list->AddRectFilled(start, ImVec2(start.x + tile, start.y + tile), colors.greenDark, 10.0f);
        list->AddRect(start, ImVec2(start.x + tile, start.y + tile), colors.green, 10.0f, 0, 1.5f);
        ui::DrawIcon(list, ui::Icon::LogoMark,
                     ImVec2(start.x + tile * 0.5f, start.y + tile * 0.5f), 22.0f, colors.green, 1.8f);

        ImGui::SetCursorScreenPos(ImVec2(start.x + tile + 12.0f, start.y + 1.0f));
        ImGui::PushFont(theme::h3());
        ImGui::TextColored(colors.textV, "DiskScope");
        ImGui::PopFont();

        ImGui::SetCursorScreenPos(ImVec2(start.x + tile + 12.0f, start.y + tile - 13.0f));
        ImGui::PushFont(theme::caption());
        ImGui::TextColored(colors.textSecondaryV, "Disk Storage Analyzer");
        ImGui::PopFont();

        ImGui::SetCursorScreenPos(ImVec2(start.x, start.y + tile + 16.0f));
    }

    hairline();
    ImGui::Dummy(ImVec2(0.0f, 10.0f));

    struct NavEntry {
        ui::Icon icon;
        const char* label;
        Section section;
    };
    const NavEntry nav[kSectionCount] = {
        {ui::Icon::Overview, "Overview", Section::Overview},
        {ui::Icon::Folder, "Directory Explorer", Section::DirectoryExplorer},
        {ui::Icon::Chart, "Largest Folders", Section::LargestFolders},
        {ui::Icon::Heap, "Heap Visualizer", Section::HeapVisualizer},
        {ui::Icon::Settings, "Settings", Section::Settings},
    };

    for (int i = 0; i < kSectionCount; ++i) {
        char id[16];
        std::snprintf(id, sizeof(id), "##nav%d", i);
        if (ui::NavItem(id, nav[i].icon, nav[i].label, state.currentSection == nav[i].section)) {
            setSection(nav[i].section);
        }
    }

    // Footer: pinned to the bottom, one status row plus one detail line.
    const float footerHeight = 74.0f;
    const float available = ImGui::GetContentRegionAvail().y;
    if (available > footerHeight) {
        ImGui::Dummy(ImVec2(0.0f, available - footerHeight));
    }

    hairline();
    ImGui::Dummy(ImVec2(0.0f, 8.0f));

    const bool hasResult = app_.engine().hasResult();
    const bool cancelled = hasResult && app_.engine().result().cancelled;
    const bool hasError = !state.errorMessage.empty();

    const char* statusText = "Idle";
    ImU32 statusColour = colors.textSecondary;
    if (state.scanRunning) {
        statusText = "Scanning";
        statusColour = colors.green;
    } else if (hasError) {
        statusText = "Error";
        statusColour = colors.red;
    } else if (cancelled) {
        statusText = "Cancelled";
        statusColour = colors.red;
    } else if (hasResult) {
        statusText = "Ready";
        statusColour = colors.green;
    }

    const ImVec2 rowStart = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(rowStart.x + 4.0f, rowStart.y + ImGui::GetTextLineHeight() * 0.5f),
                                                4.0f, statusColour);
    ImGui::Dummy(ImVec2(12.0f, ImGui::GetTextLineHeight()));
    ImGui::SameLine(0.0f, 4.0f);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(statusColour), "%s", statusText);

    const bool canClear = hasResult && !state.scanRunning;
    const float clearSize = 26.0f;
    ImGui::SetCursorScreenPos(ImVec2(rowStart.x + ImGui::GetContentRegionAvail().x - clearSize, rowStart.y - 3.0f));
    if (ui::IconButton("##clearResults", ui::Icon::Trash, "Clear results", false, canClear, clearSize)) {
        state.confirmClearResults = true;
    }
    ImGui::SetCursorScreenPos(ImVec2(rowStart.x, rowStart.y + ImGui::GetTextLineHeight() + 4.0f));

    ImGui::PushFont(theme::caption());
    if (state.scanRunning) {
        const ScanProgress progress = app_.controller().progress();
        ImGui::TextColored(colors.textSecondaryV, "%s files | %s",
                           format::count(progress.files).c_str(),
                           format::byteSize(progress.bytes).c_str());
    } else if (hasError) {
        ImGui::TextColored(colors.redV, "%s", format::ellipsize(state.errorMessage, 30).c_str());
    } else if (hasResult) {
        const AnalysisStats stats = app_.engine().stats();
        ImGui::TextColored(colors.textSecondaryV, "%s folders indexed",
                           format::count(stats.indexedFolders).c_str());
    } else {
        ImGui::TextColored(colors.textSecondaryV, "No scan loaded");
    }
    ImGui::PopFont();

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void MainWindow::drawToolbar(AppState& state, float width) {
    const theme::Metrics& metrics = theme::metrics();
    const theme::Colors& colors = theme::colors();

    // Row 1: path field + actions. The field gives up width first so the
    // buttons keep their labels at the minimum window size (1024 px).
    const float selectW = 152.0f;
    const float scanW = 96.0f;
    const float cancelW = 108.0f;
    const float rescanW = 112.0f;
    const float buttonGap = 8.0f;
    const float fieldGap = 12.0f;
    const float buttonsWidth = selectW + scanW + cancelW + rescanW + 3.0f * buttonGap;
    const float fieldWidth = std::max(200.0f, width - buttonsWidth - fieldGap);

    const float fieldHeight = 44.0f;
    const float framePadY = std::max(4.0f, (fieldHeight - ImGui::GetTextLineHeight()) * 0.5f);

    ImGui::PushStyleColor(ImGuiCol_FrameBg, colors.elevatedV);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, colors.elevatedHoverV);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, colors.elevatedV);
    ImGui::PushStyleColor(ImGuiCol_Border, colors.borderV);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, metrics.controlRadius);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(36.0f, framePadY));
    ImGui::SetNextItemWidth(fieldWidth);
    ImGui::InputTextWithHint("##ScanPath", "Select or type a folder to analyze", pathBuffer_,
                             sizeof(pathBuffer_));
    const ImVec2 fieldMin = ImGui::GetItemRectMin();
    const ImVec2 fieldMax = ImGui::GetItemRectMax();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(4);

    const float fieldMidY = (fieldMin.y + fieldMax.y) * 0.5f;
    ui::DrawIcon(ImGui::GetWindowDrawList(), ui::Icon::Folder,
                 ImVec2(fieldMin.x + 18.0f, fieldMidY), 17.0f, colors.textSecondary, 1.6f);
    if (pathBuffer_[0] != '\0') {
        // Bare glyph rather than an IconButton: a bordered button inside the field
        // reads as a second control and used to overlap long paths.
        const float clearCx = fieldMax.x - 24.0f;
        ImGui::SetCursorScreenPos(ImVec2(clearCx - 9.0f, fieldMidY - 9.0f));
        ImGui::InvisibleButton("##clearPath", ImVec2(18.0f, 18.0f));
        const bool clearHovered = ImGui::IsItemHovered();
        ui::DrawIcon(ImGui::GetWindowDrawList(), ui::Icon::Close,
                     ImVec2(clearCx, fieldMidY), 12.0f,
                     clearHovered ? colors.text : colors.textSecondary, 1.5f);
        if (clearHovered) {
            ui::Tooltip("Clear path");
        }
        if (ImGui::IsItemClicked()) {
            pathBuffer_[0] = '\0';
            state.scanRoot.clear();
        }
    }
    ImGui::SetCursorScreenPos(ImVec2(fieldMax.x + fieldGap, fieldMin.y));

    const bool running = state.scanRunning;
    const bool hasPath = pathBuffer_[0] != '\0';

    if (ui::Button("##selectFolder", "Select Folder...", ui::ButtonRole::Secondary, ui::ButtonSize::Large,
                   ui::Icon::Folder, selectW)) {
        const auto chosen = diskscope::platform::pickFolder(pathBuffer_);
        if (chosen) {
            const std::string s = chosen->string();
            std::strncpy(pathBuffer_, s.c_str(), sizeof(pathBuffer_) - 1);
            pathBuffer_[sizeof(pathBuffer_) - 1] = '\0';
            state.scanRoot = *chosen;
        }
    }
    ImGui::SameLine(0.0f, buttonGap);
    if (ui::Button("##scan", "Scan", ui::ButtonRole::Primary, ui::ButtonSize::Large, ui::Icon::Play,
                   scanW, hasPath && !running)) {
        state.scanRoot = pathBuffer_;
        app_.startScan(state.scanRoot);
    }
    ImGui::SameLine(0.0f, buttonGap);
    if (ui::Button("##cancel", "Cancel", ui::ButtonRole::Danger, ui::ButtonSize::Large, ui::Icon::Stop,
                   cancelW, running)) {
        app_.cancelScan();
    }
    ImGui::SameLine(0.0f, buttonGap);
    if (ui::Button("##rescan", "Rescan", ui::ButtonRole::Secondary, ui::ButtonSize::Large,
                   ui::Icon::Refresh, rescanW, hasPath && !running)) {
        state.scanRoot = pathBuffer_;
        app_.startScan(state.scanRoot);
    }

    ImGui::Dummy(ImVec2(0.0f, 6.0f));
    drawStatusRow(state, width);
}

void MainWindow::drawStatusRow(const AppState& state, float width) {
    const theme::Colors& colors = theme::colors();
    const bool hasResult = app_.engine().hasResult();

    ImGui::AlignTextToFramePadding();

    if (state.scanRunning) {
        const ScanProgress progress = app_.controller().progress();
        inlineIcon(ui::Icon::Scan, colors.green, theme::metrics().iconSize);

        ImGui::PushFont(theme::body());
        ImGui::TextColored(colors.greenV, "Scanning");
        ImGui::PopFont();

        statusSeparator();
        ImGui::TextColored(colors.textSecondaryV, "%s folders", format::count(progress.folders).c_str());
        statusSeparator();
        ImGui::TextColored(colors.textSecondaryV, "%s files", format::count(progress.files).c_str());
        statusSeparator();
        ImGui::TextColored(colors.textSecondaryV, "%s", format::byteSize(progress.bytes).c_str());

        if (!progress.current.empty()) {
            statusSeparator();
            const float used = ImGui::GetCursorPosX();
            const float room = std::max(80.0f, width - used - 8.0f);
            const std::string current = format::ellipsize(progress.current.string(),
                                                          static_cast<std::size_t>(room / 7.0f));
            ImGui::TextColored(colors.textDisabledV, "%s", current.c_str());
        }

        ImGui::Dummy(ImVec2(0.0f, 3.0f));
        ui::ProgressBar(-1.0f, ui::Tone::Green, 5.0f, nullptr);
        return;
    }

    if (!state.errorMessage.empty()) {
        inlineIcon(ui::Icon::AlertTriangle, colors.red, theme::metrics().iconSize);
        const float used = ImGui::GetCursorPosX();
        const std::string text = format::ellipsize(state.errorMessage,
                                                   static_cast<std::size_t>(std::max(40.0f, width - used) / 7.0f));
        ImGui::TextColored(colors.redV, "%s", text.c_str());
        return;
    }

    if (!hasResult) {
        inlineIcon(ui::Icon::Info, colors.textSecondary, theme::metrics().iconSize);
        ImGui::TextColored(colors.textSecondaryV,
                           "Select a folder and press Scan to analyze disk space.");
        return;
    }

    const AnalysisStats stats = app_.engine().stats();
    if (stats.cancelled) {
        inlineIcon(ui::Icon::XCircle, colors.red, theme::metrics().iconSize);
        ImGui::TextColored(colors.redV, "Scan cancelled");
        statusSeparator();
        ImGui::TextColored(colors.textSecondaryV, "partial results, totals are lower bounds");
        return;
    }

    inlineIcon(ui::Icon::CheckCircle, colors.green, theme::metrics().iconSize);
    ImGui::TextColored(colors.greenV, "Scan complete:");
    statusSeparator();
    ImGui::TextColored(colors.textV, "%s", format::byteSize(stats.totalBytes).c_str());
    statusSeparator();
    ImGui::TextColored(colors.textSecondaryV, "%s files", format::count(stats.fileCount).c_str());
    statusSeparator();
    ImGui::TextColored(colors.textSecondaryV, "%s folders", format::count(stats.folderCount).c_str());
    if (stats.errorCount > 0) {
        statusSeparator();
        ImGui::TextColored(colors.redV, "%s unreadable", format::count(stats.errorCount).c_str());
    }
}

void MainWindow::drawContent(AppState& state) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        ImVec2(theme::metrics().cardPadding, theme::metrics().gap));
    ImGui::BeginChild("ViewContainer", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    switch (state.section()) {
        case Section::Overview:
            dashboardView_.draw(state);
            break;
        case Section::DirectoryExplorer:
            directoryExplorerView_.draw(state);
            break;
        case Section::LargestFolders:
            largestFoldersView_.draw(state);
            break;
        case Section::HeapVisualizer:
            heapVisualizerView_.draw(state);
            break;
        case Section::Settings:
            settingsView_.draw(state);
            break;
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
}

}  // namespace diskscope

#include "ui/SettingsView.h"

#include <cstdio>
#include <string>

#include <imgui.h>

#include "services/AnalysisEngine.h"
#include "ui/AppState.h"
#include "ui/Components.h"
#include "ui/Icons.h"
#include "ui/Theme.h"
#include "utils/Formatting.h"

namespace diskscope {
namespace {

constexpr float kAutoHeight = 0.0f;

ui::CardOptions cardOptions(const char* title, ui::Icon icon) {
    ui::CardOptions options;
    options.title = title;
    options.icon = icon;
    options.height = kAutoHeight;
    options.autoHeight = true;
    return options;
}

void bodyParagraph(const char* text) {
    ImGui::PushFont(theme::caption());
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);
    ImGui::TextColored(theme::colors().textSecondaryV, "%s", text);
    ImGui::PopTextWrapPos();
    ImGui::PopFont();
}

void bullet(const char* text) {
    const theme::Colors& colors = theme::colors();
    const ImVec2 start = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(start.x + 3.0f, start.y + ImGui::GetTextLineHeight() * 0.5f),
                                                2.0f, colors.green);
    ImGui::Dummy(ImVec2(12.0f, ImGui::GetTextLineHeight()));
    ImGui::SameLine(0.0f, 4.0f);
    ImGui::PushFont(theme::caption());
    ImGui::PushTextWrapPos(start.x + ImGui::GetContentRegionAvail().x);
    ImGui::TextColored(colors.textSecondaryV, "%s", text);
    ImGui::PopTextWrapPos();
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(0.0f, 2.0f));
}

// A palette row states the role, the literal hex and what the colour means, so
// the reference table in the brief is verifiable inside the app.
void paletteRow(const char* role, const char* meaning, ImU32 colour, float width) {
    const theme::Colors& colors = theme::colors();
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const float rowH = 20.0f;

    ImDrawList* list = ImGui::GetWindowDrawList();
    // Near-black chips need a frame of their own, otherwise they read as empty
    // checkboxes on a panel of almost the same colour.
    list->AddRectFilled(ImVec2(start.x, start.y + 4.0f), ImVec2(start.x + 13.0f, start.y + 17.0f), colour, 3.0f);
    list->AddRect(ImVec2(start.x, start.y + 4.0f), ImVec2(start.x + 13.0f, start.y + 17.0f),
                  theme::withAlpha(colors.textSecondary, 0.45f), 3.0f, 0, 1.0f);

    list->AddText(theme::body(), theme::body()->FontSize, ImVec2(start.x + 20.0f, start.y + 1.0f), colors.text, role);

    const ImVec4 rgba = ImGui::ColorConvertU32ToFloat4(colour);
    char hex[16];
    std::snprintf(hex, sizeof(hex), "#%02X%02X%02X", static_cast<int>(rgba.x * 255.0f + 0.5f),
                  static_cast<int>(rgba.y * 255.0f + 0.5f), static_cast<int>(rgba.z * 255.0f + 0.5f));
    list->AddText(theme::mono(), theme::mono()->FontSize, ImVec2(start.x + 150.0f, start.y + 1.0f),
                  colors.textSecondary, hex);

    list->AddText(theme::caption(), theme::caption()->FontSize, ImVec2(start.x + 214.0f, start.y + 2.0f),
                  colors.textSecondary, meaning);

    ImGui::Dummy(ImVec2(width, rowH));
}

}  // namespace

void SettingsView::drawScanningCard(AppState& state) {
    if (ui::BeginCard("##settingsScanning", cardOptions("Scanning", ui::Icon::Scan))) {
        ui::Switch("Include hidden files and folders", &state.includeHidden,
                   "Walk dotfiles and directories marked hidden. Takes effect on the next scan.");
        ImGui::Dummy(ImVec2(0.0f, 8.0f));
        ui::Switch("Measure allocated size on disk", &state.measureAllocatedSize,
                   "Query cluster allocation per file so the on-disk figures account for block "
                   "rounding, compression and sparse files.");
        ImGui::Dummy(ImVec2(0.0f, 8.0f));
        bodyParagraph("Both options are read when a scan starts, so the current results stay untouched "
                      "until you run Rescan.");
    }
    ui::EndCard();
}

void SettingsView::drawRankingCard(AppState& state) {
    if (ui::BeginCard("##settingsRanking", cardOptions("Rankings", ui::Icon::Chart))) {
        if (ui::Switch("Include the scan root in rankings", &state.includeRoot,
                       "The root is trivially the largest folder in its own scan, so it is excluded by "
                       "default. Toggling this rebuilds the ranking heap immediately.")) {
            if (state.engine && state.engine->hasResult()) {
                state.engine->rebuildRankings(state.includeRoot);
            }
        }

        ImGui::Dummy(ImVec2(0.0f, 8.0f));
        const bool ready = state.engine && state.engine->hasResult();
        const std::uintmax_t indexed = ready ? state.engine->stats().indexedFolders : 0;
        ImGui::PushFont(theme::caption());
        ImGui::TextColored(theme::colors().textSecondaryV, "Currently indexed in the heap: %s folders",
                           format::count(indexed).c_str());
        ImGui::PopFont();
    }
    ui::EndCard();
}

void SettingsView::drawVisualizerCard(AppState& state) {
    if (ui::BeginCard("##settingsVisualizer", cardOptions("Heap visualizer", ui::Icon::Heap))) {
        ImGui::TextColored(theme::colors().textV, "Playback speed");
        ImGui::SameLine(0.0f, theme::metrics().gap);
        ui::Stepper("##fps", &state.demoFramesPerSecond, 1.0f, 30.0f, 1.0f, "%.0f fps", 110.0f);
        ImGui::Dummy(ImVec2(0.0f, 6.0f));
        bodyParagraph("Demos replay recorded frames of the real max heap on a copy of the current "
                      "folders. The recording is capped at 63 nodes (six levels) so every node stays "
                      "legible.");
    }
    ui::EndCard();
}

void SettingsView::drawPaletteCard() {
    if (ui::BeginCard("##settingsPalette", cardOptions("Appearance", ui::Icon::Layers))) {
        const theme::Colors& colors = theme::colors();
        const float width = ImGui::GetContentRegionAvail().x;

        paletteRow("Background", "application surface", colors.bg, width);
        paletteRow("Sidebar", "navigation rail", colors.sidebar, width);
        paletteRow("Panel", "cards and tables", colors.panel, width);
        paletteRow("Elevated", "inputs, meters, hover", colors.elevated, width);
        paletteRow("Border", "hairlines and outlines", colors.border, width);
        paletteRow("Primary green", "actions and selection", colors.green, width);
        paletteRow("Green hover", "hover and active accents", colors.greenHover, width);
        paletteRow("Dark green", "selected surfaces", colors.greenDark, width);
        paletteRow("Primary red", "destructive and errors", colors.red, width);
        paletteRow("Dark red", "destructive surfaces", colors.redDark, width);
        paletteRow("Text", "primary copy", colors.text, width);
        paletteRow("Secondary text", "captions and metadata", colors.textSecondary, width);

        ImGui::Dummy(ImVec2(0.0f, 8.0f));
        bodyParagraph("There is deliberately no amber: in-progress and healthy states are green, while "
                      "destructive and error states are red. Typography comes from Segoe UI with Cascadia "
                      "Mono for figures, both with built-in fallbacks.");
    }
    ui::EndCard();
}

void SettingsView::drawLimitationsCard() {
    if (ui::BeginCard("##settingsLimitations", cardOptions("Known limitations", ui::Icon::Info))) {
        bullet("Symbolic links and junctions are never followed, so a link loop cannot trap the scanner; "
               "each skipped link is counted.");
        bullet("Hard-linked files are counted once per directory entry, not once per physical inode.");
        bullet("A cancelled scan returns a consistent but partial tree: its totals are lower bounds.");
        bullet("Unreadable entries are reported and skipped rather than estimated.");
        bullet("Recursion stops at depth 256 and at most 500 errors are retained per scan.");
    }
    ui::EndCard();
}

void SettingsView::drawAboutCard(AppState& state) {
    if (ui::BeginCard("##settingsAbout", cardOptions("About DiskScope", ui::Icon::Info))) {
        const theme::Colors& colors = theme::colors();
        ui::KeyValueRow("Version", "1.0.0", 96.0f);
        ui::KeyValueRow("Interface", "Dear ImGui 1.91.9b + ImPlot 0.17", 96.0f);
        ui::KeyValueRow("Platform", "GLFW 3.4 / OpenGL 3.3 core", 96.0f);
        ui::KeyValueRow("Data structures", "N-ary directory tree + hand-written max heap", 96.0f);

        ImGui::Dummy(ImVec2(0.0f, 6.0f));
        bodyParagraph("Every size shown in this application is aggregated post-order over the directory "
                      "tree, and every ranking comes from the custom binary heap. No library heap "
                      "algorithm is used anywhere.");

        if (state.scanRoot.empty()) {
            return;
        }
        ImGui::Dummy(ImVec2(0.0f, 6.0f));
        ImGui::PushFont(theme::caption());
        ImGui::TextColored(colors.textSecondaryV, "Scan root: %s",
                           format::ellipsize(state.scanRoot.string(), 72).c_str());
        ImGui::PopFont();
    }
    ui::EndCard();
}

void SettingsView::draw(AppState& state) {
    ui::SectionHeader(ui::Icon::Settings, "Settings",
                      "Scan behaviour, ranking options, visualizer playback and the palette reference");

    ImGui::BeginChild("##settingsScroll", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None, ImGuiWindowFlags_None);

    const float width = ImGui::GetContentRegionAvail().x;
    const float gap = theme::metrics().gap;
    // Two columns only when they are wide enough to keep the captions readable.
    const bool twoColumns = width >= 880.0f;
    const float columnW = twoColumns ? (width - gap) * 0.5f : width;

    if (twoColumns) {
        if (ImGui::BeginTable("##settingsColumns", 2, ImGuiTableFlags_SizingFixedFit |
                                                        ImGuiTableFlags_NoPadOuterX |
                                                        ImGuiTableFlags_NoPadInnerX)) {
            ImGui::TableSetupColumn("##left", ImGuiTableColumnFlags_WidthFixed, columnW);
            ImGui::TableSetupColumn("##right", ImGuiTableColumnFlags_WidthFixed, columnW);
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::PushItemWidth(columnW);
            ImGui::BeginChild("##leftCol", ImVec2(columnW, 0.0f), ImGuiChildFlags_None,
                              ImGuiWindowFlags_NoScrollbar);
            drawScanningCard(state);
            ImGui::Dummy(ImVec2(0.0f, gap));
            drawVisualizerCard(state);
            ImGui::Dummy(ImVec2(0.0f, gap));
            drawAboutCard(state);
            ImGui::EndChild();
            ImGui::PopItemWidth();

            ImGui::TableNextColumn();
            ImGui::PushItemWidth(columnW);
            ImGui::BeginChild("##rightCol", ImVec2(columnW, 0.0f), ImGuiChildFlags_None,
                              ImGuiWindowFlags_NoScrollbar);
            drawRankingCard(state);
            ImGui::Dummy(ImVec2(0.0f, gap));
            drawPaletteCard();
            ImGui::Dummy(ImVec2(0.0f, gap));
            drawLimitationsCard();
            ImGui::EndChild();
            ImGui::PopItemWidth();

            ImGui::EndTable();
        }
    } else {
        drawScanningCard(state);
        ImGui::Dummy(ImVec2(0.0f, gap));
        drawRankingCard(state);
        ImGui::Dummy(ImVec2(0.0f, gap));
        drawVisualizerCard(state);
        ImGui::Dummy(ImVec2(0.0f, gap));
        drawPaletteCard();
        ImGui::Dummy(ImVec2(0.0f, gap));
        drawLimitationsCard();
        ImGui::Dummy(ImVec2(0.0f, gap));
        drawAboutCard(state);
    }

    ImGui::EndChild();
}

}  // namespace diskscope

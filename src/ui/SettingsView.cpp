#include "ui/SettingsView.h"

#include <string>

#include <imgui.h>

#include "services/AnalysisEngine.h"
#include "ui/AppState.h"
#include "ui/Theme.h"
#include "utils/Formatting.h"

namespace diskscope {

namespace {

void beginCard(const char* id, const char* title) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, theme::colors().cardV);
    ImGui::PushStyleColor(ImGuiCol_Border, theme::colors().borderV);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

    ImGui::BeginChild(id, ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar);
    ImGui::PushFont(theme::heading());
    ImGui::TextUnformatted(title);
    ImGui::PopFont();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

void endCard() {
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
    ImGui::Spacing();
}

}  // namespace

void SettingsView::draw(AppState& state) {
    (void)selectedTopKIndex_;

    // Card 1: Scan options
    beginCard("CardScanOptions", "Scan options");
    ImGui::Checkbox("Include hidden files and folders", &state.includeHidden);
    ImGui::TextColored(theme::colors().textMutedV, "Scan dotfiles and hidden directories (applied on the next scan).");
    ImGui::Spacing();
    ImGui::Checkbox("Measure allocated size on disk", &state.measureAllocatedSize);
    ImGui::TextColored(theme::colors().textMutedV, "Query cluster allocation to measure actual space on disk (applied on the next scan).");
    endCard();

    // Card 2: Rankings
    beginCard("CardRankings", "Rankings");
    if (ImGui::Checkbox("Include the scan root in rankings", &state.includeRoot)) {
        if (state.engine && state.engine->hasResult()) {
            state.engine->rebuildRankings(state.includeRoot);
        }
    }
    ImGui::TextColored(theme::colors().textMutedV, "When enabled, the root directory itself is ranked alongside subfolders.");
    ImGui::Spacing();
    const std::uintmax_t indexedCount = (state.engine && state.engine->hasResult()) ? state.engine->stats().indexedFolders : 0;
    ImGui::TextColored(theme::colors().textMutedV, "Currently indexed folders: %s", format::count(indexedCount).c_str());
    endCard();

    // Card 3: Heap visualizer
    beginCard("CardHeapVisualizer", "Heap visualizer");
    ImGui::SetNextItemWidth(260.0f);
    ImGui::SliderFloat("Playback speed", &state.demoFramesPerSecond, 1.0f, 30.0f, "%.0f fps");
    ImGui::Spacing();
    ImGui::TextColored(theme::colors().textMutedV, "Heap visualization replays the real heap on at most 63 nodes (six levels).");
    endCard();

    // Card 4: Appearance
    beginCard("CardAppearance", "Appearance");
    auto drawSwatch = [](const char* id, const ImVec4& color, const char* label) {
        ImGui::ColorButton(id, color, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoDragDrop, ImVec2(18.0f, 18.0f));
        ImGui::SameLine(0.0f, 8.0f);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
    };
    drawSwatch("##swatch_accent", theme::colors().accentV, "Accent — Interactive highlights and active selections");
    drawSwatch("##swatch_good", theme::colors().goodV, "Good — Ready status and normal indications");
    drawSwatch("##swatch_warn", theme::colors().warnV, "Warning — In-progress scans and non-fatal warnings");
    drawSwatch("##swatch_bad", theme::colors().badV, "Bad — Scan errors and unreadable entries");
    ImGui::Spacing();
    ImGui::TextColored(theme::colors().textMutedV, "Fonts come from the Windows font directory with a built-in fallback.");
    endCard();

    // Card 5: Known limitations
    beginCard("CardLimitations", "Known limitations");
    ImGui::TextColored(theme::colors().textMutedV, "• Symbolic links and junctions are not followed to avoid recursion loops.");
    ImGui::Spacing();
    ImGui::TextColored(theme::colors().textMutedV, "• Hard-linked files are counted once per directory name reference.");
    ImGui::Spacing();
    ImGui::TextColored(theme::colors().textMutedV, "• Cancelled scans are lower bounds based on visited directories.");
    endCard();
}

}  // namespace diskscope

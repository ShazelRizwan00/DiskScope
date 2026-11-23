#include "ui/HeapVisualizerView.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include <imgui.h>

#include "core/FolderNode.h"
#include "services/AnalysisEngine.h"
#include "services/HeapDemoEngine.h"
#include "structures/HeapEvent.h"
#include "ui/AppState.h"
#include "ui/Theme.h"
#include "utils/Formatting.h"
#include "visualization/HeapRenderer.h"

namespace diskscope {
namespace {

const char* kindLabel(HeapEventKind kind) {
    switch (kind) {
        case HeapEventKind::Build:
            return "build";
        case HeapEventKind::Compare:
            return "compare";
        case HeapEventKind::Swap:
            return "swap";
        case HeapEventKind::SiftUp:
            return "sift up";
        case HeapEventKind::SiftDown:
            return "sift down";
        case HeapEventKind::Insert:
            return "insert";
        case HeapEventKind::Extract:
            return "extract";
        case HeapEventKind::Done:
            return "done";
    }
    return "step";
}

void legendChip(const char* label, ImU32 colour) {
    ImGui::PushStyleColor(ImGuiCol_Button, colour);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colour);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, colour);
    ImGui::Button("##chip", ImVec2(12.0f, 12.0f));
    ImGui::PopStyleColor(3);
    ImGui::SameLine(0.0f, 6.0f);
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(theme::colors().textMutedV, "%s", label);
    ImGui::SameLine(0.0f, 18.0f);
}

void cardBegin(const char* id, float height = 0.0f) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, theme::colors().cardV);
    ImGui::PushStyleColor(ImGuiCol_Border, theme::colors().borderV);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::BeginChild(id, ImVec2(0.0f, height), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar);
}

void cardEnd() {
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

}  // namespace

void HeapVisualizerView::draw(AppState& state) {
    // ---- empty states ----------------------------------------------------
    if (!state.engine || !state.engine->hasResult()) {
        ImGui::PushFont(theme::heading());
        ImGui::TextUnformatted("Heap Visualizer");
        ImGui::PopFont();
        ImGui::Spacing();
        ImGui::TextColored(theme::colors().textMutedV,
                           "No scan loaded. Scan a folder to build the max heap that ranks its folders.");
        return;
    }

    const std::vector<const FolderNode*>& liveItems = state.engine->heap().data();
    if (liveItems.empty()) {
        ImGui::PushFont(theme::heading());
        ImGui::TextUnformatted("Heap Visualizer");
        ImGui::PopFont();
        ImGui::Spacing();
        ImGui::TextColored(theme::colors().textMutedV,
                           "The ranking heap is empty: this scan found no subfolders to rank.");
        return;
    }

    // ---- react to a rescan ----------------------------------------------
    if (cachedGeneration_ != state.engine->generation()) {
        cachedGeneration_ = state.engine->generation();
        frames_.clear();
        frameIndex_ = 0;
        playing_ = false;
        hasDemo_ = false;
        accumulator_ = 0.0f;
        lastAction_.clear();
        demoItems_ = HeapDemoEngine::selectDemoItems(liveItems, HeapDemoEngine::kDefaultDemoCap);
    }
    if (demoItems_.empty()) {
        demoItems_ = HeapDemoEngine::selectDemoItems(liveItems, HeapDemoEngine::kDefaultDemoCap);
    }

    // ---- playback clock --------------------------------------------------
    if (playing_ && !frames_.empty()) {
        accumulator_ += ImGui::GetIO().DeltaTime * std::max(state.demoFramesPerSecond, 1.0f);
        while (accumulator_ >= 1.0f && frameIndex_ + 1 < frames_.size()) {
            accumulator_ -= 1.0f;
            ++frameIndex_;
        }
        if (frameIndex_ + 1 >= frames_.size()) {
            playing_ = false;
            accumulator_ = 0.0f;
        }
    }

    // ---- controls --------------------------------------------------------
    // The demo buttons only queue a request. Rebuilding frames_ inside a button
    // handler would invalidate the frame/array that this same draw renders, so
    // the request is executed after the controls card, before frame selection.
    HeapDemoEngine demos;
    cardBegin("HeapControls", 116.0f);
    {
        const bool hasFrames = !frames_.empty();

        if (ImGui::Button("Build heap", ImVec2(104.0f, 26.0f))) {
            pendingDemo_ = 0;
        }
        ImGui::SameLine(0.0f, 6.0f);
        if (ImGui::Button("Insert folder", ImVec2(104.0f, 26.0f))) {
            pendingDemo_ = 1;
        }
        ImGui::SameLine(0.0f, 6.0f);
        if (ImGui::Button("Extract max", ImVec2(104.0f, 26.0f))) {
            pendingDemo_ = 2;
        }

        ImGui::SameLine(0.0f, 20.0f);
        if (!hasFrames) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button(playing_ ? "Pause" : "Play", ImVec2(64.0f, 26.0f))) {
            playing_ = !playing_;
        }
        ImGui::SameLine(0.0f, 6.0f);
        if (ImGui::Button("Step", ImVec2(56.0f, 26.0f))) {
            playing_ = false;
            if (frameIndex_ + 1 < frames_.size()) {
                ++frameIndex_;
            }
        }
        ImGui::SameLine(0.0f, 6.0f);
        if (ImGui::Button("Reset", ImVec2(64.0f, 26.0f))) {
            playing_ = false;
            frameIndex_ = 0;
            accumulator_ = 0.0f;
        }
        if (!hasFrames) {
            ImGui::EndDisabled();
        }

        ImGui::SameLine(0.0f, 20.0f);
        ImGui::SetNextItemWidth(150.0f);
        ImGui::SliderFloat("fps##speed", &state.demoFramesPerSecond, 1.0f, 30.0f, "%.0f fps");

        ImGui::Spacing();
        ImGui::TextColored(theme::colors().textMutedV,
                           "Demos replay the real MaxHeap on a copy of these folders; nothing here is scripted.");
    }
    cardEnd();
    ImGui::Spacing();

    // ---- run a queued demo on real folders -------------------------------
    if (pendingDemo_ >= 0) {
        if (pendingDemo_ == 0) {
            std::vector<const FolderNode*> items =
                HeapDemoEngine::selectDemoItems(liveItems, HeapDemoEngine::kDefaultDemoCap);
            // Reverse order guarantees the bottom-up build has real work to show.
            std::reverse(items.begin(), items.end());
            frames_ = demos.recordBuild(items);
            selectedOperation_ = 0;
            lastAction_ = "Bottom-up buildHeap over " + std::to_string(items.size()) + " folders";
        } else if (pendingDemo_ == 1) {
            // cap - 1 items so the inserted folder cannot add a seventh level.
            std::vector<const FolderNode*> base =
                HeapDemoEngine::selectDemoItems(liveItems, HeapDemoEngine::kDefaultDemoCap - 1);
            const FolderNode* incoming = nullptr;
            if (liveItems.size() > base.size()) {
                incoming = liveItems[base.size()];
            } else {
                incoming = liveItems.front(); // duplicate of the largest: shows a full sift to the root
            }
            frames_ = demos.recordInsert(base, incoming);
            selectedOperation_ = 1;
            lastAction_ = incoming != nullptr
                              ? ("Insert '" + format::ellipsize(incoming->name, 24) + "' followed by sift-up")
                              : "Insert";
        } else {
            frames_ = demos.recordExtract(HeapDemoEngine::selectDemoItems(liveItems, HeapDemoEngine::kDefaultDemoCap));
            selectedOperation_ = 2;
            lastAction_ = "Extract the maximum, then sift-down from the new root";
        }
        frameIndex_ = 0;
        accumulator_ = 0.0f;
        playing_ = true;
        hasDemo_ = true;
        pendingDemo_ = -1;
    }

    // ---- select the frame to render (always after any frames_ mutation) ---
    const bool hasFrames = !frames_.empty();
    const HeapEvent* frame = hasFrames ? &frames_[std::min(frameIndex_, frames_.size() - 1)] : nullptr;
    const std::vector<const FolderNode*>& array = (frame != nullptr) ? frame->array : demoItems_;
    const std::size_t highlightA = frame != nullptr ? frame->a : kNoIndex;
    const std::size_t highlightB = frame != nullptr ? frame->b : kNoIndex;
    const HeapEventKind kind = frame != nullptr ? frame->kind : HeapEventKind::Done;

    // ---- status line -----------------------------------------------------
    {
        const std::string frameText = hasFrames ? (std::to_string(frameIndex_ + 1) + " / " +
                                                   std::to_string(frames_.size()))
                                                : std::string("static");
        std::string status = lastAction_.empty() ? "Showing the current heap (press a demo button to animate)." : lastAction_;
        ImGui::TextColored(theme::colors().textMutedV, "%s", format::ellipsize(status, 96).c_str());
        if (frame != nullptr) {
            ImGui::SameLine();
            ImGui::TextColored(theme::colors().accentV, "·  frame %s · %s", frameText.c_str(), kindLabel(kind));
            if (!frame->note.empty()) {
                ImGui::SameLine();
                ImGui::TextColored(theme::colors().textMutedV, "· %s", frame->note.c_str());
            }
        }
    }
    ImGui::Spacing();
    // ---- tree canvas -----------------------------------------------------
    const ImVec2 canvasSize(ImGui::GetContentRegionAvail().x, 330.0f);
    const ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##heapTreeCanvas", canvasSize);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 canvasEnd(canvasOrigin.x + canvasSize.x, canvasOrigin.y + canvasSize.y);
    drawList->AddRectFilled(canvasOrigin, canvasEnd, theme::colors().card, 6.0f);
    drawList->AddRect(canvasOrigin, canvasEnd, theme::colors().border, 6.0f);
    constexpr float kCanvasPad = 16.0f;
    viz::drawHeapTree(drawList, array, highlightA, highlightB, kind,
                      ImVec2(canvasOrigin.x + kCanvasPad, canvasOrigin.y + kCanvasPad),
                      ImVec2(canvasSize.x - 2.0f * kCanvasPad, canvasSize.y - 2.0f * kCanvasPad));

    ImGui::Spacing();

    // ---- array representation -------------------------------------------
    ImGui::TextColored(theme::colors().textMutedV, "Level-order array representation");
    constexpr float kStripHeight = 54.0f;
    constexpr float kBoxWidth = 78.0f;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, theme::colors().cardV);
    ImGui::PushStyleColor(ImGuiCol_Border, theme::colors().borderV);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
    ImGui::BeginChild("##arrayStrip", ImVec2(0.0f, kStripHeight + 26.0f), ImGuiChildFlags_Borders,
                      ImGuiWindowFlags_HorizontalScrollbar);
    const ImVec2 stripOrigin = ImGui::GetCursorScreenPos();
    viz::drawHeapArray(ImGui::GetWindowDrawList(), array, highlightA, highlightB, kind, stripOrigin, kStripHeight);
    ImGui::Dummy(ImVec2(kBoxWidth * static_cast<float>(array.size()) + 8.0f, kStripHeight));
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    ImGui::Spacing();

    // ---- legend / summary ------------------------------------------------
    legendChip("root", theme::colors().accentSoft);
    legendChip("compare", theme::colors().warn);
    legendChip("swap", theme::colors().good);
    legendChip("insert", theme::colors().accent);
    legendChip("extract", theme::colors().bad);
    ImGui::NewLine();

    const std::string sizing = (array.size() < liveItems.size())
                                   ? format::count(array.size()) + " of " + format::count(liveItems.size()) + " folders shown"
                                   : format::count(array.size()) + " folders";
    ImGui::TextColored(theme::colors().textMutedV, "Heap size: %s · %s · root = largest folder",
                       format::count(liveItems.size()).c_str(), sizing.c_str());
    if (liveItems.size() > HeapDemoEngine::kDefaultDemoCap) {
        ImGui::TextColored(theme::colors().textMutedV,
                           "Demos are capped at %zu nodes (six levels) so the animation stays readable.",
                           static_cast<std::size_t>(HeapDemoEngine::kDefaultDemoCap));
    }
}

}  // namespace diskscope

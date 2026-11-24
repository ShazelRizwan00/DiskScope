#include "ui/HeapVisualizerView.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include <imgui.h>

#include "services/AnalysisEngine.h"
#include "services/HeapDemoEngine.h"
#include "structures/HeapEvent.h"
#include "ui/AppState.h"
#include "ui/Components.h"
#include "ui/Icons.h"
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

bool isOperationFrame(HeapEventKind kind) {
    return kind == HeapEventKind::Compare || kind == HeapEventKind::Swap ||
           kind == HeapEventKind::SiftUp || kind == HeapEventKind::SiftDown ||
           kind == HeapEventKind::Insert || kind == HeapEventKind::Extract;
}

void legendChip(const char* label, ImU32 fill, ImU32 border) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float lineH = ImGui::GetTextLineHeight();
    ImDrawList* list = ImGui::GetWindowDrawList();
    list->AddRectFilled(ImVec2(p.x, p.y + lineH * 0.5f - 5.0f), ImVec2(p.x + 10.0f, p.y + lineH * 0.5f + 5.0f),
                        fill, 3.0f);
    list->AddRect(ImVec2(p.x, p.y + lineH * 0.5f - 5.0f), ImVec2(p.x + 10.0f, p.y + lineH * 0.5f + 5.0f),
                  border, 3.0f, 0, 1.0f);
    ImGui::Dummy(ImVec2(13.0f, lineH));
    ImGui::SameLine(0.0f, 5.0f);
    ImGui::PushFont(theme::caption());
    ImGui::TextColored(theme::colors().textSecondaryV, "%s", label);
    ImGui::PopFont();
    ImGui::SameLine(0.0f, 16.0f);
}

}  // namespace

void HeapVisualizerView::requestDemo(int demo, bool pauseAtFirstHighlight) {
    pendingDemo_ = demo;
    stopAtHighlight_ = pauseAtFirstHighlight;
}

void HeapVisualizerView::drawControls(AppState& state) {
    const bool hasFrames = !frames_.empty();

    // The full row needs ~824 px. Below that the playback cluster moves to its own
    // row instead of running off the right edge at the minimum window width.
    const float required = 118.0f + 8.0f + 130.0f + 8.0f + 122.0f + 20.0f + 88.0f + 8.0f + 78.0f +
                           8.0f + 84.0f + 20.0f + 28.0f + 8.0f + 96.0f;
    const bool singleRow = ImGui::GetContentRegionAvail().x >= required;

    if (ui::Button("##buildHeap", "Build Heap", ui::ButtonRole::Primary, ui::ButtonSize::Large,
                   ui::Icon::Heap, 118.0f)) {
        pendingDemo_ = 0;
        stopAtHighlight_ = false;
    }
    ImGui::SameLine(0.0f, 8.0f);
    if (ui::Button("##insertFolder", "Insert Folder", ui::ButtonRole::Secondary, ui::ButtonSize::Large,
                   ui::Icon::ArrowRight, 130.0f)) {
        pendingDemo_ = 1;
        stopAtHighlight_ = false;
    }
    ImGui::SameLine(0.0f, 8.0f);
    if (ui::Button("##extractMax", "Extract Max", ui::ButtonRole::Secondary, ui::ButtonSize::Large,
                   ui::Icon::Stop, 122.0f)) {
        pendingDemo_ = 2;
        stopAtHighlight_ = false;
    }

    if (singleRow) {
        ImGui::SameLine(0.0f, 20.0f);
    } else {
        ImGui::Dummy(ImVec2(0.0f, 2.0f));  // playback cluster starts its own row
    }
    if (ui::Button("##playPause", playing_ ? "Pause" : "Play", ui::ButtonRole::Secondary,
                   ui::ButtonSize::Large, playing_ ? ui::Icon::Pause : ui::Icon::Play, 88.0f,
                   hasFrames)) {
        if (hasFrames) {
            playing_ = !playing_;
            accumulator_ = 0.0f;
        }
    }
    ImGui::SameLine(0.0f, 8.0f);
    if (ui::Button("##stepFrame", "Step", ui::ButtonRole::Secondary, ui::ButtonSize::Large,
                   ui::Icon::StepForward, 78.0f, hasFrames)) {
        playing_ = false;
        if (frameIndex_ + 1 < frames_.size()) {
            ++frameIndex_;
            animT_ = 0.0f;
        }
    }
    ImGui::SameLine(0.0f, 8.0f);
    if (ui::Button("##resetFrames", "Reset", ui::ButtonRole::Secondary, ui::ButtonSize::Large,
                   ui::Icon::Reset, 84.0f, hasFrames)) {
        playing_ = false;
        frameIndex_ = 0;
        accumulator_ = 0.0f;
        animT_ = 0.0f;
    }

    ImGui::SameLine(0.0f, singleRow ? 20.0f : 12.0f);
    ImGui::AlignTextToFramePadding();
    ImGui::PushFont(theme::caption());
    ImGui::TextColored(theme::colors().textSecondaryV, "FPS");
    ImGui::PopFont();
    ImGui::SameLine(0.0f, 8.0f);
    ui::Stepper("##heapFps", &state.demoFramesPerSecond, 1.0f, 30.0f, 1.0f, "%.0f", 96.0f);
    (void)required;

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    ImGui::PushFont(theme::caption());
    ImGui::TextColored(theme::colors().textSecondaryV,
                       "Demos replay the real MaxHeap on a copy of these folders; nothing here is scripted.");
    ImGui::PopFont();
}

void HeapVisualizerView::runPendingDemo(const std::vector<const FolderNode*>& liveItems) {
    if (pendingDemo_ < 0) {
        return;
    }

    HeapDemoEngine demos;
    const bool pauseLater = stopAtHighlight_;

    if (pendingDemo_ == 0) {
        std::vector<const FolderNode*> items =
            HeapDemoEngine::selectDemoItems(liveItems, HeapDemoEngine::kDefaultDemoCap);
        // Reverse order so the bottom-up build has real work to show.
        std::reverse(items.begin(), items.end());
        frames_ = demos.recordBuild(items);
        lastAction_ = "Bottom-up buildHeap over " + std::to_string(items.size()) + " folders";
    } else if (pendingDemo_ == 1) {
        // cap - 1 items so the inserted folder cannot add a seventh level.
        std::vector<const FolderNode*> base =
            HeapDemoEngine::selectDemoItems(liveItems, HeapDemoEngine::kDefaultDemoCap - 1);
        const FolderNode* incoming = nullptr;
        if (liveItems.size() > base.size()) {
            incoming = liveItems[base.size()];
        } else {
            incoming = liveItems.front();  // duplicate of the largest: a full sift to the root
        }
        frames_ = demos.recordInsert(base, incoming);
        lastAction_ = incoming != nullptr
                          ? ("Insert '" + format::ellipsize(incoming->name, 24) + "' followed by sift-up")
                          : "Insert";
    } else {
        frames_ = demos.recordExtract(
            HeapDemoEngine::selectDemoItems(liveItems, HeapDemoEngine::kDefaultDemoCap));
        lastAction_ = "Extract the maximum, then sift-down from the new root";
    }

    frameIndex_ = 0;
    accumulator_ = 0.0f;
    animT_ = 0.0f;
    arrayScroll_ = 0.0f;
    playing_ = true;
    pendingDemo_ = -1;
    stopAtHighlight_ = false;

    if (pauseLater && frames_.size() > 1) {
        // Deterministic offline capture: land on the first real operation rather
        // than hoping the clock advanced far enough.
        for (std::size_t i = 1; i < frames_.size(); ++i) {
            if (isOperationFrame(frames_[i].kind)) {
                frameIndex_ = i;
                break;
            }
        }
        playing_ = false;
        animT_ = 1.0f;
    }
}

const HeapEvent* HeapVisualizerView::currentFrame() const {
    return frames_.empty() ? nullptr : &frames_[std::min(frameIndex_, frames_.size() - 1)];
}

viz::HeapDrawState HeapVisualizerView::makeDrawState(std::size_t selectedSlot,
                                                     std::size_t hoveredSlot) const {
    viz::HeapDrawState drawState;
    drawState.selected = selectedSlot;
    drawState.hovered = hoveredSlot;
    if (const HeapEvent* frame = currentFrame()) {
        drawState.kind = frame->kind;
        drawState.highlightA = frame->a;
        drawState.highlightB = frame->b;
        drawState.animT = animT_;
    }
    return drawState;
}

void HeapVisualizerView::drawStatusLine(const std::vector<const FolderNode*>& array) const {
    const theme::Colors& colors = theme::colors();
    const HeapEvent* frame = currentFrame();

    ImGui::PushFont(theme::caption());
    const std::string action =
        lastAction_.empty() ? std::string("Showing the current heap. Pick a demo to animate it.") : lastAction_;
    ImGui::TextColored(colors.textSecondaryV, "%s", format::ellipsize(action, 92).c_str());

    if (frame != nullptr) {
        ImGui::SameLine(0.0f, 10.0f);
        ImGui::TextColored(colors.textSecondaryV, "|");
        ImGui::SameLine(0.0f, 10.0f);
        ImGui::TextColored(colors.greenV, "frame %zu / %zu", frameIndex_ + 1, frames_.size());
        ImGui::SameLine(0.0f, 10.0f);
        ImGui::TextColored(colors.textSecondaryV, "|");
        ImGui::SameLine(0.0f, 10.0f);
        ImGui::TextColored(colors.textV, "%s", kindLabel(frame->kind));
        if (!frame->note.empty()) {
            ImGui::SameLine(0.0f, 10.0f);
            ImGui::TextColored(colors.textSecondaryV, "|");
            ImGui::SameLine(0.0f, 10.0f);
            ImGui::TextColored(colors.textSecondaryV, "%s", frame->note.c_str());
        }
    }
    ImGui::PopFont();
    (void)array;
}

void HeapVisualizerView::drawDetails(const FolderNode* node, std::size_t slot, AppState& state,
                                     float width) {
    const theme::Colors& colors = theme::colors();

    // The action button is placed from the measured right edge; SameLine offsets are
    // window-relative and clipped the label against the card border.
    const ImVec2 lineOrigin = ImGui::GetCursorScreenPos();
    const float rightEdge = lineOrigin.x + width;
    const float buttonW = 190.0f;
    const bool showButton = (rightEdge - buttonW) > (lineOrigin.x + 240.0f);

    ImGui::PushFont(theme::bodyStrong());
    ImGui::TextColored(colors.greenV, "%s", node->name.c_str());
    ImGui::PopFont();
    ImGui::SameLine(0.0f, 10.0f);
    ImGui::PushFont(theme::caption());
    ImGui::TextColored(colors.textSecondaryV, "slot %zu | parent %s | %s", slot,
                       slot == 0 ? "-" : std::to_string((slot - 1) / 2).c_str(),
                       format::byteSize(node->totalBytes).c_str());
    ImGui::PopFont();

    if (showButton) {
        ImGui::SetCursorScreenPos(ImVec2(rightEdge - buttonW, lineOrigin.y));
        if (ui::Button("##openInExplorer", "Open in Directory Explorer", ui::ButtonRole::Ghost,
                       ui::ButtonSize::Small, ui::Icon::ArrowRight, buttonW)) {
            state.selectedId = node->id;
            state.requestSection(Section::DirectoryExplorer);
        }
        ImGui::SetCursorScreenPos(ImVec2(lineOrigin.x, lineOrigin.y + ImGui::GetTextLineHeight() + 4.0f));
    }

    ImGui::PushFont(theme::caption());
    ImGui::TextColored(colors.textSecondaryV, "%s",
                       format::ellipsize(node->path.string(), 110).c_str());
    ImGui::PopFont();
}

void HeapVisualizerView::drawArrayStrip(const std::vector<const FolderNode*>& array, std::size_t selectedSlot, float width) {
    const theme::Colors& colors = theme::colors();
    const float contentW = viz::heapArrayContentWidth(array.size());
    const float maxScroll = (std::max)(0.0f, contentW - width);

    // Header: label on the left, scroll affordances on the right.
    ImGui::PushFont(theme::caption());
    ImGui::TextColored(colors.textSecondaryV, "Level-order array representation");
    ImGui::PopFont();

    if (contentW > width) {
        const float buttonSize = 20.0f;
        ImGui::SameLine(0.0f, width - ImGui::GetCursorScreenPos().x -
                                    ImGui::GetWindowPos().x - 2.0f * buttonSize - 8.0f);
        if (ui::IconButton("##arrayLeft", ui::Icon::ChevronLeft, "Scroll left", false,
                           arrayScroll_ > 0.0f, buttonSize)) {
            arrayScroll_ = (std::max)(0.0f, arrayScroll_ - width * 0.8f);
        }
        ImGui::SameLine(0.0f, 4.0f);
        if (ui::IconButton("##arrayRight", ui::Icon::ChevronRight, "Scroll right", false,
                           arrayScroll_ < maxScroll - 0.5f, buttonSize)) {
            arrayScroll_ = (std::min)(maxScroll, arrayScroll_ + width * 0.8f);
        }
    }

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##arrayCanvas", ImVec2(width, viz::kArrayBoxHeight));
    const bool hovered = ImGui::IsItemHovered();

    if (hovered) {
        const float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            arrayScroll_ = std::clamp(arrayScroll_ - wheel * 60.0f, 0.0f, maxScroll);
        }
    }
    arrayScroll_ = std::clamp(arrayScroll_, 0.0f, maxScroll);

    const viz::HeapDrawState drawState = makeDrawState(selectedSlot, hoveredSlot_);

    ImGui::GetWindowDrawList()->PushClipRect(ImVec2(origin.x, origin.y),
                                             ImVec2(origin.x + width, origin.y + viz::kArrayBoxHeight), true);
    viz::drawHeapArray(ImGui::GetWindowDrawList(), array, drawState, origin, arrayScroll_, origin.x,
                       origin.x + width);
    ImGui::GetWindowDrawList()->PopClipRect();
}

void HeapVisualizerView::drawLegend() const {
    const theme::Colors& colors = theme::colors();
    legendChip("root", colors.greenDark, colors.green);
    legendChip("compare", colors.greenSoft, colors.greenHover);
    legendChip("swap", colors.red, colors.red);
    legendChip("insert", colors.green, colors.green);
    legendChip("extract", colors.redDark, colors.red);
    ImGui::NewLine();
}

void HeapVisualizerView::drawCard(AppState& state, const std::vector<const FolderNode*>& liveItems) {
    const theme::Colors& colors = theme::colors();

    ui::CardOptions options;
    options.header = false;
    // Keep a usable floor for the tree and strip; the view scrolls when the window
    // is shorter than that (1024x640 is the smallest supported size).
    options.height = (std::max)(520.0f, ImGui::GetContentRegionAvail().y);

    if (ui::BeginCard("HeapVisualizerCard", options)) {
        const float width = ImGui::GetContentRegionAvail().x;

        // Header: icon, title, subtitle and a functional expand toggle.
        const ImVec2 headerStart = ImGui::GetCursorScreenPos();
        ui::DrawIcon(ImGui::GetWindowDrawList(), ui::Icon::Heap,
                     ImVec2(headerStart.x + 10.0f, headerStart.y + 11.0f), 20.0f, colors.green, 1.6f);
        ImGui::Dummy(ImVec2(28.0f, 22.0f));
        ImGui::SameLine(0.0f, 8.0f);
        ImGui::PushFont(theme::h2());
        ImGui::TextColored(colors.textV, "Heap Visualizer");
        ImGui::PopFont();

        const float toggleW = 26.0f;
        const float toggleX = headerStart.x + width - toggleW;
        ImGui::SetCursorScreenPos(ImVec2(toggleX, headerStart.y));
        if (ui::IconButton("##expandHeap", state.heapExpanded ? ui::Icon::Collapse : ui::Icon::Expand,
                           state.heapExpanded ? "Show the array representation" : "Expand the tree",
                           state.heapExpanded, true, toggleW)) {
            state.heapExpanded = !state.heapExpanded;
        }
        ImGui::SetCursorScreenPos(ImVec2(headerStart.x, headerStart.y + 24.0f));
        ImGui::PushFont(theme::caption());
        ImGui::TextColored(colors.textSecondaryV, "Showing the current heap (press a node to drill down).");
        ImGui::PopFont();

        ImGui::Dummy(ImVec2(0.0f, 6.0f));
        ui::Divider(false);
        ImGui::Dummy(ImVec2(0.0f, 6.0f));

        // Frame selection happens after any frames_ mutation.
        const HeapEvent* frame = currentFrame();
        const std::vector<const FolderNode*>& array = (frame != nullptr) ? frame->array : demoItems_;

        drawStatusLine(array);
        ImGui::Dummy(ImVec2(0.0f, 4.0f));

        const std::size_t selectedSlot =
            (state.heapSelectedSlot >= 0) ? static_cast<std::size_t>(state.heapSelectedSlot) : kNoIndex;
        const FolderNode* selectedNode = (selectedSlot < array.size()) ? array[selectedSlot] : nullptr;

        const float statusH = ImGui::GetTextLineHeight() + 6.0f;
        const float detailsH = (selectedNode != nullptr) ? (ImGui::GetTextLineHeight() * 2.0f + 14.0f) : 0.0f;
        const float arrayH = state.heapExpanded ? 0.0f : (viz::kArrayBoxHeight + ImGui::GetTextLineHeight() + 14.0f);
        const float legendH = state.heapExpanded ? 0.0f : (ImGui::GetTextLineHeight() + 10.0f);
        const float footerH = ImGui::GetTextLineHeight() + 6.0f;
        const float reserved = statusH + detailsH + arrayH + legendH + footerH + 18.0f;
        const float treeH = (std::max)(150.0f, ImGui::GetContentRegionAvail().y - reserved);

        // Tree canvas. No inner border: the nodes sit on the card surface.
        const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        const ImVec2 canvasSize(width, treeH);
        ImGui::InvisibleButton("##heapTreeCanvas", canvasSize);
        const bool canvasHovered = ImGui::IsItemHovered();

        const ImVec2 treeOrigin(canvasPos.x, canvasPos.y);
        const viz::HeapLayout layout = viz::computeHeapLayout(array.size(), treeOrigin, canvasSize);

        hoveredSlot_ = canvasHovered ? viz::hitTestHeapNode(layout, ImGui::GetIO().MousePos) : kNoIndex;
        if (canvasHovered && hoveredSlot_ != kNoIndex && ImGui::IsMouseClicked(0)) {
            state.heapSelectedSlot = static_cast<int>(hoveredSlot_);
        }

        const viz::HeapDrawState drawState = makeDrawState(selectedSlot, hoveredSlot_);
        viz::drawHeapTree(ImGui::GetWindowDrawList(), array, layout, drawState);

        if (array.empty()) {
            ui::EmptyState(ui::Icon::Heap, "Heap is empty", "This scan found no folders to rank.");
        }

        // Details of the inspected slot.
        if (selectedNode != nullptr) {
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            drawDetails(selectedNode, selectedSlot, state, width);
        }

        if (!state.heapExpanded) {
            ImGui::Dummy(ImVec2(0.0f, 6.0f));
            // selection already reflects state.heapSelectedSlot
            drawArrayStrip(array, selectedSlot, width);
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            drawLegend();
        }

        // Footer summary.
        ImGui::PushFont(theme::caption());
        const std::string sizing = (array.size() < liveItems.size())
                                       ? format::count(array.size()) + " of " + format::count(liveItems.size()) + " folders shown"
                                       : format::count(array.size()) + " folders";
        ImGui::TextColored(colors.textSecondaryV, "Heap size: %s  |  %s  |  root = largest folder",
                           format::count(liveItems.size()).c_str(), sizing.c_str());
        if (liveItems.size() > HeapDemoEngine::kDefaultDemoCap) {
            ImGui::SameLine(0.0f, 10.0f);
            ImGui::TextColored(colors.textDisabledV, "|  demos capped at %zu nodes",
                               static_cast<std::size_t>(HeapDemoEngine::kDefaultDemoCap));
        }
        ImGui::PopFont();
    }
    ui::EndCard();
}

void HeapVisualizerView::draw(AppState& state) {
    const std::vector<const FolderNode*>* liveItemsPtr = nullptr;
    if (state.engine && state.engine->hasResult()) {
        liveItemsPtr = &state.engine->heap().data();
    }

    bool hasLive = liveItemsPtr != nullptr && !liveItemsPtr->empty();
    if (!hasLive) {
        ui::SectionHeader(ui::Icon::Heap, "Heap Visualizer",
                          "Watch the max heap that ranks folders by size");
        ui::EmptyState(ui::Icon::Heap, "Nothing to visualize",
                       state.engine && state.engine->hasResult()
                           ? "This scan found no subfolders, so the ranking heap is empty."
                           : "Scan a folder to build the max heap that ranks its folders.");
        return;
    }

    const std::vector<const FolderNode*>& liveItems = *liveItemsPtr;

    if (cachedGeneration_ != state.engine->generation()) {
        cachedGeneration_ = state.engine->generation();
        frames_.clear();
        frameIndex_ = 0;
        playing_ = false;
        accumulator_ = 0.0f;
        animT_ = 1.0f;
        lastAction_.clear();
        arrayScroll_ = 0.0f;
        state.heapSelectedSlot = -1;
        demoItems_ = HeapDemoEngine::selectDemoItems(liveItems, HeapDemoEngine::kDefaultDemoCap);
    }
    if (demoItems_.empty()) {
        demoItems_ = HeapDemoEngine::selectDemoItems(liveItems, HeapDemoEngine::kDefaultDemoCap);
    }

    ui::SectionHeader(ui::Icon::Heap, "Heap Visualizer",
                      "Playback of the real max heap: build, insert and extract");

    ImGui::BeginChild("##heapScroll", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None, ImGuiWindowFlags_None);
    drawControls(state);

    // Playback clock: advances frames while playing and restarts the settle
    // animation whenever the visible frame changes.
    if (playing_ && !frames_.empty()) {
        accumulator_ += ImGui::GetIO().DeltaTime * (std::max)(state.demoFramesPerSecond, 1.0f);
        while (accumulator_ >= 1.0f && frameIndex_ + 1 < frames_.size()) {
            accumulator_ -= 1.0f;
            ++frameIndex_;
            animT_ = 0.0f;
        }
        if (frameIndex_ + 1 >= frames_.size()) {
            playing_ = false;
            accumulator_ = 0.0f;
        }
    }
    if (animT_ < 1.0f) {
        // 180 ms settle: fast enough to keep up with 30 fps playback, slow enough
        // to read a swap.
        animT_ = (std::min)(1.0f, animT_ + ImGui::GetIO().DeltaTime / 0.18f);
    }

    runPendingDemo(liveItems);

    ImGui::Dummy(ImVec2(0.0f, theme::metrics().gap));
    drawCard(state, liveItems);
    ImGui::EndChild();
}

}  // namespace diskscope

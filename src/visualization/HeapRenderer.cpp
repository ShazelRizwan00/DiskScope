#include "visualization/HeapRenderer.h"

#include <algorithm>
#include <cstddef>
#include <string>

#include <imgui.h>

#include "ui/Theme.h"
#include "utils/Formatting.h"

namespace diskscope::viz {
namespace {

std::size_t depthOf(std::size_t index) {
    std::size_t depth = 0;
    while (index + 1 >= (static_cast<std::size_t>(1) << (depth + 1))) {
        ++depth;
    }
    return depth;
}

std::size_t slotsAtDepth(std::size_t depth) {
    return static_cast<std::size_t>(1) << depth;
}

std::size_t firstSlotAtDepth(std::size_t depth) {
    return slotsAtDepth(depth) - 1;
}

}  // namespace

float recommendedNodeRadius(std::size_t nodeCount, ImVec2 size) {
    if (nodeCount == 0) {
        return 16.0f;
    }
    const std::size_t depth = depthOf(nodeCount - 1);
    const float slots = static_cast<float>(slotsAtDepth(depth));
    const float horizontal = size.x / (slots * 2.2f);
    const float vertical = size.y / (static_cast<float>(std::max<std::size_t>(depth, 1)) * 2.2f);
    return std::clamp(std::min(horizontal, vertical), 10.0f, 28.0f);
}

HeapLayout computeHeapLayout(std::size_t nodeCount, ImVec2 origin, ImVec2 size, float nodeRadius) {
    HeapLayout layout;
    layout.nodeRadius = nodeRadius;

    if (nodeCount == 0) {
        return layout;
    }

    const std::size_t depth = depthOf(nodeCount - 1);
    layout.depth = depth;

    const float usableHeight = std::max(size.y - 2.0f * nodeRadius, 0.0f);
    layout.levelHeight = depth > 0 ? usableHeight / static_cast<float>(depth) : 0.0f;

    layout.positions.reserve(nodeCount);
    for (std::size_t i = 0; i < nodeCount; ++i) {
        const std::size_t level = depthOf(i);
        const std::size_t first = firstSlotAtDepth(level);
        const std::size_t slots = slotsAtDepth(level);
        // Centre each slot inside its share of the level width, so a level never
        // overlaps its neighbours.
        const float fraction = (static_cast<float>(i - first) + 0.5f) / static_cast<float>(slots);
        const float x = origin.x + fraction * size.x;
        const float y = origin.y + nodeRadius + static_cast<float>(level) * layout.levelHeight;
        layout.positions.emplace_back(x, y);
    }
    return layout;
}

ImU32 slotColour(std::size_t index, std::size_t highlightA, std::size_t highlightB, HeapEventKind kind) {
    const theme::Colors& colors = theme::colors();

    if (kind == HeapEventKind::Compare && (index == highlightA || index == highlightB)) {
        return colors.warn;
    }
    if (kind == HeapEventKind::Swap && (index == highlightA || index == highlightB)) {
        return colors.good;
    }
    if ((kind == HeapEventKind::Insert || kind == HeapEventKind::SiftUp) && index == highlightA) {
        return colors.accent;
    }
    if ((kind == HeapEventKind::Extract || kind == HeapEventKind::SiftDown) && index == highlightA) {
        return colors.bad;
    }
    if (index == 0) {
        return colors.accentSoft; // the root is always marked
    }
    return colors.card;
}

void drawHeapTree(ImDrawList* list, const std::vector<const FolderNode*>& array, std::size_t highlightA,
                  std::size_t highlightB, HeapEventKind kind, ImVec2 origin, ImVec2 size) {
    if (list == nullptr || array.empty()) {
        return;
    }

    const float radius = recommendedNodeRadius(array.size(), size);
    const HeapLayout layout = computeHeapLayout(array.size(), origin, size, radius);

    // Edges first so nodes paint over them.
    for (std::size_t i = 1; i < layout.positions.size(); ++i) {
        const ImVec2& child = layout.positions[i];
        const ImVec2& parent = layout.positions[(i - 1) / 2];
        list->AddLine(parent, child, theme::colors().border, 1.5f);
    }

    const float fontSize = std::clamp(radius * 0.55f, 9.0f, 13.0f);
    ImFont* font = theme::body();

    for (std::size_t i = 0; i < layout.positions.size() && i < array.size(); ++i) {
        const FolderNode* node = array[i];
        const ImVec2 center = layout.positions[i];
        const ImU32 fill = slotColour(i, highlightA, highlightB, kind);
        const ImU32 border = (i == 0) ? theme::colors().accent : theme::colors().border;

        list->AddCircleFilled(center, radius, fill, 24);
        list->AddCircle(center, radius, border, 24, i == 0 ? 2.5f : 1.0f);

        if (node == nullptr) {
            continue;
        }

        const std::string sizeText = format::byteSizeCompact(node->totalBytes);
        const std::string nameText = format::ellipsize(node->name, 10);

        const ImVec2 sizeSize = font->CalcTextSizeA(fontSize, 1000.0f, 0.0f, sizeText.c_str());
        const ImVec2 nameSize = font->CalcTextSizeA(fontSize * 0.85f, 1000.0f, 0.0f, nameText.c_str());

        list->AddText(font, fontSize, ImVec2(center.x - sizeSize.x * 0.5f, center.y - sizeSize.y * 0.75f),
                      theme::colors().text, sizeText.c_str());
        if (radius >= 14.0f) {
            list->AddText(font, fontSize * 0.85f,
                          ImVec2(center.x - nameSize.x * 0.5f, center.y + sizeSize.y * 0.15f),
                          theme::colors().textMuted, nameText.c_str());
        }
    }
}

void drawHeapArray(ImDrawList* list, const std::vector<const FolderNode*>& array, std::size_t highlightA,
                   std::size_t highlightB, HeapEventKind kind, ImVec2 origin, float height) {
    if (list == nullptr || array.empty()) {
        return;
    }

    constexpr float kBoxWidth = 74.0f;
    constexpr float kGap = 4.0f;
    ImFont* font = theme::body();

    for (std::size_t i = 0; i < array.size(); ++i) {
        const float x = origin.x + static_cast<float>(i) * (kBoxWidth + kGap);
        const ImVec2 topLeft(x, origin.y);
        const ImVec2 bottomRight(x + kBoxWidth, origin.y + height);

        const ImU32 fill = slotColour(i, highlightA, highlightB, kind);
        list->AddRectFilled(topLeft, bottomRight, fill, 4.0f);
        list->AddRect(topLeft, bottomRight, i == 0 ? theme::colors().accent : theme::colors().border, 4.0f,
                      0, 1.0f);

        const std::string indexText = "[" + std::to_string(i) + "]";
        list->AddText(font, 10.0f, ImVec2(x + 5.0f, origin.y + 4.0f), theme::colors().textMuted, indexText.c_str());

        const FolderNode* node = array[i];
        if (node != nullptr) {
            const std::string valueText = format::byteSizeCompact(node->totalBytes);
            list->AddText(font, 12.0f, ImVec2(x + 5.0f, origin.y + 20.0f), theme::colors().text, valueText.c_str());
            const std::string nameText = format::ellipsize(node->name, 11);
            list->AddText(font, 10.0f, ImVec2(x + 5.0f, origin.y + 36.0f), theme::colors().textMuted,
                          nameText.c_str());
        }
    }
}

}  // namespace diskscope::viz

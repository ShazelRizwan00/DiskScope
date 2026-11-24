#include "visualization/HeapRenderer.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <string>

#include <imgui.h>

#include "ui/Theme.h"
#include "utils/Formatting.h"

namespace diskscope::viz {
namespace {

constexpr ImU32 kDarkInk = IM_COL32(10, 26, 18, 255);

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

float easeOutCubic(float t) {
    const float clamped = std::clamp(t, 0.0f, 1.0f);
    const float inv = 1.0f - clamped;
    return 1.0f - inv * inv * inv;
}

// Shrinks the font until `text` fits `maxWidth`, then truncates the text itself.
// Node labels used to overflow the circle; measuring both dimensions is the only
// reliable fix because the mono/body fonts differ in width per glyph.
float fitFontSize(ImFont* font, const char* text, float maxWidth, float startSize, float minSize) {
    float size = startSize;
    while (size > minSize) {
        const ImVec2 measured = font->CalcTextSizeA(size, 1000.0f, 0.0f, text);
        if (measured.x <= maxWidth) {
            return size;
        }
        size -= 0.5f;
    }
    return minSize;
}

std::string fitText(ImFont* font, const std::string& text, float maxWidth, float fontSize) {
    if (font->CalcTextSizeA(fontSize, 1000.0f, 0.0f, text.c_str()).x <= maxWidth) {
        return text;
    }
    std::string clipped = text;
    while (!clipped.empty()) {
        clipped.pop_back();
        const std::string candidate = clipped + "...";
        if (font->CalcTextSizeA(fontSize, 1000.0f, 0.0f, candidate.c_str()).x <= maxWidth) {
            return candidate;
        }
    }
    return std::string();
}

}  // namespace

HeapLayout computeHeapLayout(std::size_t nodeCount, ImVec2 origin, ImVec2 size) {
    HeapLayout layout;
    if (nodeCount == 0 || size.x <= 0.0f || size.y <= 0.0f) {
        return layout;
    }

    const std::size_t depth = depthOf(nodeCount - 1);
    layout.depth = depth;

    const std::size_t deepestSlots = slotsAtDepth(depth);
    const float slotWidth = size.x / static_cast<float>(deepestSlots);

    // A node must fit its slot with a margin, and the level spacing must leave
    // room for two radii plus a visible edge.
    const float byWidth = slotWidth * 0.44f;
    const float byHeight = (depth > 0) ? (size.y / static_cast<float>(depth)) * 0.36f : size.y * 0.34f;
    const float baseRadius = std::clamp(std::min(byWidth, byHeight), 14.0f, 42.0f);

    layout.levelHeight = (depth > 0) ? (size.y - 2.0f * baseRadius) / static_cast<float>(depth) : 0.0f;

    layout.positions.reserve(nodeCount);
    layout.radii.reserve(nodeCount);
    for (std::size_t i = 0; i < nodeCount; ++i) {
        const std::size_t level = depthOf(i);
        const std::size_t first = firstSlotAtDepth(level);
        const std::size_t slots = slotsAtDepth(level);
        const float fraction = (static_cast<float>(i - first) + 0.5f) / static_cast<float>(slots);

        // Shrink with depth: the root reads as the anchor, leaves stay compact.
        const float levelScale = (level == 0) ? 1.16f : (level == 1 ? 1.0f : 0.90f);
        const float radius = std::max(13.0f, baseRadius * levelScale);

        const float x = origin.x + fraction * size.x;
        const float y = origin.y + radius + static_cast<float>(level) * layout.levelHeight;
        layout.positions.emplace_back(x, y);
        layout.radii.push_back(radius);
    }

    return layout;
}

std::size_t hitTestHeapNode(const HeapLayout& layout, ImVec2 mouse) {
    for (std::size_t i = 0; i < layout.positions.size(); ++i) {
        const ImVec2 delta(mouse.x - layout.positions[i].x, mouse.y - layout.positions[i].y);
        if (delta.x * delta.x + delta.y * delta.y <= layout.radii[i] * layout.radii[i]) {
            return i;
        }
    }
    return kNoIndex;
}

ImU32 slotColour(std::size_t index, const HeapDrawState& state) {
    const theme::Colors& colors = theme::colors();
    const bool isA = (index == state.highlightA);
    const bool isB = (index == state.highlightB);

    switch (state.kind) {
        case HeapEventKind::Swap:
            if (isA || isB) {
                return colors.red;
            }
            break;
        case HeapEventKind::Insert:
        case HeapEventKind::SiftUp:
            if (isA) {
                return colors.green;
            }
            break;
        case HeapEventKind::Extract:
        case HeapEventKind::SiftDown:
            if (isA) {
                return colors.redDark;
            }
            break;
        case HeapEventKind::Compare:
            // The pair keeps its base fill; the double ring in paintNode marks it.
            // Outlined-only comparisons were indistinguishable from the root.
            break;
        case HeapEventKind::Build:
        case HeapEventKind::Done:
            break;
    }

    return (index == 0) ? colors.greenDark : colors.panel;
}

void drawHeapTree(ImDrawList* list, const std::vector<const FolderNode*>& array, const HeapLayout& layout,
                  const HeapDrawState& state) {
    if (list == nullptr || layout.positions.empty()) {
        return;
    }

    const theme::Colors& colors = theme::colors();
    const std::size_t slots = std::min(array.size(), layout.positions.size());

    // Edges first so nodes paint over their endpoints.
    for (std::size_t i = 1; i < layout.positions.size(); ++i) {
        const std::size_t parent = (i - 1) / 2;
        const ImVec2 from(layout.positions[parent].x, layout.positions[parent].y + layout.radii[parent]);
        const ImVec2 to(layout.positions[i].x, layout.positions[i].y - layout.radii[i]);
        list->AddLine(from, to, colors.borderStrong, 1.2f);
    }

    const float settle = easeOutCubic(state.animT);
    const bool swapping = (state.kind == HeapEventKind::Swap) && (state.animT < 1.0f) &&
                          state.highlightA < slots && state.highlightB < slots;
    const bool inserting = (state.kind == HeapEventKind::Insert) && (state.animT < 1.0f) &&
                           state.highlightA < slots;

    auto paintNode = [&](std::size_t slot, ImVec2 centre, float radius, float alpha) {
        const FolderNode* node = array[slot];
        const ImU32 fill = theme::withAlpha(slotColour(slot, state), alpha);
        ImU32 border = (slot == 0) ? colors.green : colors.border;
        float borderWidth = (slot == 0) ? 2.0f : 1.0f;

        const bool isA = (slot == state.highlightA);
        const bool isB = (slot == state.highlightB);
        if (state.kind == HeapEventKind::Compare && (isA || isB)) {
            border = colors.greenHover;
            borderWidth = 2.5f;
        } else if (state.kind == HeapEventKind::Swap && (isA || isB)) {
            border = colors.red;
            borderWidth = 2.0f;
        } else if (state.kind == HeapEventKind::Extract && isA) {
            border = colors.red;
            borderWidth = 2.5f;
        } else if (slot == state.selected) {
            border = colors.green;
            borderWidth = 2.5f;
        } else if (slot == state.hovered) {
            border = colors.greenHover;
            borderWidth = 2.0f;
        }

        list->AddCircleFilled(centre, radius, fill, 28);
        list->AddCircle(centre, radius, theme::withAlpha(border, alpha), 28, borderWidth);

        // Compare gets a second, offset ring: the root is already ringed in green, so
        // an outline alone could not carry the comparison state.
        if (state.kind == HeapEventKind::Compare && (isA || isB)) {
            list->AddCircle(centre, radius + 3.5f, theme::withAlpha(colors.greenHover, alpha * 0.85f), 28, 1.5f);
        }

        if (node == nullptr) {
            return;
        }

        const bool darkFill = (slot == 0) || (state.kind == HeapEventKind::Insert && isA) ||
                              (state.kind == HeapEventKind::SiftUp && isA);
        const ImU32 sizeInk = darkFill ? colors.green : colors.text;
        const ImU32 nameInk = darkFill ? theme::withAlpha(colors.text, 0.72f) : colors.textSecondary;

        ImFont* font = theme::body();
        const std::string sizeText = format::byteSizeCompact(node->totalBytes);
        const float maxWidth = radius * 1.62f;

        const float sizeStart = std::clamp(radius * 0.44f, 10.0f, 16.0f);
        const float sizeFont = fitFontSize(font, sizeText.c_str(), maxWidth, sizeStart, 10.0f);
        const ImVec2 sizeMeasure = font->CalcTextSizeA(sizeFont, 1000.0f, 0.0f, sizeText.c_str());
        const ImVec2 sizePos(centre.x - sizeMeasure.x * 0.5f, centre.y - radius * 0.40f);
        list->AddText(font, sizeFont, sizePos, theme::withAlpha(sizeInk, alpha), sizeText.c_str());

        if (radius < 19.0f) {
            return;
        }

        const float nameStart = std::clamp(radius * 0.30f, 7.5f, 11.0f);
        const std::string nameText = fitText(font, node->name, maxWidth * 0.94f, nameStart);
        if (nameText.empty()) {
            return;
        }
        const ImVec2 nameMeasure = font->CalcTextSizeA(nameStart, 1000.0f, 0.0f, nameText.c_str());
        const ImVec2 namePos(centre.x - nameMeasure.x * 0.5f, centre.y + radius * 0.10f);
        list->AddText(font, nameStart, namePos, theme::withAlpha(nameInk, alpha), nameText.c_str());
    };

    for (std::size_t i = 0; i < slots; ++i) {
        if (swapping && (i == state.highlightA || i == state.highlightB)) {
            continue;  // painted below at an interpolated position
        }
        const float radius = inserting && i == state.highlightA
                                 ? layout.radii[i] * (0.55f + 0.45f * settle)
                                 : layout.radii[i];
        paintNode(i, layout.positions[i], radius, 1.0f);
    }

    if (swapping) {
        const std::size_t a = state.highlightA;
        const std::size_t b = state.highlightB;
        const ImVec2 fromB = layout.positions[b];
        const ImVec2 toA = layout.positions[a];
        const ImVec2 fromA = layout.positions[a];
        const ImVec2 toB = layout.positions[b];

        // Slot a now holds the item that came from b, and vice versa: sliding them
        // between the two centres is the literal swap, not decoration.
        const ImVec2 movingA(fromB.x + (toA.x - fromB.x) * settle, fromB.y + (toA.y - fromB.y) * settle);
        const ImVec2 movingB(fromA.x + (toB.x - fromA.x) * settle, fromA.y + (toB.y - fromA.y) * settle);
        paintNode(a, movingA, layout.radii[a], 1.0f);
        paintNode(b, movingB, layout.radii[b], 1.0f);
    }
}

float heapArrayContentWidth(std::size_t count) {
    if (count == 0) {
        return 0.0f;
    }
    return static_cast<float>(count) * (kArrayBoxWidth + kArrayGap) - kArrayGap;
}

void drawHeapArray(ImDrawList* list, const std::vector<const FolderNode*>& array,
                   const HeapDrawState& state, ImVec2 origin, float scrollX, float clipMinX,
                   float clipMaxX) {
    if (list == nullptr || array.empty()) {
        return;
    }

    const theme::Colors& colors = theme::colors();
    ImFont* captionFont = theme::caption();
    ImFont* monoFont = theme::mono();
    ImFont* bodyFont = theme::bodyStrong();

    for (std::size_t i = 0; i < array.size(); ++i) {
        const float x = origin.x + static_cast<float>(i) * (kArrayBoxWidth + kArrayGap) - scrollX;
        if (x + kArrayBoxWidth < clipMinX || x > clipMaxX) {
            continue;  // scrolled out of view
        }

        const ImVec2 boxMin(x, origin.y);
        const ImVec2 boxMax(x + kArrayBoxWidth, origin.y + kArrayBoxHeight);
        const bool isSelected = (i == state.selected);
        const bool highlighted = (i == state.highlightA || i == state.highlightB) &&
                                 state.kind != HeapEventKind::Done && state.kind != HeapEventKind::Build;

        ImU32 fill = colors.elevated;
        ImU32 border = colors.border;
        ImU32 ink = colors.text;
        if (isSelected) {
            fill = colors.greenDark;
            border = colors.green;
            ink = colors.green;
        }
        if (highlighted) {
            border = (state.kind == HeapEventKind::Swap || state.kind == HeapEventKind::Extract)
                         ? colors.red
                         : colors.greenHover;
        }

        list->AddRectFilled(boxMin, boxMax, fill, 8.0f);
        list->AddRect(boxMin, boxMax, border, 8.0f, 0, highlighted || isSelected ? 2.0f : 1.0f);

        char indexText[16];
        std::snprintf(indexText, sizeof(indexText), "[%zu]", i);
        list->AddText(monoFont, captionFont->FontSize, ImVec2(x + 8.0f, origin.y + 5.0f),
                      theme::withAlpha(colors.textSecondary, 0.85f), indexText);

        const FolderNode* node = array[i];
        if (node == nullptr) {
            continue;
        }
        list->AddText(bodyFont, bodyFont->FontSize, ImVec2(x + 8.0f, origin.y + 21.0f), ink,
                      format::byteSizeCompact(node->totalBytes).c_str());

        const std::string name = fitText(captionFont, node->name, kArrayBoxWidth - 16.0f,
                                         captionFont->FontSize);
        list->AddText(captionFont, captionFont->FontSize, ImVec2(x + 8.0f, origin.y + 42.0f),
                      isSelected ? theme::withAlpha(colors.green, 0.85f) : colors.textSecondary, name.c_str());
    }
}

}  // namespace diskscope::viz

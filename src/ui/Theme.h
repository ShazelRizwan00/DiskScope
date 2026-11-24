#pragma once

#include <imgui.h>

namespace diskscope::theme {

struct Colors {
    // Canonical UI palette tokens (ImU32)
    ImU32 bg = 0;
    ImU32 sidebar = 0;
    ImU32 panel = 0;
    ImU32 elevated = 0;
    ImU32 elevatedHover = 0;
    ImU32 border = 0;
    ImU32 borderStrong = 0;
    ImU32 green = 0;
    ImU32 greenHover = 0;
    ImU32 greenDark = 0;
    ImU32 greenSoft = 0;
    ImU32 red = 0;
    ImU32 redDark = 0;
    ImU32 redSoft = 0;
    ImU32 text = 0;
    ImU32 textSecondary = 0;
    ImU32 textDisabled = 0;

    // Backward-compatible aliases (ImU32)
    ImU32 background = 0;   // alias for bg
    ImU32 card = 0;         // alias for panel
    ImU32 cardHover = 0;    // alias for elevated
    ImU32 accent = 0;       // alias for green
    ImU32 accentSoft = 0;   // alias for greenSoft
    ImU32 textMuted = 0;    // alias for textSecondary
    ImU32 good = 0;         // alias for green
    ImU32 warn = 0;         // alias for red
    ImU32 bad = 0;          // alias for red

    // Canonical UI palette tokens (ImVec4)
    ImVec4 bgV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 sidebarV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 panelV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 elevatedV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 elevatedHoverV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 borderV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 borderStrongV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 greenV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 greenHoverV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 greenDarkV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 greenSoftV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 redV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 redDarkV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 redSoftV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 textV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 textSecondaryV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 textDisabledV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    // Backward-compatible aliases (ImVec4)
    ImVec4 backgroundV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f); // alias for bgV
    ImVec4 cardV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);       // alias for panelV
    ImVec4 cardHoverV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);  // alias for elevatedV
    ImVec4 accentV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);     // alias for greenV
    ImVec4 accentSoftV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f); // alias for greenSoftV
    ImVec4 textMutedV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);  // alias for textSecondaryV
    ImVec4 goodV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);       // alias for greenV
    ImVec4 warnV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);       // alias for redV
    ImVec4 badV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);        // alias for redV
};

struct Metrics {
    float sidebarWidth = 240.0f;
    float toolbarHeight = 112.0f;
    float cardPadding = 16.0f;
    float cardRadius = 12.0f;
    float controlRadius = 8.0f;
    float rowHeight = 40.0f;
    float navRowHeight = 54.0f;
    float gap = 12.0f;
    float sectionGap = 20.0f;
    float tableRowHeight = 34.0f;
    float iconSize = 20.0f;
    float iconSizeSmall = 16.0f;
};

void apply(float dpiScale = 1.0f);
void loadFonts(float dpiScale = 1.0f);

const Colors& colors();
const Metrics& metrics();

ImFont* caption();
ImFont* body();
ImFont* bodyStrong();
ImFont* h3();
ImFont* h2();
ImFont* metric();
ImFont* mono();
ImFont* heading();

ImU32 chartColour(int index);

}  // namespace diskscope::theme

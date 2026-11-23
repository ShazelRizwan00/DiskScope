#pragma once

#include <imgui.h>

namespace diskscope::theme {

struct Colors {
    ImU32 accent = 0;
    ImU32 accentSoft = 0;
    ImU32 card = 0;
    ImU32 cardHover = 0;
    ImU32 border = 0;
    ImU32 text = 0;
    ImU32 textMuted = 0;
    ImU32 good = 0;
    ImU32 warn = 0;
    ImU32 bad = 0;
    ImU32 sidebar = 0;
    ImU32 background = 0;

    ImVec4 accentV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 accentSoftV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 cardV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 cardHoverV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 borderV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 textV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 textMutedV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 goodV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 warnV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 badV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 sidebarV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImVec4 backgroundV = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
};

void apply(float dpiScale = 1.0f);
void loadFonts(float dpiScale = 1.0f);

ImFont* body();
ImFont* heading();
ImFont* mono();

const Colors& colors();

}  // namespace diskscope::theme

#include "ui/Theme.h"

#include <filesystem>

namespace diskscope::theme {

namespace {

Colors s_colors;
bool s_colorsInitialized = false;

ImFont* s_body = nullptr;
ImFont* s_heading = nullptr;
ImFont* s_mono = nullptr;

void initColors() {
    if (s_colorsInitialized) {
        return;
    }

    // Palette: dark background (#14161a), slightly lighter cards, subtle borders,
    // one restrained accent (teal/azure #38bdf8), muted grey text, semantic good/warn/bad.
    s_colors.backgroundV = ImVec4(0.078f, 0.086f, 0.102f, 1.000f); // #14161a
    s_colors.sidebarV    = ImVec4(0.063f, 0.071f, 0.086f, 1.000f); // #101216
    s_colors.cardV       = ImVec4(0.110f, 0.122f, 0.149f, 1.000f); // #1c1f26
    s_colors.cardHoverV  = ImVec4(0.145f, 0.161f, 0.196f, 1.000f); // #252932
    s_colors.borderV     = ImVec4(0.176f, 0.196f, 0.235f, 1.000f); // #2d323c

    s_colors.textV       = ImVec4(0.902f, 0.929f, 0.953f, 1.000f); // #e6edf3
    s_colors.textMutedV  = ImVec4(0.545f, 0.580f, 0.620f, 1.000f); // #8b949e

    s_colors.accentV     = ImVec4(0.220f, 0.741f, 0.973f, 1.000f); // #38bdf8 (azure/sky)
    s_colors.accentSoftV = ImVec4(0.118f, 0.227f, 0.353f, 1.000f); // subtle accent surface

    s_colors.goodV       = ImVec4(0.133f, 0.773f, 0.369f, 1.000f); // #22c55e (green)
    s_colors.warnV       = ImVec4(0.961f, 0.620f, 0.043f, 1.000f); // #f59e0b (amber)
    s_colors.badV        = ImVec4(0.937f, 0.267f, 0.267f, 1.000f); // #ef4444 (red)

    s_colors.background = ImGui::ColorConvertFloat4ToU32(s_colors.backgroundV);
    s_colors.sidebar    = ImGui::ColorConvertFloat4ToU32(s_colors.sidebarV);
    s_colors.card       = ImGui::ColorConvertFloat4ToU32(s_colors.cardV);
    s_colors.cardHover  = ImGui::ColorConvertFloat4ToU32(s_colors.cardHoverV);
    s_colors.border     = ImGui::ColorConvertFloat4ToU32(s_colors.borderV);
    s_colors.text       = ImGui::ColorConvertFloat4ToU32(s_colors.textV);
    s_colors.textMuted  = ImGui::ColorConvertFloat4ToU32(s_colors.textMutedV);
    s_colors.accent     = ImGui::ColorConvertFloat4ToU32(s_colors.accentV);
    s_colors.accentSoft = ImGui::ColorConvertFloat4ToU32(s_colors.accentSoftV);
    s_colors.good       = ImGui::ColorConvertFloat4ToU32(s_colors.goodV);
    s_colors.warn       = ImGui::ColorConvertFloat4ToU32(s_colors.warnV);
    s_colors.bad        = ImGui::ColorConvertFloat4ToU32(s_colors.badV);

    s_colorsInitialized = true;
}

}  // namespace

const Colors& colors() {
    if (!s_colorsInitialized) {
        initColors();
    }
    return s_colors;
}

void apply(float dpiScale) {
    initColors();

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* c = style.Colors;

    c[ImGuiCol_Text]                  = s_colors.textV;
    c[ImGuiCol_TextDisabled]          = s_colors.textMutedV;
    c[ImGuiCol_WindowBg]              = s_colors.backgroundV;
    c[ImGuiCol_ChildBg]               = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_PopupBg]               = s_colors.cardV;
    c[ImGuiCol_Border]                = s_colors.borderV;
    c[ImGuiCol_BorderShadow]          = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_FrameBg]               = ImVec4(0.125f, 0.141f, 0.173f, 1.0f);
    c[ImGuiCol_FrameBgHovered]        = s_colors.cardHoverV;
    c[ImGuiCol_FrameBgActive]         = s_colors.accentSoftV;
    c[ImGuiCol_TitleBg]               = s_colors.sidebarV;
    c[ImGuiCol_TitleBgActive]         = s_colors.sidebarV;
    c[ImGuiCol_TitleBgCollapsed]      = s_colors.sidebarV;
    c[ImGuiCol_MenuBarBg]             = s_colors.sidebarV;
    c[ImGuiCol_ScrollbarBg]           = s_colors.backgroundV;
    c[ImGuiCol_ScrollbarGrab]         = ImVec4(0.235f, 0.263f, 0.314f, 1.0f);
    c[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.314f, 0.353f, 0.420f, 1.0f);
    c[ImGuiCol_ScrollbarGrabActive]   = s_colors.accentV;
    c[ImGuiCol_CheckMark]             = s_colors.accentV;
    c[ImGuiCol_SliderGrab]            = s_colors.accentV;
    c[ImGuiCol_SliderGrabActive]      = s_colors.accentV;
    c[ImGuiCol_Button]                = s_colors.cardV;
    c[ImGuiCol_ButtonHovered]         = s_colors.cardHoverV;
    c[ImGuiCol_ButtonActive]          = s_colors.accentSoftV;
    c[ImGuiCol_Header]                = s_colors.cardV;
    c[ImGuiCol_HeaderHovered]         = s_colors.cardHoverV;
    c[ImGuiCol_HeaderActive]          = s_colors.accentSoftV;
    c[ImGuiCol_Separator]             = s_colors.borderV;
    c[ImGuiCol_SeparatorHovered]      = s_colors.accentV;
    c[ImGuiCol_SeparatorActive]       = s_colors.accentV;
    c[ImGuiCol_ResizeGrip]            = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_ResizeGripHovered]     = s_colors.accentV;
    c[ImGuiCol_ResizeGripActive]      = s_colors.accentV;
    c[ImGuiCol_Tab]                   = s_colors.cardV;
    c[ImGuiCol_TabHovered]            = s_colors.cardHoverV;
    c[ImGuiCol_TabActive]             = s_colors.accentSoftV;
    c[ImGuiCol_TabUnfocused]          = s_colors.backgroundV;
    c[ImGuiCol_TabUnfocusedActive]    = s_colors.cardV;
    c[ImGuiCol_PlotLines]             = s_colors.accentV;
    c[ImGuiCol_PlotLinesHovered]      = s_colors.goodV;
    c[ImGuiCol_PlotHistogram]         = s_colors.accentV;
    c[ImGuiCol_PlotHistogramHovered]  = s_colors.goodV;
    c[ImGuiCol_TableHeaderBg]         = s_colors.cardV;
    c[ImGuiCol_TableBorderStrong]     = s_colors.borderV;
    c[ImGuiCol_TableBorderLight]      = ImVec4(s_colors.borderV.x, s_colors.borderV.y, s_colors.borderV.z, 0.5f);
    c[ImGuiCol_TableRowBg]            = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_TableRowBgAlt]         = ImVec4(1.0f, 1.0f, 1.0f, 0.02f);
    c[ImGuiCol_TextSelectedBg]        = s_colors.accentSoftV;
    c[ImGuiCol_DragDropTarget]        = s_colors.accentV;
    c[ImGuiCol_NavHighlight]          = s_colors.accentV;

    // Geometry and Rounding: professional developer-tool look, consistent 8px grid
    style.WindowRounding    = 6.0f;
    style.ChildRounding     = 6.0f;
    style.FrameRounding     = 4.0f;
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding      = 4.0f;
    style.TabRounding       = 4.0f;

    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.FrameBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.TabBorderSize     = 1.0f;

    style.WindowPadding     = ImVec2(12.0f * dpiScale, 12.0f * dpiScale);
    style.FramePadding      = ImVec2(8.0f * dpiScale, 5.0f * dpiScale);
    style.ItemSpacing       = ImVec2(8.0f * dpiScale, 8.0f * dpiScale);
    style.ItemInnerSpacing  = ImVec2(6.0f * dpiScale, 6.0f * dpiScale);
    style.ScrollbarSize     = 12.0f * dpiScale;
}

void loadFonts(float dpiScale) {
    ImGuiIO& io = ImGui::GetIO();

    std::error_code ec;
    if (std::filesystem::exists("C:/Windows/Fonts/segoeui.ttf", ec)) {
        s_body = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", 16.0f * dpiScale);
    }
    if (std::filesystem::exists("C:/Windows/Fonts/segoeuib.ttf", ec)) {
        s_heading = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeuib.ttf", 18.0f * dpiScale);
    }
    if (std::filesystem::exists("C:/Windows/Fonts/consola.ttf", ec)) {
        s_mono = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/consola.ttf", 15.0f * dpiScale);
    }

    if (!s_body) {
        s_body = io.Fonts->AddFontDefault();
    }
    if (!s_heading) {
        s_heading = s_body;
    }
    if (!s_mono) {
        s_mono = s_body;
    }
}

ImFont* body() {
    return s_body ? s_body : ImGui::GetFont();
}

ImFont* heading() {
    return s_heading ? s_heading : ImGui::GetFont();
}

ImFont* mono() {
    return s_mono ? s_mono : ImGui::GetFont();
}

}  // namespace diskscope::theme

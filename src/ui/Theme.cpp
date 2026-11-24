#include "ui/Theme.h"

#include <filesystem>
#include <system_error>

namespace diskscope::theme {

namespace {

Colors s_colors;
bool s_colorsInitialized = false;

Metrics s_metrics;

ImFont* s_caption = nullptr;
ImFont* s_body = nullptr;
ImFont* s_bodyStrong = nullptr;
ImFont* s_h3 = nullptr;
ImFont* s_h2 = nullptr;
ImFont* s_metric = nullptr;
ImFont* s_mono = nullptr;

void initColors() {
    if (s_colorsInitialized) {
        return;
    }

    // Reference palette: strict dark slate / deep forest tint with crisp emerald green accents.
    // Canonical base hex colors:
    //   bg:            #090B0A -> (  9,  11,  10)
    //   sidebar:       #0D100F -> ( 13,  16,  15)
    //   panel:         #111513 -> ( 17,  21,  19)
    //   elevated:      #171C19 -> ( 23,  28,  25)
    //   border:        #252D28 -> ( 37,  45,  40)
    //   green:         #16C76A -> ( 22, 199, 106)
    //   greenHover:    #20E879 -> ( 32, 232, 121)
    //   greenDark:     #103D27 -> ( 16,  61,  39)
    //   red:           #E5484D -> (229,  72,  77)
    //   redDark:       #471B1D -> ( 71,  27,  29)
    //   text:          #F1F5F2 -> (241, 245, 242)
    //   textSecondary: #929B95 -> (146, 155, 149)

    s_colors.bgV            = ImVec4(  9.0f / 255.0f,  11.0f / 255.0f,  10.0f / 255.0f, 1.0f);
    s_colors.sidebarV       = ImVec4( 13.0f / 255.0f,  16.0f / 255.0f,  15.0f / 255.0f, 1.0f);
    s_colors.panelV         = ImVec4( 17.0f / 255.0f,  21.0f / 255.0f,  19.0f / 255.0f, 1.0f);
    s_colors.elevatedV      = ImVec4( 23.0f / 255.0f,  28.0f / 255.0f,  25.0f / 255.0f, 1.0f);

    // Derived: elevatedHover = elevated lightened ~6% toward white: elevated + (1.0f - elevated) * 0.06f
    s_colors.elevatedHoverV = ImVec4(
        s_colors.elevatedV.x + (1.0f - s_colors.elevatedV.x) * 0.06f,
        s_colors.elevatedV.y + (1.0f - s_colors.elevatedV.y) * 0.06f,
        s_colors.elevatedV.z + (1.0f - s_colors.elevatedV.z) * 0.06f,
        1.0f);

    s_colors.borderV        = ImVec4( 37.0f / 255.0f,  45.0f / 255.0f,  40.0f / 255.0f, 1.0f);

    // Derived: borderStrong = border at 60% alpha composited over bg: bg + (border - bg) * 0.60f
    s_colors.borderStrongV  = ImVec4(
        s_colors.bgV.x + (s_colors.borderV.x - s_colors.bgV.x) * 0.60f,
        s_colors.bgV.y + (s_colors.borderV.y - s_colors.bgV.y) * 0.60f,
        s_colors.bgV.z + (s_colors.borderV.z - s_colors.bgV.z) * 0.60f,
        1.0f);

    s_colors.greenV         = ImVec4( 22.0f / 255.0f, 199.0f / 255.0f, 106.0f / 255.0f, 1.0f);
    s_colors.greenHoverV    = ImVec4( 32.0f / 255.0f, 232.0f / 255.0f, 121.0f / 255.0f, 1.0f);
    s_colors.greenDarkV     = ImVec4( 16.0f / 255.0f,  61.0f / 255.0f,  39.0f / 255.0f, 1.0f);

    // Derived: greenSoft = green at 14% alpha
    s_colors.greenSoftV     = ImVec4(s_colors.greenV.x, s_colors.greenV.y, s_colors.greenV.z, 0.14f);

    s_colors.redV           = ImVec4(229.0f / 255.0f,  72.0f / 255.0f,  77.0f / 255.0f, 1.0f);
    s_colors.redDarkV       = ImVec4( 71.0f / 255.0f,  27.0f / 255.0f,  29.0f / 255.0f, 1.0f);

    // Derived: redSoft = red at 14% alpha
    s_colors.redSoftV       = ImVec4(s_colors.redV.x, s_colors.redV.y, s_colors.redV.z, 0.14f);

    s_colors.textV          = ImVec4(241.0f / 255.0f, 245.0f / 255.0f, 242.0f / 255.0f, 1.0f);
    s_colors.textSecondaryV = ImVec4(146.0f / 255.0f, 155.0f / 255.0f, 149.0f / 255.0f, 1.0f);

    // Derived: textDisabled = textSecondary at 55% alpha
    s_colors.textDisabledV  = ImVec4(s_colors.textSecondaryV.x, s_colors.textSecondaryV.y, s_colors.textSecondaryV.z, 0.55f);

    // U32 representations
    s_colors.bg            = ImGui::ColorConvertFloat4ToU32(s_colors.bgV);
    s_colors.sidebar       = ImGui::ColorConvertFloat4ToU32(s_colors.sidebarV);
    s_colors.panel         = ImGui::ColorConvertFloat4ToU32(s_colors.panelV);
    s_colors.elevated      = ImGui::ColorConvertFloat4ToU32(s_colors.elevatedV);
    s_colors.elevatedHover = ImGui::ColorConvertFloat4ToU32(s_colors.elevatedHoverV);
    s_colors.border        = ImGui::ColorConvertFloat4ToU32(s_colors.borderV);
    s_colors.borderStrong  = ImGui::ColorConvertFloat4ToU32(s_colors.borderStrongV);
    s_colors.green         = ImGui::ColorConvertFloat4ToU32(s_colors.greenV);
    s_colors.greenHover    = ImGui::ColorConvertFloat4ToU32(s_colors.greenHoverV);
    s_colors.greenDark     = ImGui::ColorConvertFloat4ToU32(s_colors.greenDarkV);
    s_colors.greenSoft     = ImGui::ColorConvertFloat4ToU32(s_colors.greenSoftV);
    s_colors.red           = ImGui::ColorConvertFloat4ToU32(s_colors.redV);
    s_colors.redDark       = ImGui::ColorConvertFloat4ToU32(s_colors.redDarkV);
    s_colors.redSoft       = ImGui::ColorConvertFloat4ToU32(s_colors.redSoftV);
    s_colors.text          = ImGui::ColorConvertFloat4ToU32(s_colors.textV);
    s_colors.textSecondary = ImGui::ColorConvertFloat4ToU32(s_colors.textSecondaryV);
    s_colors.textDisabled  = ImGui::ColorConvertFloat4ToU32(s_colors.textDisabledV);

    // Backward-compatible aliases for existing view code
    s_colors.background  = s_colors.bg;
    s_colors.card        = s_colors.panel;
    s_colors.cardHover   = s_colors.elevated;
    s_colors.accent      = s_colors.green;
    s_colors.accentSoft  = s_colors.greenSoft;
    s_colors.textMuted   = s_colors.textSecondary;
    s_colors.good        = s_colors.green;
    s_colors.warn        = s_colors.red;
    s_colors.bad         = s_colors.red;

    s_colors.backgroundV = s_colors.bgV;
    s_colors.cardV       = s_colors.panelV;
    s_colors.cardHoverV  = s_colors.elevatedV;
    s_colors.accentV     = s_colors.greenV;
    s_colors.accentSoftV = s_colors.greenSoftV;
    s_colors.textMutedV  = s_colors.textSecondaryV;
    s_colors.goodV       = s_colors.greenV;
    s_colors.warnV       = s_colors.redV;
    s_colors.badV        = s_colors.redV;

    s_colorsInitialized = true;
}

}  // namespace

const Colors& colors() {
    if (!s_colorsInitialized) {
        initColors();
    }
    return s_colors;
}

const Metrics& metrics() {
    return s_metrics;
}

void apply(float dpiScale) {
    initColors();

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* c = style.Colors;

    c[ImGuiCol_Text]                  = s_colors.textV;
    c[ImGuiCol_TextDisabled]          = s_colors.textDisabledV;
    c[ImGuiCol_WindowBg]              = s_colors.bgV;
    c[ImGuiCol_ChildBg]               = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_PopupBg]               = s_colors.elevatedV;
    c[ImGuiCol_Border]                = s_colors.borderV;
    c[ImGuiCol_BorderShadow]          = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_FrameBg]               = s_colors.panelV;
    c[ImGuiCol_FrameBgHovered]        = s_colors.elevatedHoverV;
    c[ImGuiCol_FrameBgActive]         = s_colors.greenSoftV;
    c[ImGuiCol_TitleBg]               = s_colors.sidebarV;
    c[ImGuiCol_TitleBgActive]         = s_colors.sidebarV;
    c[ImGuiCol_TitleBgCollapsed]      = s_colors.sidebarV;
    c[ImGuiCol_MenuBarBg]             = s_colors.sidebarV;
    c[ImGuiCol_ScrollbarBg]           = s_colors.bgV;
    c[ImGuiCol_ScrollbarGrab]         = s_colors.elevatedHoverV;
    c[ImGuiCol_ScrollbarGrabHovered]  = s_colors.elevatedHoverV;
    c[ImGuiCol_ScrollbarGrabActive]   = s_colors.greenV;
    c[ImGuiCol_CheckMark]             = s_colors.greenV;
    c[ImGuiCol_SliderGrab]            = s_colors.greenV;
    c[ImGuiCol_SliderGrabActive]      = s_colors.greenHoverV;
    c[ImGuiCol_Button]                = s_colors.panelV;
    c[ImGuiCol_ButtonHovered]         = s_colors.elevatedHoverV;
    c[ImGuiCol_ButtonActive]          = s_colors.greenSoftV;
    c[ImGuiCol_Header]                = s_colors.greenSoftV;
    c[ImGuiCol_HeaderHovered]         = s_colors.elevatedHoverV;
    c[ImGuiCol_HeaderActive]          = s_colors.greenSoftV;
    c[ImGuiCol_Separator]             = s_colors.borderV;
    c[ImGuiCol_SeparatorHovered]      = s_colors.greenV;
    c[ImGuiCol_SeparatorActive]       = s_colors.greenV;
    c[ImGuiCol_ResizeGrip]            = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_ResizeGripHovered]     = s_colors.greenV;
    c[ImGuiCol_ResizeGripActive]      = s_colors.greenV;
    c[ImGuiCol_Tab]                   = s_colors.panelV;
    c[ImGuiCol_TabHovered]            = s_colors.elevatedHoverV;
    c[ImGuiCol_TabActive]             = s_colors.greenSoftV;
    c[ImGuiCol_TabUnfocused]          = s_colors.bgV;
    c[ImGuiCol_TabUnfocusedActive]    = s_colors.panelV;
    c[ImGuiCol_PlotLines]             = s_colors.greenV;
    c[ImGuiCol_PlotLinesHovered]      = s_colors.greenHoverV;
    c[ImGuiCol_PlotHistogram]         = s_colors.greenV;
    c[ImGuiCol_PlotHistogramHovered]  = s_colors.greenHoverV;
    c[ImGuiCol_TableHeaderBg]         = s_colors.panelV;
    c[ImGuiCol_TableBorderStrong]     = s_colors.borderV;
    c[ImGuiCol_TableBorderLight]      = ImVec4(s_colors.borderV.x, s_colors.borderV.y, s_colors.borderV.z, 0.5f);
    c[ImGuiCol_TableRowBg]            = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_TableRowBgAlt]         = ImVec4(1.0f, 1.0f, 1.0f, 0.02f);
    c[ImGuiCol_TextSelectedBg]        = s_colors.greenSoftV;
    c[ImGuiCol_DragDropTarget]        = s_colors.greenV;
    c[ImGuiCol_NavHighlight]          = s_colors.greenV;
    c[ImGuiCol_ModalWindowDimBg]      = ImVec4(s_colors.bgV.x, s_colors.bgV.y, s_colors.bgV.z, 0.55f);

    style.WindowRounding    = 12.0f;
    style.ChildRounding     = 12.0f;
    style.FrameRounding     = 8.0f;
    style.PopupRounding     = 10.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding      = 6.0f;
    style.TabRounding       = 8.0f;

    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.PopupBorderSize   = 1.0f;
    style.TabBorderSize     = 1.0f;

    style.WindowPadding     = ImVec2(16.0f * dpiScale, 16.0f * dpiScale);
    style.FramePadding      = ImVec2(10.0f * dpiScale, 6.0f * dpiScale);
    style.ItemSpacing       = ImVec2(10.0f * dpiScale, 8.0f * dpiScale);
    style.ItemInnerSpacing  = ImVec2(6.0f * dpiScale, 6.0f * dpiScale);
    style.ScrollbarSize     = 10.0f * dpiScale;
}

void loadFonts(float dpiScale) {
    ImGuiIO& io = ImGui::GetIO();
    std::error_code ec;

    const char* regularPath = "C:/Windows/Fonts/segoeui.ttf";
    bool hasRegular = std::filesystem::exists(regularPath, ec);

    const char* semiboldPath = nullptr;
    if (std::filesystem::exists("C:/Windows/Fonts/seguisb.ttf", ec)) {
        semiboldPath = "C:/Windows/Fonts/seguisb.ttf";
    } else if (std::filesystem::exists("C:/Windows/Fonts/segoeuib.ttf", ec)) {
        semiboldPath = "C:/Windows/Fonts/segoeuib.ttf";
    }

    const char* monoPath = nullptr;
    if (std::filesystem::exists("C:/Windows/Fonts/CascadiaMono.ttf", ec)) {
        monoPath = "C:/Windows/Fonts/CascadiaMono.ttf";
    } else if (std::filesystem::exists("C:/Windows/Fonts/consola.ttf", ec)) {
        monoPath = "C:/Windows/Fonts/consola.ttf";
    }

    // Load regular sizes
    if (hasRegular) {
        s_caption = io.Fonts->AddFontFromFileTTF(regularPath, 11.0f * dpiScale);
        s_body    = io.Fonts->AddFontFromFileTTF(regularPath, 14.0f * dpiScale);
    }

    // Load semibold sizes
    const char* semiOrReg = semiboldPath ? semiboldPath : (hasRegular ? regularPath : nullptr);
    if (semiOrReg) {
        s_bodyStrong = io.Fonts->AddFontFromFileTTF(semiOrReg, 15.0f * dpiScale);
        s_h3         = io.Fonts->AddFontFromFileTTF(semiOrReg, 17.0f * dpiScale);
        s_h2         = io.Fonts->AddFontFromFileTTF(semiOrReg, 20.0f * dpiScale);
        s_metric     = io.Fonts->AddFontFromFileTTF(semiOrReg, 26.0f * dpiScale);
    }

    // Load mono size
    if (monoPath) {
        s_mono = io.Fonts->AddFontFromFileTTF(monoPath, 13.0f * dpiScale);
    }

    // Robust fallbacks so missing fonts can never crash
    if (!s_body) {
        s_body = io.Fonts->AddFontDefault();
    }
    if (!s_caption) {
        s_caption = s_body;
    }
    if (!s_bodyStrong) {
        s_bodyStrong = s_body;
    }
    if (!s_h3) {
        s_h3 = s_body;
    }
    if (!s_h2) {
        s_h2 = s_h3;
    }
    if (!s_metric) {
        s_metric = s_h2;
    }
    if (!s_mono) {
        s_mono = s_body;
    }
}

ImFont* caption() {
    return s_caption ? s_caption : ImGui::GetFont();
}

ImFont* body() {
    return s_body ? s_body : ImGui::GetFont();
}

ImFont* bodyStrong() {
    return s_bodyStrong ? s_bodyStrong : ImGui::GetFont();
}

ImFont* h3() {
    return s_h3 ? s_h3 : ImGui::GetFont();
}

ImFont* h2() {
    return s_h2 ? s_h2 : ImGui::GetFont();
}

ImFont* metric() {
    return s_metric ? s_metric : ImGui::GetFont();
}

ImFont* mono() {
    return s_mono ? s_mono : ImGui::GetFont();
}

ImFont* heading() {
    return h3();
}

ImU32 chartColour(int index) {
    if (!s_colorsInitialized) {
        initColors();
    }

    const ImU32 ramp[7] = {
        s_colors.green,
        s_colors.greenHover,
        s_colors.greenDark,
        s_colors.borderStrong,
        s_colors.textSecondary,
        s_colors.red,
        s_colors.redDark
    };

    int idx = index % 7;
    if (idx < 0) {
        idx += 7;
    }
    return ramp[idx];
}

}  // namespace diskscope::theme

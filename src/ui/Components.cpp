#include "ui/Components.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace diskscope::ui {

namespace {

struct ToneColors {
    ImU32 fill;
    ImU32 stroke;
    ImU32 text;
    ImVec4 fillV;
    ImVec4 strokeV;
    ImVec4 textV;
};

ToneColors getToneColors(Tone tone) {
    const auto& c = theme::colors();
    ToneColors tc{};
    switch (tone) {
        case Tone::Green: {
            tc.fill    = c.greenSoft;
            tc.stroke  = c.green;
            tc.text    = c.green;
            tc.fillV   = c.greenSoftV;
            tc.strokeV = c.greenV;
            tc.textV   = c.greenV;
            break;
        }
        case Tone::Red: {
            tc.fill    = c.redSoft;
            tc.stroke  = c.red;
            tc.text    = c.red;
            tc.fillV   = c.redSoftV;
            tc.strokeV = c.redV;
            tc.textV   = c.redV;
            break;
        }
        case Tone::Neutral: {
            tc.fill    = c.elevated;
            tc.stroke  = c.border;
            tc.text    = c.text;
            tc.fillV   = c.elevatedV;
            tc.strokeV = c.borderV;
            tc.textV   = c.textV;
            break;
        }
        case Tone::Muted:
        default: {
            tc.fill    = c.panel;
            tc.stroke  = c.border;
            tc.text    = c.textSecondary;
            tc.fillV   = c.panelV;
            tc.strokeV = c.borderV;
            tc.textV   = c.textSecondaryV;
            break;
        }
    }
    return tc;
}

}  // namespace

bool Button(const char* id,
            const char* label,
            ButtonRole role,
            ButtonSize size,
            Icon icon,
            float width,
            bool enabled,
            bool loading,
            const char* tooltip) {
    const auto& c = theme::colors();
    const auto& m = theme::metrics();

    float btnH = 34.0f;
    float iconSz = 16.0f;
    float padX = 14.0f;
    ImFont* font = theme::body();

    if (size == ButtonSize::Small) {
        btnH = 28.0f;
        iconSz = 14.0f;
        padX = 10.0f;
        font = theme::body();
    } else if (size == ButtonSize::Large) {
        btnH = 40.0f;
        iconSz = 18.0f;
        padX = 18.0f;
        font = theme::bodyStrong();
    } else {
        font = theme::bodyStrong();
    }

    ImVec4 bgCol = c.elevatedV;
    ImVec4 bgHoverCol = c.elevatedHoverV;
    ImVec4 bgActiveCol = c.greenSoftV;
    ImVec4 borderCol = c.borderV;
    ImVec4 textCol = c.textV;
    const ImU32 darkInk = IM_COL32(10, 26, 18, 255);

    switch (role) {
        case ButtonRole::Primary: {
            bgCol = c.greenV;
            bgHoverCol = c.greenHoverV;
            bgActiveCol = c.greenDarkV;
            borderCol = c.greenV;
            textCol = ImVec4(10.0f / 255.0f, 26.0f / 255.0f, 18.0f / 255.0f, 1.0f);
            break;
        }
        case ButtonRole::Secondary: {
            bgCol = c.elevatedV;
            bgHoverCol = c.elevatedHoverV;
            bgActiveCol = c.greenSoftV;
            borderCol = c.borderV;
            textCol = c.textV;
            break;
        }
        case ButtonRole::Danger: {
            bgCol = c.redDarkV;
            bgHoverCol = ImVec4(c.redV.x, c.redV.y, c.redV.z, 0.22f);
            bgActiveCol = c.redDarkV;
            borderCol = ImVec4(c.redV.x, c.redV.y, c.redV.z, 0.40f);
            textCol = c.redV;
            break;
        }
        case ButtonRole::Ghost: {
            bgCol = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
            bgHoverCol = c.elevatedV;
            bgActiveCol = c.elevatedHoverV;
            borderCol = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
            textCol = c.textV;
            break;
        }
    }

    if (!enabled) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.45f);
    }

    ImGui::PushFont(font);
    const ImVec2 textSize = ImGui::CalcTextSize(label ? label : "");
    const bool hasLeading = (icon != Icon::None || loading);
    const float contentW = textSize.x + (hasLeading ? (iconSz + 6.0f) : 0.0f);
    const float btnW = (width > 0.0f) ? width : (contentW + padX * 2.0f);

    const ImVec2 pos = ImGui::GetCursorScreenPos();
    bool clicked = ImGui::InvisibleButton(id, ImVec2(btnW, btnH));
    const bool hovered = ImGui::IsItemHovered();
    const bool held = ImGui::IsItemActive();

    if (!enabled || loading) {
        clicked = false;
    }

    ImDrawList* list = ImGui::GetWindowDrawList();
    const ImVec2 min = pos;
    const ImVec2 max(pos.x + btnW, pos.y + btnH);

    ImVec4 currentBg = bgCol;
    if (held && enabled) {
        currentBg = bgActiveCol;
    } else if (hovered && enabled) {
        currentBg = bgHoverCol;
    }

    list->AddRectFilled(min, max, ImGui::ColorConvertFloat4ToU32(currentBg), m.controlRadius);
    if (borderCol.w > 0.0f) {
        list->AddRect(min, max, ImGui::ColorConvertFloat4ToU32(borderCol), m.controlRadius, 0, 1.0f);
    }

    const ImU32 inkU32 = (role == ButtonRole::Primary) ? darkInk : ImGui::ColorConvertFloat4ToU32(textCol);
    const float startX = pos.x + (btnW - contentW) * 0.5f;
    float textStartX = startX;

    if (loading) {
        const ImVec2 spinnerCentre(startX + iconSz * 0.5f, pos.y + btnH * 0.5f);
        const float spinnerRadius = iconSz * 0.42f;
        const float t = static_cast<float>(ImGui::GetTime()) * 6.0f;
        list->PathArcTo(spinnerCentre, spinnerRadius, t, t + 4.5f, 16);
        list->PathStroke(inkU32, 0, 1.8f);
        textStartX += iconSz + 6.0f;
    } else if (icon != Icon::None) {
        const ImVec2 iconCentre(startX + iconSz * 0.5f, pos.y + btnH * 0.5f);
        DrawIcon(list, icon, iconCentre, iconSz, inkU32, 1.5f);
        textStartX += iconSz + 6.0f;
    }

    if (label && label[0] != '\0') {
        const ImVec2 textPos(textStartX, pos.y + (btnH - textSize.y) * 0.5f);
        list->AddText(font, font->FontSize, textPos, inkU32, label);
    }

    ImGui::PopFont();

    if (!enabled) {
        ImGui::PopStyleVar();
    }

    if (tooltip && hovered && enabled) {
        Tooltip(tooltip);
    }

    return clicked;
}

bool BeginCard(const char* id, const CardOptions& options) {
    const auto& c = theme::colors();
    const auto& m = theme::metrics();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, c.panelV);
    ImGui::PushStyleColor(ImGuiCol_Border, c.borderV);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, m.cardRadius);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        options.padding ? ImVec2(m.cardPadding, m.cardPadding) : ImVec2(0.0f, 0.0f));

    const ImVec2 childSize(0.0f, options.height);
    ImGuiChildFlags childFlags = ImGuiChildFlags_Borders;
    if (options.autoHeight) {
        childFlags |= ImGuiChildFlags_AutoResizeY;
    }
    const bool visible = ImGui::BeginChild(id, childSize, childFlags, ImGuiWindowFlags_None);

    if (visible && options.header && options.title) {
        if (options.icon != Icon::None) {
            const ImVec2 p = ImGui::GetCursorScreenPos();
            const float sz = m.iconSize;
            const ImVec2 centre(p.x + sz * 0.5f, p.y + sz * 0.5f + 1.0f);
            DrawIcon(ImGui::GetWindowDrawList(), options.icon, centre, sz, c.green);
            ImGui::Dummy(ImVec2(sz, sz));
            ImGui::SameLine(0.0f, 8.0f);
        }

        ImGui::PushFont(theme::h3());
        ImGui::TextColored(c.textV, "%s", options.title);
        ImGui::PopFont();

        if (options.subtitle && options.subtitle[0] != '\0') {
            ImGui::PushFont(theme::caption());
            ImGui::TextColored(c.textSecondaryV, "%s", options.subtitle);
            ImGui::PopFont();
        }

        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        Divider(false);
        ImGui::Dummy(ImVec2(0.0f, m.gap));
    }

    return visible;
}

void EndCard() {
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

bool BeginCardFill(const char* id, const CardOptions& options) {
    CardOptions opt = options;
    opt.height = 0.0f;
    return BeginCard(id, opt);
}

void SectionHeader(Icon icon, const char* title, const char* subtitle) {
    const auto& c = theme::colors();

    if (icon != Icon::None) {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float sz = 22.0f;
        const ImVec2 centre(p.x + sz * 0.5f, p.y + sz * 0.5f + 1.0f);
        DrawIcon(ImGui::GetWindowDrawList(), icon, centre, sz, c.green);
        ImGui::Dummy(ImVec2(sz, sz));
        ImGui::SameLine(0.0f, 10.0f);
    }

    ImGui::PushFont(theme::h2());
    ImGui::TextColored(c.textV, "%s", title ? title : "");
    ImGui::PopFont();

    if (subtitle && subtitle[0] != '\0') {
        ImGui::PushFont(theme::caption());
        ImGui::TextColored(c.textSecondaryV, "%s", subtitle);
        ImGui::PopFont();
    }

    ImGui::Dummy(ImVec2(0.0f, 16.0f));
}

bool StatCard(const char* id,
              Icon icon,
              const char* label,
              const char* value,
              const char* sub,
              Tone tone,
              ImVec2 size) {
    const auto& c = theme::colors();
    const auto& m = theme::metrics();
    const ToneColors tc = getToneColors(tone);

    const float cardH = (size.y > 0.0f) ? size.y : 96.0f;
    const float cardW = size.x;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, c.panelV);
    ImGui::PushStyleColor(ImGuiCol_Border, c.borderV);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, m.cardRadius);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(m.cardPadding, 12.0f));

    bool clicked = false;
    if (ImGui::BeginChild(id, ImVec2(cardW, cardH), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar)) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 p = ImGui::GetCursorScreenPos();

        // 32x32 rounded icon tile
        if (icon != Icon::None) {
            const ImVec2 tileMin = p;
            const ImVec2 tileMax(p.x + 32.0f, p.y + 32.0f);
            dl->AddRectFilled(tileMin, tileMax, tc.fill, 8.0f);
            dl->AddRect(tileMin, tileMax, tc.stroke, 8.0f, 0, 1.0f);
            DrawIcon(dl, icon, ImVec2(p.x + 16.0f, p.y + 16.0f), 18.0f, tc.stroke);

            ImGui::Dummy(ImVec2(32.0f, 32.0f));
            ImGui::SameLine(0.0f, 12.0f);
        }

        ImGui::BeginGroup();

        ImGui::PushFont(theme::caption());
        ImGui::TextColored(c.textSecondaryV, "%s", label ? label : "");
        ImGui::PopFont();

        ImGui::PushFont(theme::metric());
        ImGui::TextColored(c.textV, "%s", value ? value : "");
        ImGui::PopFont();

        if (sub && sub[0] != '\0') {
            ImGui::PushFont(theme::caption());
            ImGui::TextColored(c.textSecondaryV, "%s", sub);
            ImGui::PopFont();
        }

        ImGui::EndGroup();

        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
            clicked = true;
        }
    }
    ImGui::EndChild();

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);

    return clicked;
}

bool StatCard(const char* id,
              Icon icon,
              const char* label,
              const char* value,
              const char* sub,
              Tone tone) {
    return StatCard(id, icon, label, value, sub, tone, ImVec2(0.0f, 96.0f));
}

bool SearchInput(const char* id, std::string& value, const char* hint, float width) {
    const auto& c = theme::colors();
    const auto& m = theme::metrics();

    const float w = (width > 0.0f) ? width : ImGui::GetContentRegionAvail().x;
    const ImVec2 pos = ImGui::GetCursorScreenPos();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(32.0f, 7.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, m.controlRadius);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, c.panelV);
    ImGui::PushStyleColor(ImGuiCol_Border, c.borderV);

    ImGui::SetNextItemWidth(w);

    char buf[256];
    const std::size_t copyLen = std::min(value.size(), sizeof(buf) - 1);
    std::memcpy(buf, value.data(), copyLen);
    buf[copyLen] = '\0';

    bool changed = ImGui::InputTextWithHint(id, hint ? hint : "", buf, sizeof(buf));
    if (changed) {
        value = buf;
    }

    const float frameH = ImGui::GetFrameHeight();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Leading search icon
    const ImVec2 searchCentre(pos.x + 16.0f, pos.y + frameH * 0.5f);
    DrawIcon(dl, Icon::Search, searchCentre, 15.0f, c.textSecondary);

    // In-field clear button
    if (!value.empty()) {
        const ImVec2 clearCentre(pos.x + w - 16.0f, pos.y + frameH * 0.5f);
        const ImVec2 mousePos = ImGui::GetIO().MousePos;
        const bool hovered = (std::abs(mousePos.x - clearCentre.x) <= 10.0f &&
                              std::abs(mousePos.y - clearCentre.y) <= 10.0f);

        if (hovered && ImGui::IsMouseClicked(0)) {
            value.clear();
            changed = true;
        }

        const ImU32 clearCol = hovered ? c.text : c.textSecondary;
        DrawIcon(dl, Icon::Close, clearCentre, 13.0f, clearCol, 1.4f);
    }

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);

    return changed;
}

bool Segmented(const char* id, const char* const* labels, int count, int& index, float width) {
    if (count <= 0 || !labels) {
        return false;
    }

    const auto& c = theme::colors();
    const auto& m = theme::metrics();
    bool changed = false;

    ImGui::PushID(id);
    const ImVec2 startPos = ImGui::GetCursorScreenPos();
    const float availW = ImGui::GetContentRegionAvail().x;
    // width == 0 fills the row, which suits full-width switches; a fixed width
    // keeps a compact toggle from eating the space next to it.
    const float totalW = (width > 0.0f) ? (std::min)(width, availW) : availW;
    const float segW = (totalW > 0.0f) ? (totalW / static_cast<float>(count)) : 80.0f;
    const float segH = 32.0f;

    ImDrawList* dl = ImGui::GetWindowDrawList();

    for (int i = 0; i < count; ++i) {
        const bool active = (index == i);
        const ImVec2 segMin(startPos.x + static_cast<float>(i) * segW, startPos.y);
        const ImVec2 segMax(segMin.x + segW, segMin.y + segH);

        char segId[32];
        std::snprintf(segId, sizeof(segId), "##seg_%d", i);

        ImGui::SetCursorScreenPos(segMin);
        const bool clicked = ImGui::InvisibleButton(segId, ImVec2(segW, segH));
        const bool hovered = ImGui::IsItemHovered();

        if (clicked && index != i) {
            index = i;
            changed = true;
        }

        ImU32 bg = c.elevated;
        ImU32 border = c.border;
        ImU32 ink = c.textSecondary;

        if (active) {
            bg = c.greenDark;
            border = c.green;
            ink = c.green;
        } else if (hovered) {
            bg = c.elevatedHover;
            border = c.borderStrong;
            ink = c.text;
        }

        ImDrawFlags roundingFlags = 0;
        if (i == 0) {
            roundingFlags = ImDrawFlags_RoundCornersLeft;
        } else if (i == count - 1) {
            roundingFlags = ImDrawFlags_RoundCornersRight;
        } else {
            roundingFlags = ImDrawFlags_RoundCornersNone;
        }

        dl->AddRectFilled(segMin, segMax, bg, m.controlRadius, roundingFlags);
        dl->AddRect(segMin, segMax, border, m.controlRadius, roundingFlags, 1.0f);

        ImFont* font = active ? theme::bodyStrong() : theme::body();
        const ImVec2 labelSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, labels[i]);
        const ImVec2 textPos(segMin.x + (segW - labelSize.x) * 0.5f, segMin.y + (segH - labelSize.y) * 0.5f);
        dl->AddText(font, font->FontSize, textPos, ink, labels[i]);
    }

    ImGui::PopID();
    ImGui::SetCursorScreenPos(ImVec2(startPos.x, startPos.y + segH));
    return changed;
}

void StatusPill(Tone tone, const char* text) {
    if (!text || text[0] == '\0') {
        return;
    }

    const ToneColors tc = getToneColors(tone);
    ImFont* font = theme::caption();

    const ImVec2 textSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, text);
    const float pillH = 22.0f;
    const float pillW = textSize.x + 28.0f;

    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    dl->AddRectFilled(p, ImVec2(p.x + pillW, p.y + pillH), tc.fill, pillH * 0.5f);
    dl->AddRect(p, ImVec2(p.x + pillW, p.y + pillH), tc.stroke, pillH * 0.5f, 0, 1.0f);

    // Indicator dot
    dl->AddCircleFilled(ImVec2(p.x + 10.0f, p.y + pillH * 0.5f), 3.0f, tc.stroke);

    // Text
    const ImVec2 textPos(p.x + 18.0f, p.y + (pillH - textSize.y) * 0.5f);
    dl->AddText(font, font->FontSize, textPos, tc.text, text);

    ImGui::Dummy(ImVec2(pillW, pillH));
}

void ProgressBar(float fraction, Tone tone, float height, const char* overlay) {
    const auto& c = theme::colors();
    const ToneColors tc = getToneColors(tone);

    const float w = ImGui::GetContentRegionAvail().x;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Track
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + height), c.border, height * 0.5f);

    if (fraction >= 0.0f) {
        const float clamped = std::clamp(fraction, 0.0f, 1.0f);
        if (clamped > 0.0f) {
            dl->AddRectFilled(p, ImVec2(p.x + w * clamped, p.y + height), tc.stroke, height * 0.5f);
        }
    } else {
        // Indeterminate animation
        const float t = static_cast<float>(ImGui::GetTime());
        const float barW = w * 0.35f;
        const float cycle = std::fmod(t * 0.85f, 1.0f);
        const float startX = p.x + (w + barW) * cycle - barW;
        const float minX = std::max(p.x, startX);
        const float maxX = std::min(p.x + w, startX + barW);
        if (maxX > minX) {
            dl->AddRectFilled(ImVec2(minX, p.y), ImVec2(maxX, p.y + height), tc.stroke, height * 0.5f);
        }
    }

    if (overlay && overlay[0] != '\0') {
        ImFont* font = theme::caption();
        const ImVec2 sz = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, overlay);
        const ImVec2 textPos(p.x + (w - sz.x) * 0.5f, p.y + (height - sz.y) * 0.5f);
        dl->AddText(font, font->FontSize, textPos, c.text, overlay);
    }

    ImGui::Dummy(ImVec2(w, height));
}

void BarMeter(float fraction, float width, Tone tone, float height) {
    const auto& c = theme::colors();
    const ToneColors tc = getToneColors(tone);

    const float clamped = std::clamp(fraction, 0.0f, 1.0f);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Track sits one step above the panel, otherwise it disappears on card
    // backgrounds and the fill reads as a floating bar.
    dl->AddRectFilled(p, ImVec2(p.x + width, p.y + height), c.elevated, height * 0.5f);

    // Active fill
    if (clamped > 0.0f) {
        dl->AddRectFilled(p, ImVec2(p.x + width * clamped, p.y + height), tc.stroke, height * 0.5f);
    }

    ImGui::Dummy(ImVec2(width, height));
}

void Badge(const char* text, Tone tone) {
    if (!text || text[0] == '\0') {
        return;
    }

    const ToneColors tc = getToneColors(tone);
    ImFont* font = theme::caption();

    const ImVec2 textSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, text);
    const float badgeH = 18.0f;
    const float badgeW = textSize.x + 12.0f;

    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    dl->AddRectFilled(p, ImVec2(p.x + badgeW, p.y + badgeH), tc.fill, 4.0f);
    dl->AddRect(p, ImVec2(p.x + badgeW, p.y + badgeH), tc.stroke, 4.0f, 0, 1.0f);

    const ImVec2 textPos(p.x + 6.0f, p.y + (badgeH - textSize.y) * 0.5f);
    dl->AddText(font, font->FontSize, textPos, tc.text, text);

    ImGui::Dummy(ImVec2(badgeW, badgeH));
}

void Tooltip(const char* text) {
    if (!text || text[0] == '\0') {
        return;
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        const auto& c = theme::colors();
        ImGui::PushStyleColor(ImGuiCol_PopupBg, c.elevatedV);
        ImGui::PushStyleColor(ImGuiCol_Border, c.borderStrongV);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));

        ImGui::BeginTooltip();
        ImGui::PushFont(theme::caption());
        ImGui::TextColored(c.textV, "%s", text);
        ImGui::PopFont();
        ImGui::EndTooltip();

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(2);
    }
}

void Divider(bool vertical) {
    const auto& c = theme::colors();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    if (vertical) {
        const float h = ImGui::GetContentRegionAvail().y;
        dl->AddLine(p, ImVec2(p.x, p.y + h), c.border, 1.0f);
        ImGui::Dummy(ImVec2(1.0f, h));
    } else {
        const float w = ImGui::GetContentRegionAvail().x;
        dl->AddLine(p, ImVec2(p.x + w, p.y), c.border, 1.0f);
        ImGui::Dummy(ImVec2(w, 1.0f));
    }
}

void KeyValueRow(const char* label, const char* value, float labelWidth) {
    const auto& c = theme::colors();

    ImGui::PushFont(theme::caption());
    ImGui::TextColored(c.textSecondaryV, "%s", label ? label : "");
    ImGui::PopFont();

    ImGui::SameLine(labelWidth);

    ImGui::PushFont(theme::bodyStrong());
    ImGui::TextColored(c.textV, "%s", value ? value : "");
    ImGui::PopFont();
}

void EmptyState(Icon icon, const char* title, const char* body) {
    const auto& c = theme::colors();
    const ImVec2 avail = ImGui::GetContentRegionAvail();

    ImGui::Dummy(ImVec2(0.0f, std::max(20.0f, (avail.y - 120.0f) * 0.35f)));

    if (icon != Icon::None) {
        const float sz = 38.0f;
        const float iconX = ImGui::GetCursorPosX() + (avail.x - sz) * 0.5f;
        ImGui::SetCursorPosX(iconX);
        const ImVec2 p = ImGui::GetCursorScreenPos();
        DrawIcon(ImGui::GetWindowDrawList(), icon, ImVec2(p.x + sz * 0.5f, p.y + sz * 0.5f), sz, c.textSecondary);
        ImGui::Dummy(ImVec2(sz, sz));
        ImGui::Dummy(ImVec2(0.0f, 8.0f));
    }

    if (title && title[0] != '\0') {
        ImGui::PushFont(theme::h3());
        const float titleW = ImGui::CalcTextSize(title).x;
        if (avail.x > titleW) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - titleW) * 0.5f);
        }
        ImGui::TextColored(c.textV, "%s", title);
        ImGui::PopFont();
    }

    if (body && body[0] != '\0') {
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        ImGui::PushFont(theme::caption());
        const float bodyW = ImGui::CalcTextSize(body).x;
        if (avail.x > bodyW) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - bodyW) * 0.5f);
        }
        ImGui::TextColored(c.textSecondaryV, "%s", body);
        ImGui::PopFont();
    }
}

void ErrorState(const char* title, const char* body) {
    const auto& c = theme::colors();
    const ImVec2 avail = ImGui::GetContentRegionAvail();

    ImGui::Dummy(ImVec2(0.0f, std::max(20.0f, (avail.y - 120.0f) * 0.35f)));

    const float sz = 38.0f;
    const float iconX = ImGui::GetCursorPosX() + (avail.x - sz) * 0.5f;
    ImGui::SetCursorPosX(iconX);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    DrawIcon(ImGui::GetWindowDrawList(), Icon::AlertTriangle, ImVec2(p.x + sz * 0.5f, p.y + sz * 0.5f), sz, c.red);
    ImGui::Dummy(ImVec2(sz, sz));
    ImGui::Dummy(ImVec2(0.0f, 8.0f));

    if (title && title[0] != '\0') {
        ImGui::PushFont(theme::h3());
        const float titleW = ImGui::CalcTextSize(title).x;
        if (avail.x > titleW) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - titleW) * 0.5f);
        }
        ImGui::TextColored(c.redV, "%s", title);
        ImGui::PopFont();
    }

    if (body && body[0] != '\0') {
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        ImGui::PushFont(theme::caption());
        const float bodyW = ImGui::CalcTextSize(body).x;
        if (avail.x > bodyW) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - bodyW) * 0.5f);
        }
        ImGui::TextColored(c.textSecondaryV, "%s", body);
        ImGui::PopFont();
    }
}

ConfirmResult ConfirmDialog(const char* id,
                           bool* open,
                           const char* title,
                           const char* body,
                           const char* confirmLabel,
                           const char* cancelLabel,
                           bool danger) {
    ConfirmResult res{};
    if (!open) {
        return res;
    }

    if (*open && !ImGui::IsPopupOpen(id)) {
        ImGui::OpenPopup(id);
    }

    const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(380.0f, 0.0f), ImGuiCond_Appearing);

    ImGui::PushStyleColor(ImGuiCol_PopupBg, theme::colors().panelV);
    ImGui::PushStyleColor(ImGuiCol_Border, theme::colors().borderV);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 20.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, theme::metrics().cardRadius);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);

    if (ImGui::BeginPopupModal(id, open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar)) {
        res.open = true;

        ImGui::PushFont(theme::h3());
        ImGui::TextColored(theme::colors().textV, "%s", title ? title : "");
        ImGui::PopFont();

        ImGui::Dummy(ImVec2(0.0f, 6.0f));
        if (body && body[0] != '\0') {
            ImGui::PushFont(theme::caption());
            ImGui::TextWrapped("%s", body);
            ImGui::PopFont();
        }

        ImGui::Dummy(ImVec2(0.0f, 16.0f));

        const float btnW = 95.0f;
        const float spacing = 10.0f;
        const float totalW = btnW * 2.0f + spacing;
        const float avail = ImGui::GetContentRegionAvail().x;
        if (avail > totalW) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - totalW));
        }

        if (Button("##Cancel", cancelLabel ? cancelLabel : "Cancel", ButtonRole::Secondary, ButtonSize::Medium, Icon::None, btnW)) {
            *open = false;
            res.open = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine(0.0f, spacing);

        const ButtonRole confRole = danger ? ButtonRole::Danger : ButtonRole::Primary;
        if (Button("##Confirm", confirmLabel ? confirmLabel : "Confirm", confRole, ButtonSize::Medium, Icon::None, btnW)) {
            *open = false;
            res.open = false;
            res.confirmed = true;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    } else {
        res.open = false;
    }

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);

    return res;
}

bool BeginTable(const char* id, int columns, ImGuiTableFlags extraFlags, ImVec2 size) {
    const ImGuiTableFlags flags = ImGuiTableFlags_RowBg
                                | ImGuiTableFlags_BordersInnerH
                                | ImGuiTableFlags_NoBordersInBody
                                | extraFlags;

    const auto& c = theme::colors();
    ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, c.panelV);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, c.greenSoftV);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, c.greenSoftV);
    ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, c.borderV);
    ImGui::PushStyleColor(ImGuiCol_TableBorderLight, c.borderV);

    const float padY = std::max(2.0f, (theme::metrics().tableRowHeight - ImGui::GetTextLineHeight()) * 0.5f);
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.0f, padY));

    const bool open = ImGui::BeginTable(id, columns, flags, size);
    if (!open) {
        ImGui::PopStyleVar(1);
        ImGui::PopStyleColor(5);
    }
    return open;
}

void TableHeader(const char* const* labels, int count) {
    if (!labels) {
        return;
    }
    for (int i = 0; i < count; ++i) {
        ImGui::TableSetupColumn(labels[i]);
    }
    ImGui::TableHeadersRow();
}

void EndTable() {
    ImGui::EndTable();
    ImGui::PopStyleVar(1);
    ImGui::PopStyleColor(5);
}

bool NavItem(const char* id, Icon icon, const char* label, bool active) {
    const auto& c = theme::colors();
    const auto& m = theme::metrics();

    const float h = m.navRowHeight;
    const float w = ImGui::GetContentRegionAvail().x;
    const ImVec2 pos = ImGui::GetCursorScreenPos();

    const bool clicked = ImGui::InvisibleButton(id, ImVec2(w, h));
    const bool hovered = ImGui::IsItemHovered();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 min = pos;
    const ImVec2 max(pos.x + w, pos.y + h);

    ImU32 inkCol = c.textSecondary;

    if (active) {
        // Active state: greenDark fill + 3px green left indicator + green ink
        dl->AddRectFilled(min, max, c.greenDark, 8.0f);
        dl->AddRectFilled(ImVec2(min.x, min.y + 10.0f), ImVec2(min.x + 3.0f, max.y - 10.0f), c.green, 1.5f);
        inkCol = c.green;
    } else if (hovered) {
        // Hover state: greenSoft fill + white/text ink
        dl->AddRectFilled(min, max, c.greenSoft, 8.0f);
        inkCol = c.text;
    }

    // Icon + label
    const float iconX = pos.x + 16.0f;
    const float centerY = pos.y + h * 0.5f;
    DrawIcon(dl, icon, ImVec2(iconX + 10.0f, centerY), 20.0f, inkCol);

    ImFont* font = active ? theme::bodyStrong() : theme::body();
    const ImVec2 textSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, label ? label : "");
    const ImVec2 textPos(iconX + 20.0f + 12.0f, centerY - textSize.y * 0.5f);
    dl->AddText(font, font->FontSize, textPos, inkCol, label ? label : "");

    return clicked;
}

}  // namespace diskscope::ui
namespace diskscope::ui {

bool Switch(const char* label, bool* value, const char* description) {
    if (value == nullptr) {
        return false;
    }

    const theme::Colors& c = theme::colors();

    const float trackW = 38.0f;
    const float trackH = 20.0f;
    const float rowH = 24.0f;

    const ImVec2 start = ImGui::GetCursorScreenPos();
    const float totalW = ImGui::GetContentRegionAvail().x;

    ImGui::PushID(label);
    const bool clicked = ImGui::InvisibleButton("##switchRow", ImVec2(totalW, rowH));
    const bool hovered = ImGui::IsItemHovered();
    if (clicked) {
        *value = !*value;
    }

    // The knob eases toward its target through per-ID storage: the transition
    // survives between frames without the view owning any animation state.
    float* anim = ImGui::GetStateStorage()->GetFloatRef(ImGui::GetID("##knobAnim"),
                                                        *value ? 1.0f : 0.0f);
    const float target = *value ? 1.0f : 0.0f;
    *anim += (target - *anim) * std::min(1.0f, ImGui::GetIO().DeltaTime * 18.0f);

    ImDrawList* list = ImGui::GetWindowDrawList();
    const ImVec2 trackMin(start.x, start.y + (rowH - trackH) * 0.5f);
    const ImVec2 trackMax(trackMin.x + trackW, trackMin.y + trackH);
    const ImU32 trackColour = *value ? c.greenDark : c.elevated;
    const ImU32 borderColour = *value ? c.green : (hovered ? c.borderStrong : c.border);
    list->AddRectFilled(trackMin, trackMax, trackColour, trackH * 0.5f);
    list->AddRect(trackMin, trackMax, borderColour, trackH * 0.5f, 0, 1.0f);

    const float knobX = trackMin.x + 10.0f + (*anim) * (trackW - 20.0f);
    list->AddCircleFilled(ImVec2(knobX, trackMin.y + trackH * 0.5f), 7.0f,
                          *value ? c.green : c.textSecondary);

    list->AddText(theme::body(), theme::body()->FontSize,
                  ImVec2(trackMax.x + 10.0f, start.y + (rowH - theme::body()->FontSize) * 0.5f),
                  hovered ? c.text : c.text, label ? label : "");
    ImGui::PopID();

    if (description != nullptr && description[0] != '\0') {
        ImGui::SetCursorScreenPos(ImVec2(start.x + trackW + 10.0f, start.y + rowH));
        ImGui::PushFont(theme::caption());
        ImGui::PushTextWrapPos(start.x + totalW);
        ImGui::TextColored(c.textSecondaryV, "%s", description);
        ImGui::PopTextWrapPos();
        ImGui::PopFont();
    } else {
        ImGui::SetCursorScreenPos(ImVec2(start.x, start.y + rowH));
    }

    return clicked;
}

bool Stepper(const char* id, float* value, float minValue, float maxValue, float step,
             const char* format, float width) {
    if (value == nullptr) {
        return false;
    }

    const theme::Colors& c = theme::colors();
    bool changed = false;

    ImGui::PushID(id);
    const float arrowW = 20.0f;
    const float fieldW = (std::max)(48.0f, width - arrowW - 2.0f);

    const float before = *value;
    ImGui::PushStyleColor(ImGuiCol_FrameBg, c.elevatedV);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, c.elevatedHoverV);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, c.elevatedV);
    ImGui::PushStyleColor(ImGuiCol_Border, c.borderV);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, theme::metrics().controlRadius);
    ImGui::SetNextItemWidth(fieldW);
    ImGui::InputFloat("##value", value, 0.0f, 0.0f, format);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);

    if (*value < minValue) {
        *value = minValue;
    }
    if (*value > maxValue) {
        *value = maxValue;
    }
    if (*value != before) {
        changed = true;
    }

    const ImVec2 fieldMin = ImGui::GetItemRectMin();
    const ImVec2 fieldMax = ImGui::GetItemRectMax();
    const float halfH = (fieldMax.y - fieldMin.y) * 0.5f;
    ImDrawList* list = ImGui::GetWindowDrawList();

    ImGui::SetCursorScreenPos(ImVec2(fieldMax.x + 2.0f, fieldMin.y));
    ImGui::InvisibleButton("##up", ImVec2(arrowW, halfH));
    const bool upHovered = ImGui::IsItemHovered();
    if (ImGui::IsItemClicked()) {
        *value = (std::min)(maxValue, *value + step);
        changed = true;
    }
    list->AddRectFilled(ImVec2(fieldMax.x + 2.0f, fieldMin.y), ImVec2(fieldMax.x + 2.0f + arrowW, fieldMin.y + halfH),
                        upHovered ? c.elevatedHover : c.elevated, 4.0f);
    DrawIcon(list, Icon::ChevronUp,
             ImVec2(fieldMax.x + 2.0f + arrowW * 0.5f, fieldMin.y + halfH * 0.5f), 10.0f,
             upHovered ? c.text : c.textSecondary, 1.5f);

    ImGui::SetCursorScreenPos(ImVec2(fieldMax.x + 2.0f, fieldMin.y + halfH));
    ImGui::InvisibleButton("##down", ImVec2(arrowW, halfH));
    const bool downHovered = ImGui::IsItemHovered();
    if (ImGui::IsItemClicked()) {
        *value = (std::max)(minValue, *value - step);
        changed = true;
    }
    list->AddRectFilled(ImVec2(fieldMax.x + 2.0f, fieldMin.y + halfH),
                        ImVec2(fieldMax.x + 2.0f + arrowW, fieldMax.y),
                        downHovered ? c.elevatedHover : c.elevated, 4.0f);
    DrawIcon(list, Icon::ChevronDown,
             ImVec2(fieldMax.x + 2.0f + arrowW * 0.5f, fieldMin.y + halfH * 1.5f), 10.0f,
             downHovered ? c.text : c.textSecondary, 1.5f);

    ImGui::SetCursorScreenPos(ImVec2(fieldMin.x, fieldMax.y));
    ImGui::Dummy(ImVec2(fieldW + arrowW + 2.0f, 0.0f));
    ImGui::PopID();
    return changed;
}

}  // namespace diskscope::ui

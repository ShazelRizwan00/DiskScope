#include "ui/Icons.h"

namespace diskscope::ui {

namespace {

inline ImVec2 grid(ImVec2 c, float s, float x, float y) {
    return ImVec2(c.x + (x - 12.0f) * s, c.y + (y - 12.0f) * s);
}

}  // namespace

void DrawIcon(ImDrawList* list, Icon icon, ImVec2 centre, float size, ImU32 colour, float stroke) {
    if (!list || size <= 0.0f) {
        return;
    }

    const float s = size / 24.0f;

    switch (icon) {
        case Icon::None: {
            break;
        }

        case Icon::LogoMark: {
            // Filled emerald rounded square with crisp horizontal layer lines
            list->AddRectFilled(grid(centre, s, 2.5f, 2.5f), grid(centre, s, 21.5f, 21.5f), colour, 5.0f * s);
            const ImU32 ink = IM_COL32(10, 26, 18, 255);
            list->AddRectFilled(grid(centre, s, 6.0f, 7.0f), grid(centre, s, 18.0f, 9.5f), ink, 1.2f * s);
            list->AddRectFilled(grid(centre, s, 6.0f, 11.0f), grid(centre, s, 18.0f, 13.5f), ink, 1.2f * s);
            list->AddRectFilled(grid(centre, s, 6.0f, 15.0f), grid(centre, s, 14.0f, 17.5f), ink, 1.2f * s);
            break;
        }

        case Icon::Overview: {
            // 2x2 grid representing analytics dashboard cards
            list->AddRect(grid(centre, s, 3.5f, 3.5f), grid(centre, s, 10.5f, 10.5f), colour, 1.5f * s, 0, stroke);
            list->AddRect(grid(centre, s, 13.5f, 3.5f), grid(centre, s, 20.5f, 10.5f), colour, 1.5f * s, 0, stroke);
            list->AddRect(grid(centre, s, 3.5f, 13.5f), grid(centre, s, 10.5f, 20.5f), colour, 1.5f * s, 0, stroke);
            list->AddRect(grid(centre, s, 13.5f, 13.5f), grid(centre, s, 20.5f, 20.5f), colour, 1.5f * s, 0, stroke);
            break;
        }

        case Icon::Folder: {
            // Classic file folder tab and body outline
            list->PathLineTo(grid(centre, s, 3.0f, 8.0f));
            list->PathLineTo(grid(centre, s, 3.0f, 19.0f));
            list->PathLineTo(grid(centre, s, 21.0f, 19.0f));
            list->PathLineTo(grid(centre, s, 21.0f, 9.0f));
            list->PathLineTo(grid(centre, s, 11.5f, 9.0f));
            list->PathLineTo(grid(centre, s, 9.5f, 6.0f));
            list->PathLineTo(grid(centre, s, 3.0f, 6.0f));
            list->PathStroke(colour, ImDrawFlags_Closed, stroke);
            break;
        }

        case Icon::FolderOpen: {
            // Folder with open front angled flap
            list->PathLineTo(grid(centre, s, 3.0f, 8.0f));
            list->PathLineTo(grid(centre, s, 3.0f, 6.0f));
            list->PathLineTo(grid(centre, s, 9.5f, 6.0f));
            list->PathLineTo(grid(centre, s, 11.5f, 9.0f));
            list->PathLineTo(grid(centre, s, 20.0f, 9.0f));
            list->PathLineTo(grid(centre, s, 20.0f, 12.0f));
            list->PathStroke(colour, 0, stroke);

            list->PathLineTo(grid(centre, s, 2.5f, 19.0f));
            list->PathLineTo(grid(centre, s, 5.5f, 12.0f));
            list->PathLineTo(grid(centre, s, 21.5f, 12.0f));
            list->PathLineTo(grid(centre, s, 18.5f, 19.0f));
            list->PathStroke(colour, ImDrawFlags_Closed, stroke);
            break;
        }

        case Icon::FolderTree: {
            // Folder with hierarchical child branch
            list->AddRect(grid(centre, s, 3.0f, 4.0f), grid(centre, s, 14.0f, 11.0f), colour, 1.5f * s, 0, stroke);
            list->AddLine(grid(centre, s, 6.0f, 11.0f), grid(centre, s, 6.0f, 19.0f), colour, stroke);
            list->AddLine(grid(centre, s, 6.0f, 19.0f), grid(centre, s, 11.0f, 19.0f), colour, stroke);
            list->AddRect(grid(centre, s, 11.0f, 15.0f), grid(centre, s, 20.0f, 21.0f), colour, 1.5f * s, 0, stroke);
            break;
        }

        case Icon::Layers: {
            // Stacked isometric layers
            list->PathLineTo(grid(centre, s, 12.0f, 4.0f));
            list->PathLineTo(grid(centre, s, 21.0f, 8.5f));
            list->PathLineTo(grid(centre, s, 12.0f, 13.0f));
            list->PathLineTo(grid(centre, s, 3.0f, 8.5f));
            list->PathStroke(colour, ImDrawFlags_Closed, stroke);

            list->PathLineTo(grid(centre, s, 3.0f, 12.5f));
            list->PathLineTo(grid(centre, s, 12.0f, 17.0f));
            list->PathLineTo(grid(centre, s, 21.0f, 12.5f));
            list->PathStroke(colour, 0, stroke);

            list->PathLineTo(grid(centre, s, 3.0f, 16.5f));
            list->PathLineTo(grid(centre, s, 12.0f, 21.0f));
            list->PathLineTo(grid(centre, s, 21.0f, 16.5f));
            list->PathStroke(colour, 0, stroke);
            break;
        }

        case Icon::Heap: {
            // Binary max heap tree: root connected to two child nodes
            const float r = 3.0f * s;
            list->AddCircle(grid(centre, s, 12.0f, 5.0f), r, colour, 16, stroke);
            list->AddCircle(grid(centre, s, 6.0f, 18.0f), r, colour, 16, stroke);
            list->AddCircle(grid(centre, s, 18.0f, 18.0f), r, colour, 16, stroke);
            list->AddLine(grid(centre, s, 10.0f, 7.5f), grid(centre, s, 7.5f, 15.5f), colour, stroke);
            list->AddLine(grid(centre, s, 14.0f, 7.5f), grid(centre, s, 16.5f, 15.5f), colour, stroke);
            break;
        }

        case Icon::Settings: {
            // Two sliding controls rather than a toothed gear: at 20 px the radial
            // teeth average out into a sunburst, while sliders stay unmistakable.
            list->AddLine(grid(centre, s, 3.5f, 7.5f), grid(centre, s, 12.4f, 7.5f), colour, stroke);
            list->AddLine(grid(centre, s, 17.6f, 7.5f), grid(centre, s, 20.5f, 7.5f), colour, stroke);
            list->AddCircle(grid(centre, s, 15.0f, 7.5f), 2.6f * s, colour, 12, stroke);

            list->AddLine(grid(centre, s, 3.5f, 16.5f), grid(centre, s, 5.4f, 16.5f), colour, stroke);
            list->AddLine(grid(centre, s, 10.6f, 16.5f), grid(centre, s, 20.5f, 16.5f), colour, stroke);
            list->AddCircle(grid(centre, s, 8.0f, 16.5f), 2.6f * s, colour, 12, stroke);
            break;
        }

        case Icon::Search: {
            // Magnifying glass outline with handle
            list->AddCircle(grid(centre, s, 10.0f, 10.0f), 5.5f * s, colour, 20, stroke);
            list->AddLine(grid(centre, s, 14.0f, 14.0f), grid(centre, s, 20.0f, 20.0f), colour, stroke * 1.25f);
            break;
        }

        case Icon::Close: {
            // Clean symmetrical cross
            list->AddLine(grid(centre, s, 6.5f, 6.5f), grid(centre, s, 17.5f, 17.5f), colour, stroke);
            list->AddLine(grid(centre, s, 17.5f, 6.5f), grid(centre, s, 6.5f, 17.5f), colour, stroke);
            break;
        }

        case Icon::ChevronDown: {
            list->AddLine(grid(centre, s, 6.5f, 9.5f), grid(centre, s, 12.0f, 15.0f), colour, stroke);
            list->AddLine(grid(centre, s, 12.0f, 15.0f), grid(centre, s, 17.5f, 9.5f), colour, stroke);
            break;
        }

        case Icon::ChevronRight: {
            list->AddLine(grid(centre, s, 9.5f, 6.5f), grid(centre, s, 15.0f, 12.0f), colour, stroke);
            list->AddLine(grid(centre, s, 15.0f, 12.0f), grid(centre, s, 9.5f, 17.5f), colour, stroke);
            break;
        }

        case Icon::ChevronLeft: {
            list->AddLine(grid(centre, s, 14.5f, 6.5f), grid(centre, s, 9.0f, 12.0f), colour, stroke);
            list->AddLine(grid(centre, s, 9.0f, 12.0f), grid(centre, s, 14.5f, 17.5f), colour, stroke);
            break;
        }

        case Icon::ChevronUp: {
            list->AddLine(grid(centre, s, 6.5f, 14.5f), grid(centre, s, 12.0f, 9.0f), colour, stroke);
            list->AddLine(grid(centre, s, 12.0f, 9.0f), grid(centre, s, 17.5f, 14.5f), colour, stroke);
            break;
        }

        case Icon::Scan: {
            // Scanner reticle with four corner brackets and horizontal laser line
            list->AddLine(grid(centre, s, 4.0f, 9.0f), grid(centre, s, 4.0f, 4.0f), colour, stroke);
            list->AddLine(grid(centre, s, 4.0f, 4.0f), grid(centre, s, 9.0f, 4.0f), colour, stroke);

            list->AddLine(grid(centre, s, 15.0f, 4.0f), grid(centre, s, 20.0f, 4.0f), colour, stroke);
            list->AddLine(grid(centre, s, 20.0f, 4.0f), grid(centre, s, 20.0f, 9.0f), colour, stroke);

            list->AddLine(grid(centre, s, 4.0f, 15.0f), grid(centre, s, 4.0f, 20.0f), colour, stroke);
            list->AddLine(grid(centre, s, 4.0f, 20.0f), grid(centre, s, 9.0f, 20.0f), colour, stroke);

            list->AddLine(grid(centre, s, 15.0f, 20.0f), grid(centre, s, 20.0f, 20.0f), colour, stroke);
            list->AddLine(grid(centre, s, 20.0f, 20.0f), grid(centre, s, 20.0f, 15.0f), colour, stroke);

            list->AddLine(grid(centre, s, 6.0f, 12.0f), grid(centre, s, 18.0f, 12.0f), colour, stroke);
            break;
        }

        case Icon::Play: {
            list->PathLineTo(grid(centre, s, 8.0f, 5.5f));
            list->PathLineTo(grid(centre, s, 18.0f, 12.0f));
            list->PathLineTo(grid(centre, s, 8.0f, 18.5f));
            list->PathStroke(colour, ImDrawFlags_Closed, stroke);
            break;
        }

        case Icon::Pause: {
            list->AddLine(grid(centre, s, 8.5f, 6.0f), grid(centre, s, 8.5f, 18.0f), colour, stroke * 1.5f);
            list->AddLine(grid(centre, s, 15.5f, 6.0f), grid(centre, s, 15.5f, 18.0f), colour, stroke * 1.5f);
            break;
        }

        case Icon::StepForward: {
            list->PathLineTo(grid(centre, s, 6.5f, 6.0f));
            list->PathLineTo(grid(centre, s, 14.5f, 12.0f));
            list->PathLineTo(grid(centre, s, 6.5f, 18.0f));
            list->PathStroke(colour, ImDrawFlags_Closed, stroke);
            list->AddLine(grid(centre, s, 17.5f, 6.0f), grid(centre, s, 17.5f, 18.0f), colour, stroke * 1.3f);
            break;
        }

        case Icon::Reset: {
            // Circular reload/undo arrow
            list->PathArcTo(grid(centre, s, 12.0f, 12.0f), 7.0f * s, 0.8f, 5.5f, 16);
            list->PathStroke(colour, 0, stroke);
            list->AddLine(grid(centre, s, 11.0f, 5.0f), grid(centre, s, 7.5f, 6.5f), colour, stroke);
            list->AddLine(grid(centre, s, 8.5f, 2.5f), grid(centre, s, 7.5f, 6.5f), colour, stroke);
            break;
        }

        case Icon::Trash: {
            // Trash bin with lid and handle
            list->AddLine(grid(centre, s, 5.0f, 6.0f), grid(centre, s, 19.0f, 6.0f), colour, stroke);
            list->AddLine(grid(centre, s, 10.0f, 4.0f), grid(centre, s, 14.0f, 4.0f), colour, stroke);

            list->PathLineTo(grid(centre, s, 6.5f, 6.5f));
            list->PathLineTo(grid(centre, s, 7.5f, 20.0f));
            list->PathLineTo(grid(centre, s, 16.5f, 20.0f));
            list->PathLineTo(grid(centre, s, 17.5f, 6.5f));
            list->PathStroke(colour, 0, stroke);

            list->AddLine(grid(centre, s, 10.0f, 10.0f), grid(centre, s, 10.0f, 16.5f), colour, stroke);
            list->AddLine(grid(centre, s, 14.0f, 10.0f), grid(centre, s, 14.0f, 16.5f), colour, stroke);
            break;
        }

        case Icon::CheckCircle: {
            list->AddCircle(grid(centre, s, 12.0f, 12.0f), 8.5f * s, colour, 20, stroke);
            list->AddLine(grid(centre, s, 8.0f, 12.0f), grid(centre, s, 11.0f, 15.0f), colour, stroke);
            list->AddLine(grid(centre, s, 11.0f, 15.0f), grid(centre, s, 16.0f, 9.0f), colour, stroke);
            break;
        }

        case Icon::AlertTriangle: {
            list->PathLineTo(grid(centre, s, 12.0f, 3.5f));
            list->PathLineTo(grid(centre, s, 21.0f, 19.5f));
            list->PathLineTo(grid(centre, s, 3.0f, 19.5f));
            list->PathStroke(colour, ImDrawFlags_Closed, stroke);
            list->AddLine(grid(centre, s, 12.0f, 8.5f), grid(centre, s, 12.0f, 14.0f), colour, stroke);
            list->AddCircleFilled(grid(centre, s, 12.0f, 17.0f), stroke * 0.65f, colour);
            break;
        }

        case Icon::XCircle: {
            list->AddCircle(grid(centre, s, 12.0f, 12.0f), 8.5f * s, colour, 20, stroke);
            list->AddLine(grid(centre, s, 8.5f, 8.5f), grid(centre, s, 15.5f, 15.5f), colour, stroke);
            list->AddLine(grid(centre, s, 15.5f, 8.5f), grid(centre, s, 8.5f, 15.5f), colour, stroke);
            break;
        }

        case Icon::Info: {
            list->AddCircle(grid(centre, s, 12.0f, 12.0f), 8.5f * s, colour, 20, stroke);
            list->AddCircleFilled(grid(centre, s, 12.0f, 7.5f), stroke * 0.65f, colour);
            list->AddLine(grid(centre, s, 12.0f, 11.0f), grid(centre, s, 12.0f, 16.5f), colour, stroke);
            break;
        }

        case Icon::Disk: {
            list->AddRect(grid(centre, s, 4.0f, 6.0f), grid(centre, s, 20.0f, 18.0f), colour, 2.0f * s, 0, stroke);
            list->AddLine(grid(centre, s, 4.0f, 13.5f), grid(centre, s, 20.0f, 13.5f), colour, stroke);
            list->AddCircleFilled(grid(centre, s, 16.5f, 16.0f), 1.2f * s, colour);
            break;
        }

        case Icon::File: {
            list->PathLineTo(grid(centre, s, 5.0f, 3.5f));
            list->PathLineTo(grid(centre, s, 14.0f, 3.5f));
            list->PathLineTo(grid(centre, s, 19.0f, 8.5f));
            list->PathLineTo(grid(centre, s, 19.0f, 20.5f));
            list->PathLineTo(grid(centre, s, 5.0f, 20.5f));
            list->PathStroke(colour, ImDrawFlags_Closed, stroke);
            list->AddLine(grid(centre, s, 14.0f, 3.5f), grid(centre, s, 14.0f, 8.5f), colour, stroke);
            list->AddLine(grid(centre, s, 14.0f, 8.5f), grid(centre, s, 19.0f, 8.5f), colour, stroke);
            break;
        }

        case Icon::Chart: {
            list->AddLine(grid(centre, s, 3.0f, 20.0f), grid(centre, s, 21.0f, 20.0f), colour, stroke);
            list->AddLine(grid(centre, s, 6.5f, 20.0f), grid(centre, s, 6.5f, 14.0f), colour, stroke * 1.8f);
            list->AddLine(grid(centre, s, 12.0f, 20.0f), grid(centre, s, 12.0f, 6.0f), colour, stroke * 1.8f);
            list->AddLine(grid(centre, s, 17.5f, 20.0f), grid(centre, s, 17.5f, 10.0f), colour, stroke * 1.8f);
            break;
        }

        case Icon::Expand: {
            list->AddLine(grid(centre, s, 9.0f, 4.0f), grid(centre, s, 4.0f, 4.0f), colour, stroke);
            list->AddLine(grid(centre, s, 4.0f, 4.0f), grid(centre, s, 4.0f, 9.0f), colour, stroke);

            list->AddLine(grid(centre, s, 15.0f, 4.0f), grid(centre, s, 20.0f, 4.0f), colour, stroke);
            list->AddLine(grid(centre, s, 20.0f, 4.0f), grid(centre, s, 20.0f, 9.0f), colour, stroke);

            list->AddLine(grid(centre, s, 4.0f, 15.0f), grid(centre, s, 4.0f, 20.0f), colour, stroke);
            list->AddLine(grid(centre, s, 4.0f, 20.0f), grid(centre, s, 9.0f, 20.0f), colour, stroke);

            list->AddLine(grid(centre, s, 20.0f, 15.0f), grid(centre, s, 20.0f, 20.0f), colour, stroke);
            list->AddLine(grid(centre, s, 20.0f, 20.0f), grid(centre, s, 15.0f, 20.0f), colour, stroke);
            break;
        }

        case Icon::Collapse: {
            list->AddLine(grid(centre, s, 4.0f, 9.0f), grid(centre, s, 9.0f, 9.0f), colour, stroke);
            list->AddLine(grid(centre, s, 9.0f, 9.0f), grid(centre, s, 9.0f, 4.0f), colour, stroke);

            list->AddLine(grid(centre, s, 20.0f, 9.0f), grid(centre, s, 15.0f, 9.0f), colour, stroke);
            list->AddLine(grid(centre, s, 15.0f, 9.0f), grid(centre, s, 15.0f, 4.0f), colour, stroke);

            list->AddLine(grid(centre, s, 4.0f, 15.0f), grid(centre, s, 9.0f, 15.0f), colour, stroke);
            list->AddLine(grid(centre, s, 9.0f, 15.0f), grid(centre, s, 9.0f, 20.0f), colour, stroke);

            list->AddLine(grid(centre, s, 20.0f, 15.0f), grid(centre, s, 15.0f, 15.0f), colour, stroke);
            list->AddLine(grid(centre, s, 15.0f, 15.0f), grid(centre, s, 15.0f, 20.0f), colour, stroke);
            break;
        }

        case Icon::Refresh: {
            list->PathArcTo(grid(centre, s, 12.0f, 12.0f), 7.0f * s, 0.4f, 2.7f, 16);
            list->PathStroke(colour, 0, stroke);
            list->AddLine(grid(centre, s, 7.0f, 18.0f), grid(centre, s, 5.0f, 14.0f), colour, stroke);
            list->AddLine(grid(centre, s, 2.0f, 16.5f), grid(centre, s, 5.0f, 14.0f), colour, stroke);

            list->PathArcTo(grid(centre, s, 12.0f, 12.0f), 7.0f * s, 3.5f, 5.8f, 16);
            list->PathStroke(colour, 0, stroke);
            list->AddLine(grid(centre, s, 17.0f, 6.0f), grid(centre, s, 19.0f, 10.0f), colour, stroke);
            list->AddLine(grid(centre, s, 22.0f, 7.5f), grid(centre, s, 19.0f, 10.0f), colour, stroke);
            break;
        }

        case Icon::Stop: {
            list->AddRect(grid(centre, s, 6.0f, 6.0f), grid(centre, s, 18.0f, 18.0f), colour, 1.5f * s, 0, stroke);
            break;
        }

        case Icon::ArrowRight: {
            list->AddLine(grid(centre, s, 4.0f, 12.0f), grid(centre, s, 18.5f, 12.0f), colour, stroke);
            list->AddLine(grid(centre, s, 13.5f, 7.0f), grid(centre, s, 18.5f, 12.0f), colour, stroke);
            list->AddLine(grid(centre, s, 13.5f, 17.0f), grid(centre, s, 18.5f, 12.0f), colour, stroke);
            break;
        }
    }
}

bool IconButton(const char* id, Icon icon, const char* tooltip, bool active, bool enabled, float buttonSize) {
    const auto& c = theme::colors();

    ImVec2 pos = ImGui::GetCursorScreenPos();
    bool clicked = ImGui::InvisibleButton(id, ImVec2(buttonSize, buttonSize));

    bool hovered = ImGui::IsItemHovered();
    bool held = ImGui::IsItemActive();

    if (!enabled) {
        clicked = false;
    }

    ImDrawList* list = ImGui::GetWindowDrawList();
    ImVec2 min = pos;
    ImVec2 max(pos.x + buttonSize, pos.y + buttonSize);

    ImU32 bgCol = c.panel;
    ImU32 borderCol = c.border;
    ImU32 iconCol = c.textSecondary;

    if (!enabled) {
        bgCol = c.panel;
        borderCol = c.border;
        iconCol = c.textDisabled;
    } else if (active) {
        bgCol = c.greenDark;
        borderCol = c.green;
        iconCol = c.green;
    } else if (held) {
        bgCol = c.elevated;
        borderCol = c.borderStrong;
        iconCol = c.text;
    } else if (hovered) {
        bgCol = c.elevatedHover;
        borderCol = c.borderStrong;
        iconCol = c.text;
    }

    list->AddRectFilled(min, max, bgCol, 6.0f);
    list->AddRect(min, max, borderCol, 6.0f, 0, 1.0f);

    const ImVec2 centre(pos.x + buttonSize * 0.5f, pos.y + buttonSize * 0.5f);
    DrawIcon(list, icon, centre, buttonSize * 0.58f, iconCol, 1.5f);

    if (tooltip && hovered && enabled) {
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
            ImGui::PushStyleColor(ImGuiCol_PopupBg, c.elevatedV);
            ImGui::PushStyleColor(ImGuiCol_Border, c.borderStrongV);
            ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));
            ImGui::BeginTooltip();
            ImGui::PushFont(theme::caption());
            ImGui::TextColored(c.textV, "%s", tooltip);
            ImGui::PopFont();
            ImGui::EndTooltip();
            ImGui::PopStyleVar(3);
            ImGui::PopStyleColor(2);
        }
    }

    return clicked;
}

}  // namespace diskscope::ui

#pragma once

#include <string>

#include <imgui.h>

#include "ui/Icons.h"
#include "ui/Theme.h"

namespace diskscope::ui {

enum class Tone {
    Neutral,
    Green,
    Red,
    Muted
};

enum class ButtonRole {
    Primary,
    Secondary,
    Danger,
    Ghost
};

enum class ButtonSize {
    Small,
    Medium,
    Large
};

struct CardOptions {
    const char* title = nullptr;
    const char* subtitle = nullptr;
    Icon icon = Icon::None;
    float height = 0.0f;
    bool header = true;
    bool padding = true;
    // Shrink-wrap to the content height instead of filling the parent. Settings
    // sections need this; full-height cards are the default for panes.
    bool autoHeight = false;
};

struct ConfirmResult {
    bool confirmed = false;
    bool open = false;
};

// Generic button supporting design-system roles, sizes, icons, disabled & loading states.
bool Button(const char* id,
            const char* label,
            ButtonRole role = ButtonRole::Secondary,
            ButtonSize size = ButtonSize::Medium,
            Icon icon = Icon::None,
            float width = 0.0f,
            bool enabled = true,
            bool loading = false,
            const char* tooltip = nullptr);

// Surface card with panel fill, subtle border, rounded corners and optional header.
// CONTRACT: Returns what ImGui::BeginChild returns. EndCard() MUST always be called
// regardless of the return value, e.g.:
//   if (BeginCard("MyCard", options)) {
//       // draw card body
//   }
//   EndCard();
bool BeginCard(const char* id, const CardOptions& options = {});
void EndCard();
bool BeginCardFill(const char* id, const CardOptions& options = {});

// Standard section header (h2 title, leading icon, caption subtitle, 16px bottom gap).
void SectionHeader(Icon icon, const char* title, const char* subtitle = nullptr);

// Summary metric card with 32x32 icon tile, caption label, large metric value and sub-line.
bool StatCard(const char* id,
              Icon icon,
              const char* label,
              const char* value,
              const char* sub,
              Tone tone = Tone::Neutral);

bool StatCard(const char* id,
              Icon icon,
              const char* label,
              const char* value,
              const char* sub,
              Tone tone,
              ImVec2 size);

// Single-line text input with leading search icon and in-field clear button.
bool SearchInput(const char* id, std::string& value, const char* hint, float width = 0.0f);

// Connected segmented switch control (e.g. Top 5 / 10 / 20). `width` of 0 fills
// the available row; a positive width keeps the control compact.
bool Segmented(const char* id, const char* const* labels, int count, int& index, float width = 0.0f);

// Linear progress bar (determinate when fraction >= 0, animated indeterminate when fraction < 0).
void ProgressBar(float fraction, Tone tone = Tone::Green, float height = 8.0f, const char* overlay = nullptr);

// Thin share/size meter clamped to [0, 1].
void BarMeter(float fraction, float width, Tone tone = Tone::Green, float height = 4.0f);

// Toggle switch with an optional caption underneath. Returns true when toggled.
bool Switch(const char* label, bool* value, const char* description = nullptr);

// Numeric stepper: value field with increment/decrement chevrons. Returns true
// when the value changed. The value is clamped to [minValue, maxValue].
bool Stepper(const char* id, float* value, float minValue, float maxValue, float step,
             const char* format = "%.0f", float width = 92.0f);

// Contextual tooltip displayed after 250ms delay on the previous item.
void Tooltip(const char* text);

// 1px hairline divider (horizontal or vertical).
void Divider(bool vertical = false);

// Key-value pair aligned in two columns.
void KeyValueRow(const char* label, const char* value, float labelWidth = 150.0f);

// Centred state block with tone-tinted icon, title and wrapped caption.
void EmptyState(Icon icon, const char* title, const char* body);

// Centred modal confirm dialog with primary/danger confirm and secondary cancel.
ConfirmResult ConfirmDialog(const char* id,
                           bool* open,
                           const char* title,
                           const char* body,
                           const char* confirmLabel,
                           const char* cancelLabel = "Cancel",
                           bool danger = true);

// Thin wrapper over ImGui tables applying the design system.
bool BeginTable(const char* id, int columns, ImGuiTableFlags extraFlags = 0, ImVec2 size = ImVec2(0.0f, 0.0f));
void EndTable();

// Draws `text` right-aligned at `rightX` on the current line without moving the
// cursor. Column widths are measured rather than assumed, because the mono font
// is wider than the body font at the same nominal size.
void RightAlignedText(float rightX, ImFont* font, ImU32 colour, const char* text);

// Right-aligned table header cell: the label in the secondary colour, advancing
// the row cursor by the text height the way a left-aligned header label does.
void TableHeaderCellRight(const char* text);

// Sidebar navigation item with active indicator bar, icon and hover transitions.
bool NavItem(const char* id, Icon icon, const char* label, bool active);

}  // namespace diskscope::ui

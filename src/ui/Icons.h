#pragma once

#include <imgui.h>
#include "ui/Theme.h"

namespace diskscope::ui {

enum class Icon {
    None,
    LogoMark,
    Overview,
    Folder,
    FolderOpen,
    FolderTree,
    Layers,
    SiteMap,
    Heap,
    Settings,
    Search,
    Close,
    ChevronDown,
    ChevronRight,
    ChevronLeft,
    ChevronUp,
    Scan,
    Play,
    Pause,
    StepForward,
    Reset,
    Trash,
    Check,
    CheckCircle,
    AlertTriangle,
    XCircle,
    Info,
    Disk,
    File,
    Chart,
    ListNumbered,
    Expand,
    Collapse,
    Refresh,
    Stop,
    Dot,
    ArrowRight
};

void DrawIcon(ImDrawList* list, Icon icon, ImVec2 centre, float size, ImU32 colour, float stroke = 1.6f);
void IconText(Icon icon, const char* text, ImU32 colour, float size = theme::metrics().iconSize);
ImVec2 IconMeasure(Icon icon, float size);
bool IconButton(const char* id, Icon icon, const char* tooltip = nullptr, bool active = false, bool enabled = true, float buttonSize = 28.0f);

}  // namespace diskscope::ui

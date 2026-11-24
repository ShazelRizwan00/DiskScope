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
    CheckCircle,
    AlertTriangle,
    XCircle,
    Info,
    Disk,
    File,
    Chart,
    Expand,
    Collapse,
    Refresh,
    Stop,
    ArrowRight
};

void DrawIcon(ImDrawList* list, Icon icon, ImVec2 centre, float size, ImU32 colour, float stroke = 1.6f);
bool IconButton(const char* id, Icon icon, const char* tooltip = nullptr, bool active = false, bool enabled = true, float buttonSize = 28.0f);

}  // namespace diskscope::ui

#pragma once

#include "ui/AppState.h"

namespace diskscope {

class SettingsView {
public:
    SettingsView() = default;
    void draw(AppState& state);

private:
    int selectedTopKIndex_ = 1;
};


}  // namespace diskscope


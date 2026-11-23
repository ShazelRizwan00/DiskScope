#pragma once

namespace diskscope::viz {

// ImPlot axis formatter for axes measured in megabytes: prints the tick as a
// compact byte size ("512M", "1.2G"), so charts never show raw bytes.
int byteAxisTickMb(double valueMegabytes, char* buffer, int size, void* userData);

// Applies DiskScope's palette to the ImPlot style singleton. Idempotent, so it
// is safe to call whenever a chart is drawn.
void applyPlotStyle();

}  // namespace diskscope::viz

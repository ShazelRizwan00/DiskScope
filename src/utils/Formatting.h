#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace diskscope::format {

// Bytes are the single source of truth everywhere else; these helpers are the
// only place where they turn into human text.
//
// byteSize(0)            -> "0 B"
// byteSize(1536)         -> "1.50 KB"
// byteSize(1234567890)   -> "1.15 GB"
std::string byteSize(std::uintmax_t bytes);

// Short form for chart axes and tight table cells: "1.2G", "850M", "12K".
std::string byteSizeCompact(std::uintmax_t bytes);

// Thousands separators: 1234567 -> "1,234,567".
std::string count(std::uintmax_t value);

// 0.2561 -> "25.6%"
std::string percent(double fraction, int decimals = 1);

// "C:/Users/Shazel/Documents" shown relative to the scan root when possible.
std::string relativePath(const std::filesystem::path& path,
                         const std::filesystem::path& base);

// Clips long text with an ellipsis so tables keep their column widths.
std::string ellipsize(const std::string& text, std::size_t maxChars);

// Lower-case copy used by the case-insensitive folder search.
std::string toLower(const std::string& text);

}  // namespace diskscope::format

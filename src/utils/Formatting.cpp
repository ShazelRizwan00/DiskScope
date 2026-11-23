#include "utils/Formatting.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string>

namespace diskscope::format {
namespace {

constexpr double kKilo = 1024.0;

std::string formatBytes(std::uintmax_t bytes, bool withSpace, int decimals, bool shortUnit) {
    static constexpr std::array<const char*, 6> kUnits{"B", "KB", "MB", "GB", "TB", "PB"};

    if (bytes < 1024) {
        return std::to_string(bytes) + (withSpace ? " " : "") + kUnits[0];
    }

    double value = static_cast<double>(bytes);
    std::size_t unit = 0;
    while (value >= kKilo && unit + 1 < kUnits.size()) {
        value /= kKilo;
        ++unit;
    }

    std::array<char, 32> buffer{};
    if (decimals > 0) {
        std::snprintf(buffer.data(), buffer.size(), "%.*f", decimals, value);
    } else {
        std::snprintf(buffer.data(), buffer.size(), "%.0f", value);
    }

    std::string unitText = kUnits[unit];
    if (shortUnit && unitText != "B") {
        unitText.pop_back(); // "KB" -> "K", "GB" -> "G"
    }

    std::string text(buffer.data());
    // Drop a meaningless ".0" so compact values read "512K" / "2G" instead of
    // "512.0K" / "2.0G". Two-decimal byteSize() output is left untouched.
    if (shortUnit && text.size() >= 2 && text.compare(text.size() - 2, 2, ".0") == 0) {
        text.erase(text.size() - 2);
    }
    if (withSpace) {
        text += ' ';
    }
    text += unitText;
    return text;
}

}  // namespace

std::string byteSize(std::uintmax_t bytes) {
    return formatBytes(bytes, true, 2, false);
}

std::string byteSizeCompact(std::uintmax_t bytes) {
    // Chart axes and tight cells: one decimal, no space, short unit ("1.2G").
    return formatBytes(bytes, false, bytes < 1024 ? 0 : 1, true);
}

std::string count(std::uintmax_t value) {
    const std::string digits = std::to_string(value);
    const int length = static_cast<int>(digits.size());
    std::string out;
    out.reserve(digits.size() + digits.size() / 3);

    for (int i = 0; i < length; ++i) {
        if (i > 0 && (length - i) % 3 == 0) {
            out += ',';
        }
        out += digits[static_cast<std::size_t>(i)];
    }
    return out;
}

std::string percent(double fraction, int decimals) {
    const int clamped = std::clamp(decimals, 0, 4);
    double value = fraction * 100.0;
    if (value != 0.0 && std::abs(value) < 0.05) {
        value = 0.0; // avoid "-0.0%" and "0.0%" for negligible fractions
    }
    std::array<char, 32> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "%.*f%%", clamped, value);
    return std::string(buffer.data());
}

std::string relativePath(const std::filesystem::path& path, const std::filesystem::path& base) {
    if (base.empty()) {
        return path.generic_u8string();
    }

    std::error_code ec;
    const std::filesystem::path rel = std::filesystem::relative(path, base, ec);
    if (ec || rel.empty()) {
        return path.generic_u8string();
    }
    // ".." means the node is outside the scan root; show the absolute path then.
    const std::string text = rel.generic_u8string();
    if (text.rfind("..", 0) == 0) {
        return path.generic_u8string();
    }
    return text;
}

std::string ellipsize(const std::string& text, std::size_t maxChars) {
    if (text.size() <= maxChars) {
        return text;
    }
    if (maxChars <= 3) {
        return text.substr(0, maxChars);
    }
    // Byte-oriented on purpose: names come from the filesystem as UTF-8 and this
    // is only used to keep table cells from growing, not to split words.
    return text.substr(0, maxChars - 3) + "...";
}

std::string toLower(const std::string& text) {
    std::string out = text;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
    });
    return out;
}

}  // namespace diskscope::format

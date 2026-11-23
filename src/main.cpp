#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

#include "ui/Application.h"

namespace {

namespace fs = std::filesystem;

void printUsage() {
    std::printf(
        "DiskScope - disk space analyzer with a hand-written max binary heap\n"
        "\n"
        "Usage: DiskScope [options]\n"
        "  --root <path>       directory to scan on start\n"
        "  --smoke             run the automated smoke test (headless) and exit\n"
        "  --capture <dir>     write one BMP per view into <dir>\n"
        "  --frames <n>        frames rendered per view before capture (default 2)\n"
        "  --help              show this message\n");
}

// Deterministic fixture for smoke runs: nested folders, an empty folder, a
// zero-byte file and two sibling folders of exactly equal size (which exercises
// the heap's deterministic tie-break).
bool buildSmokeFixture(const fs::path& root) {
    std::error_code ec;
    fs::remove_all(root, ec);
    fs::create_directories(root, ec);

    auto writeFile = [](const fs::path& path, std::size_t bytes) {
        std::error_code fileEc;
        fs::create_directories(path.parent_path(), fileEc);
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        const std::string payload(bytes, 'x');
        out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
        return out.good();
    };

    bool ok = writeFile(root / "documents" / "thesis.pdf", 24000);
    ok = ok && writeFile(root / "documents" / "notes.txt", 1200);
    ok = ok && writeFile(root / "projects" / "diskscope" / "main.cpp", 8000);
    ok = ok && writeFile(root / "projects" / "diskscope" / "build" / "app.exe", 96000);
    ok = ok && writeFile(root / "projects" / "archive" / "old.zip", 48000);
    ok = ok && writeFile(root / "media" / "videos" / "trip.mp4", 128000);
    ok = ok && writeFile(root / "media" / "photos" / "a.jpg", 30000);
    ok = ok && writeFile(root / "media" / "photos" / "b.jpg", 30000); // equal sizes
    ok = ok && writeFile(root / "downloads" / "installer.msi", 64000);
    ok = ok && writeFile(root / "downloads" / "empty.txt", 0);       // zero-byte file
    fs::create_directories(root / "empty_folder", ec);               // empty directory
    ok = ok && writeFile(root / "readme.md", 150);
    return ok;
}

}  // namespace

int main(int argc, char** argv) {
    diskscope::AppOptions options;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i] != nullptr ? argv[i] : "";
        auto next = [&](const char* what) -> const char* {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "DiskScope: %s expects a value\n", what);
                return nullptr;
            }
            return argv[++i];
        };

        if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        }
        if (arg == "--smoke") {
            options.smoke = true;
            continue;
        }
        if (arg == "--root") {
            const char* value = next("--root");
            if (value == nullptr) {
                return 2;
            }
            options.root = value;
            continue;
        }
        if (arg == "--capture") {
            const char* value = next("--capture");
            if (value == nullptr) {
                return 2;
            }
            options.capture = true;
            options.captureDir = value;
            continue;
        }
        if (arg == "--frames") {
            const char* value = next("--frames");
            if (value == nullptr) {
                return 2;
            }
            options.frames = std::max(1, std::atoi(value));
            continue;
        }

        std::fprintf(stderr, "DiskScope: unknown option '%s'\n", arg.c_str());
        printUsage();
        return 2;
    }

    // A smoke run with no explicit root scans the generated fixture, so the
    // result is reproducible on any machine.
    if (options.smoke && options.root.empty()) {
        std::error_code ec;
        const fs::path fixture = fs::temp_directory_path(ec) / "diskscope_smoke_fixture";
        if (!buildSmokeFixture(fixture)) {
            std::fprintf(stderr, "DiskScope: could not create the smoke fixture\n");
            return 1;
        }
        options.root = fixture;
    }

    diskscope::Application app(options);
    return app.run();
}

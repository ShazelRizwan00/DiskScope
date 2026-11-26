
//Shazel (B)
#pragma once

#include <atomic>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include "core/FileSystemScanner.h"

namespace diskscope {

// Runs the filesystem scan on a worker thread so the UI never blocks.
//
// Threading contract:
//  * the worker owns the tree it is building; nothing else may touch it;
//  * progress is published as a copy under a mutex (cheap, coarse updates);
//  * exactly one finished ScanResult is handed to the UI thread through
//    takeResult(), which moves it out and leaves the slot empty;
//  * requestCancel() is safe to call at any time from the UI thread.
class ScanController {
public:
    ScanController() = default;
    ~ScanController();

    ScanController(const ScanController&) = delete;
    ScanController& operator=(const ScanController&) = delete;

    // Starts a scan. Returns false when a scan is already running or the path
    // is not an existing directory.
    bool start(const std::filesystem::path& root, const ScanOptions& options = {});

    // Cooperative cancellation; the worker stops at the next checkpoint and
    // still returns a partial tree marked `cancelled`.
    void requestCancel();

    bool running() const { return running_.load(std::memory_order_acquire); }
    ScanProgress progress() const;

    // Non-blocking: returns the finished result exactly once.
    std::optional<ScanResult> takeResult();

    // Empty when the last start()/scan succeeded.
    std::string lastError() const;

private:
    void run();

    mutable std::mutex mutex_;
    ScanProgress progress_;
    std::optional<ScanResult> pending_;
    std::string error_;
    std::filesystem::path root_;
    ScanOptions options_;

    std::thread worker_;
    std::atomic<bool> cancelRequested_{false};
    std::atomic<bool> running_{false};
};

}  // namespace diskscope

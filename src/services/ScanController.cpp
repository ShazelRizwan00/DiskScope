#include "services/ScanController.h"

#include <exception>
#include <system_error>
#include <utility>

namespace diskscope {

ScanController::~ScanController() {
    requestCancel();
    if (worker_.joinable()) {
        worker_.join();
    }
}

bool ScanController::start(const std::filesystem::path& root, const ScanOptions& options) {
    if (running_.load(std::memory_order_acquire)) {
        return false;
    }

    // A finished worker is still joinable; assigning to a joinable std::thread
    // terminates the process, so reclaim it first.
    if (worker_.joinable()) {
        worker_.join();
    }

    std::error_code ec;
    if (!std::filesystem::is_directory(root, ec)) {
        std::lock_guard<std::mutex> lock(mutex_);
        error_ = ec ? ec.message() : "selected path is not a directory";
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        root_ = root;
        options_ = options;
        error_.clear();
        progress_ = ScanProgress{};
        pending_.reset();
    }

    cancelRequested_.store(false, std::memory_order_release);
    running_.store(true, std::memory_order_release);
    worker_ = std::thread(&ScanController::run, this);
    return true;
}

void ScanController::run() {
    try {
        // Progress snapshots are copied under the mutex; the scan itself works on
        // its own tree and never touches shared state.
        auto publish = [this](const ScanProgress& snapshot) {
            std::lock_guard<std::mutex> lock(mutex_);
            progress_ = snapshot;
        };
        auto cancelled = [this]() { return cancelRequested_.load(std::memory_order_relaxed); };

        ScanResult result = scanDirectory(root_, options_, publish, cancelled);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            pending_ = std::move(result);
            error_.clear();
        }
    } catch (const std::exception& ex) {
        std::lock_guard<std::mutex> lock(mutex_);
        error_ = ex.what();
    } catch (...) {
        std::lock_guard<std::mutex> lock(mutex_);
        error_ = "scan failed with an unknown error";
    }

    // Release store after the result is visible: the UI can rely on running()
    // being false only once takeResult() has something to hand over.
    running_.store(false, std::memory_order_release);
}

void ScanController::requestCancel() {
    cancelRequested_.store(true, std::memory_order_release);
}

ScanProgress ScanController::progress() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return progress_;
}

std::optional<ScanResult> ScanController::takeResult() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!pending_.has_value()) {
        return std::nullopt;
    }
    std::optional<ScanResult> result = std::move(pending_);
    pending_.reset();
    return result;
}

std::string ScanController::lastError() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return error_;
}

}  // namespace diskscope

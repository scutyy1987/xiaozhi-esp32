#include "opencat/bt_command_gateway.h"

#include <chrono>
#include <utility>

#include <esp_log.h>

namespace {

const char* const kTag = "BtCmdGateway";

}  // namespace

struct BtCommandGateway::QueueItem {
    std::string payload;
    std::chrono::steady_clock::time_point deadline;
    bool has_deadline = false;
};

BtCommandGateway::BtCommandGateway() = default;

BtCommandGateway::~BtCommandGateway() {
    Stop();
}

bool BtCommandGateway::Start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_) {
        return true;
    }

    running_ = true;
    worker_ = std::thread(&BtCommandGateway::WorkerLoop, this);
    ESP_LOGI(kTag, "worker started");
    return true;
}

void BtCommandGateway::Stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_) {
            return;
        }
        running_ = false;
    }

    cv_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
    ESP_LOGI(kTag, "worker stopped");
}

bool BtCommandGateway::Enqueue(std::string payload, uint32_t timeout_ms) {
    QueueItem item;
    item.payload = std::move(payload);
    if (timeout_ms > 0) {
        item.has_deadline = true;
        item.deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push_back(std::move(item));
        ++stats_.enqueued;
    }

    cv_.notify_one();
    return true;
}

void BtCommandGateway::SetSender(std::function<bool(std::string_view)> sender) {
    std::lock_guard<std::mutex> lock(mutex_);
    sender_ = std::move(sender);
}

size_t BtCommandGateway::QueueSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

BtCommandGateway::Stats BtCommandGateway::GetStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

void BtCommandGateway::WorkerLoop() {
    while (true) {
        QueueItem item;
        std::function<bool(std::string_view)> sender;

        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() { return !running_ || !queue_.empty(); });

            if (!running_ && queue_.empty()) {
                return;
            }

            item = std::move(queue_.front());
            queue_.pop_front();

            if (item.has_deadline && std::chrono::steady_clock::now() > item.deadline) {
                ++stats_.expired;
                ESP_LOGW(kTag, "drop expired payload");
                continue;
            }

            sender = sender_;
            if (!sender) {
                ++stats_.dropped;
                ESP_LOGW(kTag, "sender not set, payload dropped");
                continue;
            }
        }

        if (sender(item.payload)) {
            std::lock_guard<std::mutex> lock(mutex_);
            ++stats_.sent;
        } else {
            std::lock_guard<std::mutex> lock(mutex_);
            ++stats_.dropped;
            ESP_LOGW(kTag, "sender failed, payload dropped");
        }
    }
}

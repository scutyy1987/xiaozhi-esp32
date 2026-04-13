#ifndef OPEN_CAT_BT_COMMAND_GATEWAY_H_
#define OPEN_CAT_BT_COMMAND_GATEWAY_H_

#include <cstddef>
#include <cstdint>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

class BtCommandGateway {
public:
    struct Stats {
        size_t enqueued = 0;
        size_t sent = 0;
        size_t dropped = 0;
        size_t expired = 0;
    };

    BtCommandGateway();
    ~BtCommandGateway();

    bool Start();
    void Stop();

    bool Enqueue(std::string payload, uint32_t timeout_ms = 0);
    void SetSender(std::function<bool(std::string_view)> sender);

    size_t QueueSize() const;
    Stats GetStats() const;

private:
    struct QueueItem;
    void WorkerLoop();

    mutable std::mutex mutex_;
    std::deque<QueueItem> queue_;
    std::condition_variable cv_;
    std::thread worker_;
    bool running_ = false;

    std::function<bool(std::string_view)> sender_;
    Stats stats_;
};

#endif  // OPEN_CAT_BT_COMMAND_GATEWAY_H_

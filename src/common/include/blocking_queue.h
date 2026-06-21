#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
class BlockingQueue {
 public:
  void Push(const T& value) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      queue_.push(value);
    }
    cond_.notify_one();
  }

  bool Pop(T* value, int timeout_ms) {
    std::unique_lock<std::mutex> lock(mutex_);

    bool ok = cond_.wait_for(
        lock,
        std::chrono::milliseconds(timeout_ms),
        [this]() { return !queue_.empty(); });

    if (!ok) {
      return false;
    }

    *value = queue_.front();
    queue_.pop();
    return true;
  }

 private:
  std::mutex mutex_;
  std::condition_variable cond_;
  std::queue<T> queue_;
};
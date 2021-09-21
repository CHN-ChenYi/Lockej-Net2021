#pragma once

#include <mutex>
#include <queue>

template <typename T>
class ThreadSafeQueue {
 public:
  bool push(T const &value) {
    std::lock_guard<std::mutex> lock(mutex);
    q.push(value);
    return true;
  }
  bool pop(T &value) {
    std::lock_guard<std::mutex> lock(mutex);
    if (q.empty()) return false;
    value = q.front();
    q.pop();
    return true;
  }
  bool empty() const {
    std::lock_guard<std::mutex> lock(mutex);
    return q.empty();
  }

 private:
  std::queue<T> q;
  mutable std::mutex mutex;
};

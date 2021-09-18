#pragma once

#include <netinet/in.h>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "Queue.hpp"

using socket_fd = int;

class Pool {
 public:
  Pool(const std::string &hostname);
  ~Pool();
  void AddClient(const sockaddr_in &addr, const socket_fd &fd);

 private:
  std::string hostname_;
  size_t hostname_len_;

  std::atomic<bool> is_exit_;
  std::mutex clients_mutex_;
  std::map<socket_fd, sockaddr_in> clients_;
  std::map<socket_fd, std::unique_ptr<std::thread>> threads_;

  struct MessageInfo {
    socket_fd src;
    std::string content;
  };
  std::map<socket_fd, ThreadSafeQueue<MessageInfo>> msg_queues_;
};

#include "Pool.hpp"

#include <sys/ioctl.h>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

#include "../MsgDef.hpp"
#include "Queue.hpp"

static std::string hostname;

std::atomic<bool> is_exit;
std::mutex clients_mutex;
std::map<socket_fd, sockaddr_in> clients;
std::map<socket_fd, std::unique_ptr<std::thread>> threads;
std::map<socket_fd, ThreadSafeQueue<std::string>> msg_queues;

void Init(const std::string &hostname) { ::hostname = hostname; }

void AddClient(const sockaddr_in &addr, const socket_fd &fd) {
  if (is_exit) return;
  {
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients[fd] = addr;
  }
  auto f = [&, addr, fd]() {
    for (;;) {
      if (is_exit) {
        unsigned msg = kDisconnect;
        if (send(fd, static_cast<void *>(&msg), sizeof(msg), 0) == -1) {
          // TODO
        }
        break;
      }
      if (!msg_queues[fd].empty()) {
        // TODO
      }
      int count;
      ioctl(fd, FIONREAD, &count);
      if (count) {
        // TODO
      }
    }
  };
  threads[fd] =
      std::make_unique<decltype(std::thread(f))>(std::thread(std::move(f)));
}

void Exit() {
  is_exit = true;
  for (const auto &thread : threads) {
    if (thread.second->joinable()) thread.second->join();
  }
}

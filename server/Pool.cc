#include "Pool.hpp"

#include <sys/ioctl.h>

#include <atomic>
#include <ctime>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

#include "../MsgDef.hpp"
#include "Queue.hpp"

static std::string hostname;
static size_t hostname_len;

static std::atomic<bool> is_exit;
static std::mutex clients_mutex;
static std::map<socket_fd, sockaddr_in> clients;
static std::map<socket_fd, std::unique_ptr<std::thread>> threads;
static std::map<socket_fd, ThreadSafeQueue<std::string>> msg_queues;

template <typename... Args>
void SendMessage(Args &&...args) {
  if (~send(std::forward<Args>(args)...)) {
    // TODO
  }
}

template <typename... Args>
void RecvMessage(Args &&...args) {
  if (~recv(std::forward<Args>(args)...) == -1) {
    // TODO
  }
}

void Init(const std::string &hostname) {
  ::hostname = hostname;
  hostname_len = ::hostname.length();
}

void AddClient(const sockaddr_in &addr, const socket_fd &fd) {
  if (is_exit) return;
  {
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients[fd] = addr;
  }
  auto f = [&, addr, fd]() {
    unsigned msg_type;
    for (;;) {
      // exit
      if (is_exit) {
        msg_type = static_cast<unsigned>(MsgType::kDisconnect);
        SendMessage(fd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type),
                    0);
        break;
      }
      // forward message
      if (!msg_queues[fd].empty()) {
        msg_type = static_cast<unsigned>(MsgType::kMsg);
        SendMessage(fd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type),
                    0);
        std::string msg;
        msg_queues[fd].pop(msg);
        const size_t msg_len = msg.length();
        SendMessage(fd, reinterpret_cast<const void *>(&msg_len),
                    sizeof(msg_len), 0);
        SendMessage(fd, reinterpret_cast<const void *>(msg.data()),
                    sizeof(char) * msg_len, 0);
      }
      // handle request
      int count;
      ioctl(fd, FIONREAD, &count);
      if (count) {
        RecvMessage(fd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type),
                    0);
        const MsgType msg_type_ = static_cast<MsgType>(msg_type);
        if (msg_type_ == MsgType::kDisconnect) {
          break;
        } else if (msg_type_ == MsgType::kTime) {
          std::time_t cur_time = time(nullptr);
          SendMessage(fd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type),
                      0);
          SendMessage(fd, reinterpret_cast<void *>(&cur_time), sizeof(cur_time),
                      0);
        } else if (msg_type_ == MsgType::kHostname) {
          SendMessage(fd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type),
                      0);
          SendMessage(fd, reinterpret_cast<void *>(&hostname_len),
                      sizeof(hostname_len), 0);
          SendMessage(fd, reinterpret_cast<const void *>(hostname.data()),
                      sizeof(char) * hostname_len, 0);
        } else if (msg_type_ == MsgType::kList) {
          std::lock_guard<std::mutex> lock(clients_mutex);
          const size_t list_len = clients.size();
          SendMessage(fd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type),
                      0);
          SendMessage(fd, reinterpret_cast<const void *>(&list_len),
                      sizeof(list_len), 0);
          for (const auto &client : clients) {
            SendMessage(fd, reinterpret_cast<const void *>(&client.first),
                        sizeof(client.first), 0);
            SendMessage(fd, reinterpret_cast<const void *>(&client.second),
                        sizeof(client.second), 0);
          }
        } else if (msg_type_ == MsgType::kMsg) {
          int dst;
          size_t msg_len;
          std::string msg;
          RecvMessage(fd, reinterpret_cast<void *>(&dst), sizeof(dst), 0);
          RecvMessage(fd, reinterpret_cast<void *>(&msg_len), sizeof(msg_len),
                      0);
          msg.resize(msg_len);
          RecvMessage(fd, reinterpret_cast<void *>(&msg.data()[0]),
                      sizeof(char) * msg_len, 0);
          if (msg_queues.count(dst)) {
            msg_queues[dst].push(msg);
            msg_type = static_cast<unsigned>(MsgType::kSuccess);
            SendMessage(fd, reinterpret_cast<void *>(&msg_type),
                        sizeof(msg_type), 0);
          } else {
            msg_type = static_cast<unsigned>(MsgType::kError);
            SendMessage(fd, reinterpret_cast<void *>(&msg_type),
                        sizeof(msg_type), 0);
          }
        } else {  // TODO
        }
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

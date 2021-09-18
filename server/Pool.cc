#include "Pool.hpp"

#include <arpa/inet.h>
#include <sys/ioctl.h>

#include <ctime>
#include <iostream>

#include "../MsgDef.hpp"

extern int errno;

template <typename... Args>
void SendMessage(Args &&...args) {
  if (!~send(std::forward<Args>(args)...)) {
    std::cout << "send() failed! errno: " << errno << std::endl;
  }
}

template <typename... Args>
void RecvMessage(Args &&...args) {
  if (!~recv(std::forward<Args>(args)...)) {
    std::cout << "recv() failed! errno: " << errno << std::endl;
  }
}

Pool::Pool(const std::string &hostname) {
  hostname_ = hostname;
  hostname_len_ = hostname_.length();
}

Pool::~Pool() {
  is_exit_ = true;
  for (const auto &thread : threads_) {
    if (thread.second->joinable()) thread.second->join();
  }
}

void Pool::AddClient(const sockaddr_in &addr, const socket_fd &fd) {
  if (is_exit_) return;
  {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_[fd] = addr;
  }
  auto f = [&, addr, fd]() {
    unsigned msg_type;
    for (;;) {
      // exit
      if (is_exit_) {
        msg_type = static_cast<unsigned>(MsgType::kDisconnect);
        SendMessage(fd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type),
                    0);
        break;
      }
      // forward message
      if (!msg_queues_[fd].empty()) {
        msg_type = static_cast<unsigned>(MsgType::kMsg);
        MessageInfo msg;
        msg_queues_[fd].pop(msg);
        const size_t msg_len = msg.content.length();
        SendMessage(fd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type),
                    0);
        SendMessage(fd, reinterpret_cast<void *>(&msg.src), sizeof(msg.src), 0);
        SendMessage(fd, reinterpret_cast<const void *>(&msg_len),
                    sizeof(msg_len), 0);
        SendMessage(fd, reinterpret_cast<void *>(msg.content.data()),
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
          char address[INET_ADDRSTRLEN];
          inet_ntop(AF_INET, &addr.sin_addr, address, sizeof(address));
          std::cout << address << ':' << addr.sin_port << " disconnected."
                    << std::endl;
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
          SendMessage(fd, reinterpret_cast<void *>(&hostname_len_),
                      sizeof(hostname_len_), 0);
          SendMessage(fd, reinterpret_cast<void *>(hostname_.data()),
                      sizeof(char) * hostname_len_, 0);
        } else if (msg_type_ == MsgType::kList) {
          std::lock_guard<std::mutex> lock(clients_mutex_);
          const size_t list_len = clients_.size();
          SendMessage(fd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type),
                      0);
          SendMessage(fd, reinterpret_cast<const void *>(&list_len),
                      sizeof(list_len), 0);
          for (const auto &client : clients_) {
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
          RecvMessage(fd, reinterpret_cast<void *>(msg.data()),
                      sizeof(char) * msg_len, 0);
          if (msg_queues_.count(dst)) {
            msg_queues_[dst].push(MessageInfo{fd, msg});
            msg_type = static_cast<unsigned>(MsgType::kSuccess);
            SendMessage(fd, reinterpret_cast<void *>(&msg_type),
                        sizeof(msg_type), 0);
          } else {
            msg_type = static_cast<unsigned>(MsgType::kError);
            SendMessage(fd, reinterpret_cast<void *>(&msg_type),
                        sizeof(msg_type), 0);
          }
        } else {
          std::cout << "unknown message type: " << msg_type << std::endl;
        }
      }
    }
  };
  threads_[fd] =
      std::make_unique<decltype(std::thread(f))>(std::thread(std::move(f)));
}

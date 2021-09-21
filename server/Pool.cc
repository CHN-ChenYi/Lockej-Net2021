#include "Pool.hpp"

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <ctime>
#include <future>
#include <iostream>

#include "../MsgDef.hpp"

extern int errno;

template <typename... Args>
void SendMessage(Args &&...args)
{
  if (!~send(std::forward<Args>(args)...))
  {
    std::cout << "send() failed! errno: " << errno << std::endl;
  }
}

template <typename... Args>
void RecvMessage(Args &&...args)
{
  if (!~recv(std::forward<Args>(args)...))
  {
    std::cout << "recv() failed! errno: " << errno << std::endl;
  }
}

Pool::Pool(const std::string &hostname)
{
  is_exit_ = false;
  hostname_ = hostname;
  hostname_len_ = hostname_.length();
}

Pool::~Pool()
{
  is_exit_ = true;
  for (const auto &thread : threads_)
  {
    if (thread.second->joinable())
      thread.second->join();
  }
}

void Pool::AddClient(const sockaddr_in &addr, const socket_fd &fd)
{
  if (is_exit_)
    return;
  {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_[fd] = addr;
  }
  auto f = [this, addr, fd]()
  {
    std::mutex mutex;
    std::atomic<bool> recv_heartbeat = false;
    std::future<void> alive = std::async(
        std::launch::async,
        [fd](std::mutex *const mutex, std::atomic<bool> *recv_heartbeat)
        {
          int heartbeat_counter = 0;
          unsigned msg_type = static_cast<unsigned>(MsgType::kHeartBeat);
          for (;;)
          {
            std::this_thread::sleep_for(kHeartBeatInterval);
            // { // TODO: uncomment this block
            //   std::lock_guard<std::mutex> lock(*mutex);
            //   if (!~send(fd, reinterpret_cast<void *>(&msg_type),
            //              sizeof(msg_type), 0)) {
            //     if (errno == 104 || errno == 9) return;  // peer closed || closed by server
            //     std::cout << "send() heartbeat failed! errno: " << errno
            //               << std::endl;
            //   }
            // }
            if (*recv_heartbeat)
            {
              heartbeat_counter = 0;
              *recv_heartbeat = false;
            }
            else
            {
              heartbeat_counter++;
              if (heartbeat_counter >= kHeartBeatThreshold)
                return;
            }
          }
        },
        &mutex, &recv_heartbeat);
    unsigned msg_type;
    for (;;)
    {
      // exit
      if (is_exit_)
      {
        msg_type = static_cast<unsigned>(MsgType::kDisconnect);
        {
          std::lock_guard<std::mutex> lock(mutex);
          SendMessage(fd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type),
                      0);
        }
        close(fd);
        return;
      }
      // forward message
      if (!msg_queues_[fd].empty())
      {
        msg_type = static_cast<unsigned>(MsgType::kMsg);
        MessageInfo msg;
        msg_queues_[fd].pop(msg);
        const size_t msg_len = msg.content.length();
        {
          std::lock_guard<std::mutex> lock(mutex);
          SendMessage(fd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type),
                      0);
          SendMessage(fd, reinterpret_cast<void *>(&msg.src), sizeof(msg.src),
                      0);
          SendMessage(fd, reinterpret_cast<const void *>(&msg_len),
                      sizeof(msg_len), 0);
          SendMessage(fd, reinterpret_cast<void *>(msg.content.data()),
                      sizeof(char) * msg_len, 0);
        }
        std::cout << "Sent Message to " << fd << " from " << msg.src << ": "
                  << msg.content << std::endl;
      }
      // handle requestint count;
      int count;
      ioctl(fd, FIONREAD, &count);
      if (count)
      {
        RecvMessage(fd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type),
                    0);
        const MsgType msg_type_ = static_cast<MsgType>(msg_type);
        switch (msg_type_)
        {
        case MsgType::kTime:
        {
          std::time_t cur_time = time(nullptr);
          {
            std::lock_guard<std::mutex> lock(mutex);
            SendMessage(fd, reinterpret_cast<void *>(&msg_type),
                        sizeof(msg_type), 0);
            SendMessage(fd, reinterpret_cast<void *>(&cur_time),
                        sizeof(cur_time), 0);
          }
        }
        break;
        case MsgType::kHostname:
        {
          std::lock_guard<std::mutex> lock(mutex);
          SendMessage(fd, reinterpret_cast<void *>(&msg_type),
                      sizeof(msg_type), 0);
          SendMessage(fd, reinterpret_cast<void *>(&hostname_len_),
                      sizeof(hostname_len_), 0);
          SendMessage(fd, reinterpret_cast<void *>(hostname_.data()),
                      sizeof(char) * hostname_len_, 0);
        }
        break;
        case MsgType::kList:
        {
          std::lock_guard<std::mutex> lock(clients_mutex_);
          const size_t list_len = clients_.size();
          {
            std::lock_guard<std::mutex> lock(mutex);
            SendMessage(fd, reinterpret_cast<void *>(&msg_type),
                        sizeof(msg_type), 0);
            SendMessage(fd, reinterpret_cast<const void *>(&fd),
                        sizeof(fd), 0);
            SendMessage(fd, reinterpret_cast<const void *>(&list_len),
                        sizeof(list_len), 0);
            for (const auto &client : clients_)
            {
              SendMessage(fd, reinterpret_cast<const void *>(&client.first),
                          sizeof(client.first), 0);
              SendMessage(fd, reinterpret_cast<const void *>(&client.second),
                          sizeof(client.second), 0);
            }
          }
        }
        break;
        case MsgType::kMsg:
        {
          int dst;
          size_t msg_len;
          std::string msg;
          RecvMessage(fd, reinterpret_cast<void *>(&dst), sizeof(dst), 0);
          RecvMessage(fd, reinterpret_cast<void *>(&msg_len), sizeof(msg_len),
                      0);
          msg.resize(msg_len);
          RecvMessage(fd, reinterpret_cast<void *>(msg.data()),
                      sizeof(char) * msg_len, 0);
          std::cout << "Sent Message to " << dst << " from " << fd << ": "
                    << msg << std::endl;
          if (msg_queues_.count(dst))
          {
            msg_queues_[dst].push(MessageInfo{fd, msg});
            msg_type = static_cast<unsigned>(MsgType::kSuccess);
            {
              std::lock_guard<std::mutex> lock(mutex);
              SendMessage(fd, reinterpret_cast<void *>(&msg_type),
                          sizeof(msg_type), 0);
            }
          }
          else
          {
            msg_type = static_cast<unsigned>(MsgType::kError);
            {
              std::lock_guard<std::mutex> lock(mutex);
              SendMessage(fd, reinterpret_cast<void *>(&msg_type),
                          sizeof(msg_type), 0);
            }
          }
        }
        break;
        case MsgType::kHeartBeat:
        {
          recv_heartbeat = true;
        }
        break;
        default:
          std::cout << "unknown message type: " << msg_type << std::endl;
        case MsgType::kDisconnect:
        {
          close(fd);
          std::lock_guard<std::mutex> lock(clients_mutex_);
          clients_.erase(fd);
          char address[INET_ADDRSTRLEN];
          inet_ntop(AF_INET, &addr.sin_addr, address, sizeof(address));
          std::cout << address << ':' << addr.sin_port << " disconnected."
                    << std::endl;
          return;
        }
        }
      }
      // check if alive
      // if (alive.wait_for(std::chrono::milliseconds(1)) ==
      //     std::future_status::ready) {
      //   close(fd);
      //   std::lock_guard<std::mutex> lock(clients_mutex_);
      //   clients_.erase(fd);
      //   char address[INET_ADDRSTRLEN];
      //   inet_ntop(AF_INET, &addr.sin_addr, address, sizeof(address));
      //   std::cout << address << ':' << addr.sin_port
      //             << " disconnected accidentally." << std::endl;
      //   return;
      // }
    }
  };
  threads_[fd] =
      std::make_unique<decltype(std::thread(f))>(std::thread(std::move(f)));
}

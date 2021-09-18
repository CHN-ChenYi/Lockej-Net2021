#pragma once

#include <netinet/in.h>

#include <string>

using socket_fd = int;

void Init(const std::string &hostname);

void AddClient(const sockaddr_in &addr, const socket_fd &fd);

void Exit();

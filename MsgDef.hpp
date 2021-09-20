#pragma once

#include <chrono>
using namespace std::chrono_literals;

const auto kHeartBeatInterval = 100ms;
const int kHeartBeatThreshold = 10;

const int kServerPort = 4500;

enum class MsgType {
  kHeartBeat = -1,
  kDisconnect = 0,
  kTime,
  kHostname,
  kList,
  kMsg,
  kError,
  kSuccess
};

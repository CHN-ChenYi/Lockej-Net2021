#pragma once

const int kServerPort = 4500;

enum class MsgType {
  kDisconnect = 0,
  kTime,
  kHostname,
  kList,
  kMsg,
  kError,
  kSuccess
};

#pragma once

enum class MsgType {
  kDisconnect = 0,
  kTime,
  kHostname,
  kList,
  kMsg,
  kError,
  kSuccess
};

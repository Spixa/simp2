#pragma once

#include <cinttypes>
#include <string>

namespace simp {
  enum class Packets : uint32_t {
    SendMessagePacket = 0,
    JoinMessagePacket = 1,
    LeaveMessagePacket = 2,
    IdentifyMessagePacket = 3,
    RequestConsolePacket = 4,
    CommandResponsePacket = 5,
    DirectMessagePacket = 6,
    SendListPacket = 7
  };

  enum class DisconnectReason {
    DisconnectLeave = 0,
    DisconnectKick = 1,
    DisconnectUnnamed = 2,
    DisconnectSpam = 3
  };

  
};

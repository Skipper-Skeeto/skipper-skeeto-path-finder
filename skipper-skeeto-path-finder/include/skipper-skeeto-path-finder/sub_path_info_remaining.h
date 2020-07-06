#pragma once

#include <array>
#include <deque>
#include <vector>

class Room;

class SubPathInfoRemaining {
public:
  std::deque<std::vector<const Room *>> remainingUnfinishedSubPaths{{}};
  std::vector<const Room *> nextRoomsForFirstSubPath;
  std::array<bool, ROOM_COUNT> unavailableRooms{};
};

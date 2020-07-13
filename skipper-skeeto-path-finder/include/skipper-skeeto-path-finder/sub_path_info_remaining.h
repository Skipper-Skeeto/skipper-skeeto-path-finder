#pragma once

#include "skipper-skeeto-path-finder/info.h"
#include "skipper-skeeto-path-finder/sub_path.h"

#include <array>
#include <deque>
#include <memory>
#include <vector>

class Room;

class SubPathInfoRemaining {
public:
  std::deque<SubPath> remainingUnfinishedSubPaths{{}};
  std::vector<const Room *> nextRoomsForFirstSubPath;
  std::array<bool, ROOM_COUNT> unavailableRooms{};
};

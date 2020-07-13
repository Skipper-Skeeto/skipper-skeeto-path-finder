#pragma once

#include "skipper-skeeto-path-finder/info.h"
#include "skipper-skeeto-path-finder/sub_path.h"

#include <array>
#include <vector>

class Room;

class SubPathInfoRemaining {
public:
  std::vector<SubPath> remainingUnfinishedSubPaths{{}};
  std::vector<const Room *> nextRoomsForFirstSubPath;
  std::array<bool, ROOM_COUNT> unavailableRooms{};
};

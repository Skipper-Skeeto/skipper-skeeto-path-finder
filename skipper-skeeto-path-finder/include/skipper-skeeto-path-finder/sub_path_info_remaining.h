#pragma once

#include "skipper-skeeto-path-finder/sub_path.h"

#include <array>
#include <deque>
#include <memory>
#include <vector>

class Room;

class SubPathInfoRemaining {
public:
  std::deque<std::shared_ptr<SubPath>> remainingUnfinishedSubPaths{std::make_shared<SubPath>()};
  std::vector<const Room *> nextRoomsForFirstSubPath;
  std::array<bool, ROOM_COUNT> unavailableRooms{};
};

#include "skipper-skeeto-path-finder/sub_path.h"
#include "skipper-skeeto-path-finder/room.h"

SubPath::SubPath(const Room *newRoom, const SubPath &parent) {
  lastRoomIndex = newRoom->roomIndex;
  size = parent.visitedRoomsCount() + 1;
}

bool SubPath::isEmpty() const {
  return size == 0;
}

int SubPath::getLastRoomIndex() const {
  return lastRoomIndex;
}

int SubPath::visitedRoomsCount() const {
  return size;
}

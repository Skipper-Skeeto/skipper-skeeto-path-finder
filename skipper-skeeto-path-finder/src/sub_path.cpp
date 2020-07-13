#include "skipper-skeeto-path-finder/sub_path.h"

SubPath::SubPath(const Room *newRoom, const SubPath &parent) {
  lastRoom = newRoom;
  size = parent.visitedRoomsCount() + 1;
}

bool SubPath::isEmpty() const {
  return lastRoom == nullptr;
}

const Room *SubPath::getLastRoom() const {
  return lastRoom;
}

int SubPath::visitedRoomsCount() const {
  return size;
}

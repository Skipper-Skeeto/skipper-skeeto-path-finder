#include "skipper-skeeto-path-finder/sub_path.h"
#include "skipper-skeeto-path-finder/room.h"

SubPath::SubPath(const Room *newRoom, const SubPath &parent) {
  lastRoomIndex = newRoom->getUniqueIndex();
  size = parent.visitedRoomsCount() + 1;
}

SubPath::SubPath(std::istream &instream) {
  instream.read(reinterpret_cast<char *>(&lastRoomIndex), sizeof(lastRoomIndex));
  instream.read(reinterpret_cast<char *>(&size), sizeof(size));
}

void SubPath::serialize(std::ostream &outstream) const {
  outstream.write(reinterpret_cast<const char *>(&lastRoomIndex), sizeof(lastRoomIndex));
  outstream.write(reinterpret_cast<const char *>(&size), sizeof(size));
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

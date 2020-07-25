#include "skipper-skeeto-path-finder/sub_path_info_remaining.h"

SubPathInfoRemaining::SubPathInfoRemaining(std::istream &instream) {
  int remainingUnfinishedSubPathsSize = 0;
  instream.read(reinterpret_cast<char *>(&remainingUnfinishedSubPathsSize), sizeof(remainingUnfinishedSubPathsSize));
  for (int index = 0; index < remainingUnfinishedSubPathsSize; ++index) {
    remainingUnfinishedSubPaths.emplace_back(instream);
  }

  int nextRoomIndexesForFirstSubPathSize = 0;
  instream.read(reinterpret_cast<char *>(&nextRoomIndexesForFirstSubPathSize), sizeof(nextRoomIndexesForFirstSubPathSize));
  for (int index = 0; index < nextRoomIndexesForFirstSubPathSize; ++index) {
    unsigned char roomIndex;
    instream.read(reinterpret_cast<char *>(&roomIndex), sizeof(roomIndex));
    nextRoomIndexesForFirstSubPath.push_back(roomIndex);
  }

  for (int index = 0; index < ROOM_COUNT; ++index) {
    bool isUnavailable = false;
    instream.read(reinterpret_cast<char *>(&isUnavailable), sizeof(isUnavailable));
    unavailableRooms[index] = isUnavailable;
  }
}

void SubPathInfoRemaining::serialize(std::ostream &outstream) const {
  int remainingUnfinishedSubPathsSize = remainingUnfinishedSubPaths.size();
  outstream.write(reinterpret_cast<const char *>(&remainingUnfinishedSubPathsSize), sizeof(remainingUnfinishedSubPathsSize));
  for (const auto &subPath : remainingUnfinishedSubPaths) {
    subPath.serialize(outstream);
  }

  int nextRoomIndexesForFirstSubPathSize = nextRoomIndexesForFirstSubPath.size();
  outstream.write(reinterpret_cast<const char *>(&nextRoomIndexesForFirstSubPathSize), sizeof(nextRoomIndexesForFirstSubPathSize));
  for (const auto &subPath : nextRoomIndexesForFirstSubPath) {
    outstream.write(reinterpret_cast<const char *>(&subPath), sizeof(subPath));
  }

  for (auto isUnavailable : unavailableRooms) {
    outstream.write(reinterpret_cast<const char *>(&isUnavailable), sizeof(isUnavailable));
  }
}

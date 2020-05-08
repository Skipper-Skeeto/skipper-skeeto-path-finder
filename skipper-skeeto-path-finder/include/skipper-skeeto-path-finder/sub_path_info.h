#pragma once

#include <array>
#include <deque>
#include <list>
#include <vector>

class Room;
class Path;

class SubPathInfo {
public:
  SubPathInfo() = default;
  SubPathInfo(const SubPathInfo &subPathInfo){};

  std::list<Path>::iterator getNextPath();

  void erase(const std::list<Path>::iterator &iterator);

  void push_back(Path &&path);

  bool empty();

  std::deque<std::vector<const Room *>> remainingUnfinishedSubPaths{{}};
  std::vector<const Room *> nextRoomsForFirstSubPath;
  std::array<bool, ROOM_COUNT> unavailableRooms{};

private:
  std::list<Path> paths;
  std::list<Path>::iterator lastPathIterator = paths.end();
};
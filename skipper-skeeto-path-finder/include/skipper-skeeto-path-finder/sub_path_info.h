#pragma once

#include <array>
#include <deque>
#include <list>
#include <mutex>
#include <vector>

class Room;
class Path;

class SubPathInfo {
public:
  static const int MAX_DEPTH = 10;

  SubPathInfo() = default;
  SubPathInfo(const SubPathInfo &subPathInfo){};

  std::list<Path>::iterator getNextPath();

  void erase(const std::list<Path>::iterator &iterator);

  void push_back(Path &&path);

  bool empty();

  static std::array<std::pair<int, int>, MAX_DEPTH + 1> getTotalAndFinishedPathsCount();

  std::deque<std::vector<const Room *>> remainingUnfinishedSubPaths{{}};
  std::vector<const Room *> nextRoomsForFirstSubPath;
  std::array<bool, ROOM_COUNT> unavailableRooms{};

private:
  static std::array<std::mutex, MAX_DEPTH + 1> totalPathsMutexes;
  static std::array<std::mutex, MAX_DEPTH + 1> finishedPathsMutexes;

  static std::array<int, MAX_DEPTH + 1> totalPaths;
  static std::array<int, MAX_DEPTH + 1> finishedPaths;

  std::list<Path> paths;
  std::list<Path>::iterator lastPathIterator = paths.end();
};
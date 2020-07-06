#pragma once

#include <array>
#include <deque>
#include <mutex>
#include <vector>

class Room;
class Path;
class SubPathInfoRemaining;

class SubPathInfo {
public:
  static const int MAX_DEPTH = 10;

  SubPathInfo();

  SubPathInfo(const SubPathInfo &subPathInfo);

  ~SubPathInfo();

  std::vector<Path *>::iterator getNextPath();

  void erase(const std::vector<Path *>::iterator &iterator);

  void push_back(Path &&path);

  bool empty();

  SubPathInfoRemaining *remaining;

  static std::array<std::pair<int, int>, MAX_DEPTH + 1> getTotalAndFinishedPathsCount();

private:
  static std::array<std::mutex, MAX_DEPTH + 1> totalPathsMutexes;
  static std::array<std::mutex, MAX_DEPTH + 1> finishedPathsMutexes;

  static std::array<int, MAX_DEPTH + 1> totalPaths;
  static std::array<int, MAX_DEPTH + 1> finishedPaths;

  std::vector<Path *> paths;
  unsigned lastPathIndex = -1;
};
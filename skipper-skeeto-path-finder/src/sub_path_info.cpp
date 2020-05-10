#include "skipper-skeeto-path-finder/sub_path_info.h"

#include "skipper-skeeto-path-finder/path.h"

std::array<std::mutex, SubPathInfo::MAX_DEPTH + 1> SubPathInfo::totalPathsMutexes{};
std::array<std::mutex, SubPathInfo::MAX_DEPTH + 1> SubPathInfo::finishedPathsMutexes{};

std::array<int, SubPathInfo::MAX_DEPTH + 1> SubPathInfo::totalPaths{};
std::array<int, SubPathInfo::MAX_DEPTH + 1> SubPathInfo::finishedPaths{};

std::list<Path>::iterator SubPathInfo::getNextPath() {
  if (lastPathIterator == paths.end() || ++lastPathIterator == paths.end()) {
    lastPathIterator = paths.begin();
  }

  return lastPathIterator;
}

void SubPathInfo::erase(const std::list<Path>::iterator &iterator) {
  if (iterator->depth <= MAX_DEPTH) {
    std::lock_guard<std::mutex> finishedGuard(finishedPathsMutexes[iterator->depth]);
    ++finishedPaths[iterator->depth];
  }

  lastPathIterator = paths.erase(iterator);
  if (lastPathIterator != paths.begin()) {
    --lastPathIterator;
  } else {
    lastPathIterator = paths.end();
  }
}

void SubPathInfo::push_back(Path &&path) {
  if (path.depth <= MAX_DEPTH) {
    std::lock_guard<std::mutex> totalGuard(totalPathsMutexes[path.depth]);
    ++totalPaths[path.depth];
  }

  paths.push_back(std::move(path));
}

bool SubPathInfo::empty() {
  return paths.empty();
}

std::array<std::pair<int, int>, SubPathInfo::MAX_DEPTH + 1> SubPathInfo::getTotalAndFinishedPathsCount() {
  std::array<std::pair<int, int>, MAX_DEPTH + 1> totalAndFinishedPathsCount;

  for (int depth = 0; depth < totalAndFinishedPathsCount.size(); ++depth) {
    std::lock_guard<std::mutex> totalGuard(totalPathsMutexes[depth]);
    std::lock_guard<std::mutex> finishedGuard(finishedPathsMutexes[depth]);

    totalAndFinishedPathsCount[depth] = std::make_pair(totalPaths[depth], finishedPaths[depth]);
  }

  return totalAndFinishedPathsCount;
}

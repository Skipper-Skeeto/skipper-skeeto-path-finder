#include "skipper-skeeto-path-finder/sub_path_info.h"

#include "skipper-skeeto-path-finder/path.h"

std::array<std::mutex, SubPathInfo::MAX_DEPTH + 1> SubPathInfo::totalPathsMutexes{};
std::array<std::mutex, SubPathInfo::MAX_DEPTH + 1> SubPathInfo::finishedPathsMutexes{};

std::array<int, SubPathInfo::MAX_DEPTH + 1> SubPathInfo::totalPaths{};
std::array<int, SubPathInfo::MAX_DEPTH + 1> SubPathInfo::finishedPaths{};

std::vector<Path *>::iterator SubPathInfo::getNextPath() {
  if (lastPathIndex >= paths.size() || ++lastPathIndex >= paths.size()) {
    lastPathIndex = 0;
  }

  return paths.begin() + lastPathIndex;
}

void SubPathInfo::erase(const std::vector<Path *>::iterator &iterator) {
  auto pathDepth = (*iterator)->depth;
  if (pathDepth <= MAX_DEPTH) {
    std::lock_guard<std::mutex> finishedGuard(finishedPathsMutexes[pathDepth]);
    ++finishedPaths[pathDepth];
  }

  if (iterator == paths.begin() + lastPathIndex) {
    // If the are going to delete the "current" index,
    // we want the next to be the one after that
    --lastPathIndex;
  }

  delete *iterator;
  paths.erase(iterator);
}

void SubPathInfo::push_back(Path &&path) {
  if (path.depth <= MAX_DEPTH) {
    std::lock_guard<std::mutex> totalGuard(totalPathsMutexes[path.depth]);
    ++totalPaths[path.depth];
  }

  paths.push_back(new Path(std::move(path)));
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

#include "skipper-skeeto-path-finder/sub_path_info.h"

#include "skipper-skeeto-path-finder/path.h"
#include "skipper-skeeto-path-finder/sub_path_info_remaining.h"

std::array<std::mutex, SubPathInfo::MAX_DEPTH + 1> SubPathInfo::totalPathsMutexes{};
std::array<std::mutex, SubPathInfo::MAX_DEPTH + 1> SubPathInfo::finishedPathsMutexes{};

std::array<int, SubPathInfo::MAX_DEPTH + 1> SubPathInfo::totalPaths{};
std::array<int, SubPathInfo::MAX_DEPTH + 1> SubPathInfo::finishedPaths{};

SubPathInfo::SubPathInfo() {
  remaining = new SubPathInfoRemaining();
}

SubPathInfo::SubPathInfo(const SubPathInfo &subPathInfo) {
  remaining = new SubPathInfoRemaining();
}

SubPathInfo::~SubPathInfo() {
  cleanUp();
}

void SubPathInfo::deserialize(std::istream &instream, const Path *parent) {
  cleanUp();

  bool hasRemaining = false;
  instream.read(reinterpret_cast<char *>(&hasRemaining), sizeof(hasRemaining));
  if (hasRemaining) {
    remaining = new SubPathInfoRemaining(instream);
  }

  instream.read(reinterpret_cast<char *>(&lastPathIndex), sizeof(lastPathIndex));

  int pathCount = 0;
  instream.read(reinterpret_cast<char *>(&pathCount), sizeof(pathCount));
  for (int index = 0; index < pathCount; ++index) {
    paths.push_back(new Path(instream, parent));
  }
}

void SubPathInfo::serialize(std::ostream &outstream) const {
  bool hasRemaining = (remaining != nullptr);
  outstream.write(reinterpret_cast<const char *>(&hasRemaining), sizeof(hasRemaining));
  if (hasRemaining) {
    remaining->serialize(outstream);
  }

  outstream.write(reinterpret_cast<const char *>(&lastPathIndex), sizeof(lastPathIndex));

  int pathCount = paths.size();
  outstream.write(reinterpret_cast<const char *>(&pathCount), sizeof(pathCount));
  for (auto path : paths) {
    path->serialize(outstream);
  }
}

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

bool SubPathInfo::erase(const Path *path) {
  auto pathIterator = std::find(paths.begin(), paths.end(), path);
  if (pathIterator == paths.end()) {
    return false;
  }

  erase(pathIterator);
  return true;
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

void SubPathInfo::cleanUp() {
  if (remaining != nullptr) {
    delete remaining;
    remaining = nullptr;
  }

  for (auto path : paths) {
    delete path;
  }
  paths.clear();
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

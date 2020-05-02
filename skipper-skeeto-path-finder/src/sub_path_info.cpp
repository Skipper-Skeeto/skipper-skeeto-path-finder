#include "skipper-skeeto-path-finder/sub_path_info.h"

#include "skipper-skeeto-path-finder/path.h"

std::list<Path>::iterator SubPathInfo::getNextPath() {
  if (lastPathIterator == paths.end() || ++lastPathIterator == paths.end()) {
    lastPathIterator = paths.begin();
  }

  return lastPathIterator;
}

void SubPathInfo::erase(const std::list<Path>::iterator &iterator) {
  lastPathIterator = paths.erase(iterator);
  if (lastPathIterator != paths.begin()) {
    --lastPathIterator;
  } else {
    lastPathIterator = paths.end();
  }
}

void SubPathInfo::push_back(Path &&path) {
  paths.push_back(std::move(path));
}

bool SubPathInfo::empty() {
  return paths.empty();
}

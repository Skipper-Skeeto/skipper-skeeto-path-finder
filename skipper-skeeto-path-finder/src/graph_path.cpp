#include "skipper-skeeto-path-finder/graph_path.h"

#include "skipper-skeeto-path-finder/info.h"

const State GraphPath::ALL_VERTICES_STATE_MASK = (1ULL << VERTICES_COUNT) - 1;

GraphPath::GraphPath(char startVertexIndex) {
  setCurrentVertex(startVertexIndex);
}

GraphPath::GraphPath(char vertexIndex, GraphPath *previousPath, char extraDistance) {
  this->previousPath = previousPath;
  state = previousPath->getState(); // Has to be before altering the local state
  distance = previousPath->getDistance() + extraDistance;
  setCurrentVertex(vertexIndex);
}

void GraphPath::initialize(const std::vector<GraphPath> &nextPaths) {
  this->nextPaths = nextPaths;
  focusedNextPathIndex = 0;
}

char GraphPath::getCurrentVertex() const {
  return state >> VERTICES_COUNT;
}

bool GraphPath::isInitialized() const {
  return focusedNextPathIndex >= 0;
}

bool GraphPath::isExhausted() const {
  return nextPaths.empty();
}

bool GraphPath::isFinished() const {
  return meetsCondition(ALL_VERTICES_STATE_MASK);
}

unsigned char GraphPath::getDistance() const {
  return distance;
}

State GraphPath::getState() const {
  return state;
}

bool GraphPath::meetsCondition(const State &condition) const {
  return (state & condition) == condition;
}

bool GraphPath::hasVisitedVertex(char vertexIndex) const {
  return meetsCondition(1ULL << vertexIndex);
}

std::vector<GraphPath>::iterator GraphPath::getFocusedNextPath() {
  return nextPaths.begin() + focusedNextPathIndex;
}

void GraphPath::eraseNextPath(const std::vector<GraphPath>::iterator &pathIterator) {
  nextPaths.erase(pathIterator);

  if (focusedNextPathIndex >= nextPaths.size()) {
    focusedNextPathIndex = 0;
  }
}

void GraphPath::bumpFocusedNextPath() {
  if (++focusedNextPathIndex >= nextPaths.size()) {
    focusedNextPathIndex = 0;
  }
}

std::vector<char> GraphPath::getRoute() const {
  if (previousPath == nullptr) {
    return std::vector<char>{getCurrentVertex()};
  } else {
    auto partialRoute = previousPath->getRoute();
    partialRoute.push_back(getCurrentVertex());
    return partialRoute;
  }
}

void GraphPath::setCurrentVertex(char vertexIndex) {
  state &= ALL_VERTICES_STATE_MASK; // Clear everything except visited vertices state
  state |= ((State)vertexIndex << VERTICES_COUNT);
  state |= (1ULL << vertexIndex);
}

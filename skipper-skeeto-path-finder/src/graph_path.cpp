#include "skipper-skeeto-path-finder/graph_path.h"

#include "skipper-skeeto-path-finder/info.h"

#define VISITED_VERTICES_INDEX 0
#define CURRENT_VERTEX_INDEX (VISITED_VERTICES_INDEX + VERTICES_COUNT)
#define DISTANCE_INDEX (CURRENT_VERTEX_INDEX + CURRENT_VERTEX_BITS)
#define FOCUSED_NEXT_PATH_INDEX (DISTANCE_INDEX + DISTANCE_BITS)
static_assert(sizeof(State) < (FOCUSED_NEXT_PATH_INDEX + FOCUSED_NEXT_PATH_BITS), "Bits does not fit in state");

#define ALL_VERTICES_STATE_MASK ((1ULL << VERTICES_COUNT) - 1)
#define NOT_INITIALIZED_FOCUSED_MASK ((1ULL << FOCUSED_NEXT_PATH_BITS) - 1)

GraphPath::GraphPath(char startVertexIndex) {
  setCurrentVertex(startVertexIndex);
  setState<FOCUSED_NEXT_PATH_INDEX, FOCUSED_NEXT_PATH_BITS>(NOT_INITIALIZED_FOCUSED_MASK);
}

GraphPath::GraphPath(char vertexIndex, GraphPath *previousPath, char extraDistance) {
  this->previousPath = previousPath;
  setState<VISITED_VERTICES_INDEX, VERTICES_COUNT>(previousPath->getVisitedVertices()); // Has to be before setting current vertex
  setState<DISTANCE_INDEX, DISTANCE_BITS>(previousPath->getDistance() + extraDistance);
  setCurrentVertex(vertexIndex);
  setState<FOCUSED_NEXT_PATH_INDEX, FOCUSED_NEXT_PATH_BITS>(NOT_INITIALIZED_FOCUSED_MASK);
}

void GraphPath::initialize(const std::vector<GraphPath> &nextPaths) {
  this->nextPaths = nextPaths;
  setState<FOCUSED_NEXT_PATH_INDEX, FOCUSED_NEXT_PATH_BITS>(0);
}

char GraphPath::getCurrentVertex() const {
  return getState<CURRENT_VERTEX_INDEX, CURRENT_VERTEX_BITS>();
}

bool GraphPath::isInitialized() const {
  return getState<FOCUSED_NEXT_PATH_INDEX, FOCUSED_NEXT_PATH_BITS>() != NOT_INITIALIZED_FOCUSED_MASK;
}

bool GraphPath::isExhausted() const {
  return nextPaths.empty();
}

bool GraphPath::isFinished() const {
  return meetsCondition(ALL_VERTICES_STATE_MASK);
}

State GraphPath::getUniqueState() const {
  return getState<VISITED_VERTICES_INDEX, VERTICES_COUNT>() | (getState<CURRENT_VERTEX_INDEX, CURRENT_VERTEX_BITS>() << VERTICES_COUNT);
}

unsigned char GraphPath::getDistance() const {
  return getState<DISTANCE_INDEX, DISTANCE_BITS>();
}

State GraphPath::getVisitedVertices() const {
  return getState<VISITED_VERTICES_INDEX, VERTICES_COUNT>();
}

bool GraphPath::meetsCondition(const State &condition) const {
  return (getState<VISITED_VERTICES_INDEX, VERTICES_COUNT>() & condition) == condition;
}

bool GraphPath::hasVisitedVertex(char vertexIndex) const {
  return meetsCondition(1ULL << vertexIndex);
}

std::vector<GraphPath>::iterator GraphPath::getFocusedNextPath() {
  return nextPaths.begin() + getState<FOCUSED_NEXT_PATH_INDEX, FOCUSED_NEXT_PATH_BITS>();
}

void GraphPath::eraseNextPath(const std::vector<GraphPath>::iterator &pathIterator) {
  nextPaths.erase(pathIterator);

  auto focusedNextPathIndex = getState<FOCUSED_NEXT_PATH_INDEX, FOCUSED_NEXT_PATH_BITS>();
  if (focusedNextPathIndex >= nextPaths.size()) {
    setState<FOCUSED_NEXT_PATH_INDEX, FOCUSED_NEXT_PATH_BITS>(0);
  }
}

void GraphPath::bumpFocusedNextPath() {
  auto focusedNextPathIndex = getState<FOCUSED_NEXT_PATH_INDEX, FOCUSED_NEXT_PATH_BITS>();

  if (++focusedNextPathIndex >= nextPaths.size()) {
    focusedNextPathIndex = 0;
  }

  setState<FOCUSED_NEXT_PATH_INDEX, FOCUSED_NEXT_PATH_BITS>(focusedNextPathIndex);
}

int GraphPath::getNextPathsCount() const {
  return nextPaths.size();
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

void GraphPath::serialize(std::ostream &outstream) const {
  outstream.write(reinterpret_cast<const char *>(&state), sizeof(state));

  int pathCount = nextPaths.size();
  outstream.write(reinterpret_cast<const char *>(&pathCount), sizeof(pathCount));
  for (auto path : nextPaths) {
    path.serialize(outstream);
  }
}

void GraphPath::deserialize(std::istream &instream, const GraphPath *previousPath) {
  if (previousPath != nullptr) {
    this->previousPath = previousPath;
  }

  instream.read(reinterpret_cast<char *>(&state), sizeof(state));

  int pathCount = 0;
  instream.read(reinterpret_cast<char *>(&pathCount), sizeof(pathCount));
  for (int index = 0; index < pathCount; ++index) {
    GraphPath path;
    path.deserialize(instream, this);
    nextPaths.push_back(path);
  }
}

void GraphPath::cleanUp() {
  nextPaths.clear();
}

void GraphPath::setCurrentVertex(char vertexIndex) {
  setState<CURRENT_VERTEX_INDEX, CURRENT_VERTEX_BITS>(vertexIndex);
  state |= (1ULL << vertexIndex);
}

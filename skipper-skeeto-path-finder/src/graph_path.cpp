#include "skipper-skeeto-path-finder/graph_path.h"

#include "skipper-skeeto-path-finder/graph_path_pool.h"
#include "skipper-skeeto-path-finder/info.h"

#define FOCUSED_SUB_PATH_SET_BITS 1
#define HAS_STATE_MAX_BITS 1
#define VISITED_VERTICES_BITS (VERTICES_COUNT - 1) // We always reached start, so we shift the visited vertices state bits

#define VISITED_VERTICES_INDEX 0
#define CURRENT_VERTEX_INDEX (VISITED_VERTICES_INDEX + VISITED_VERTICES_BITS)
#define DISTANCE_INDEX (CURRENT_VERTEX_INDEX + CURRENT_VERTEX_BITS)
#define FOCUSED_SUB_PATH_SET_INDEX (DISTANCE_INDEX + DISTANCE_BITS)
#define HAS_STATE_MAX_INDEX (FOCUSED_SUB_PATH_SET_INDEX + FOCUSED_SUB_PATH_SET_BITS)
static_assert(sizeof(State) < (HAS_STATE_MAX_INDEX + HAS_STATE_MAX_BITS), "Bits does not fit in state");

#define ALL_VERTICES_STATE_MASK ((1ULL << VISITED_VERTICES_BITS) - 1)

void GraphPath::initialize(char vertexIndex, unsigned long int parentPathIndex, const GraphPath *parentPath, char extraDistance) {
  this->parentPathIndex = parentPathIndex;
  state = 0;
  bestEndDistance = 255;

  State parentVisitedVertices = 0;
  State parentDistance = 0;
  if (parentPath != nullptr) {
    parentVisitedVertices = parentPath->getVisitedVerticesState();
    parentDistance = parentPath->getDistance();
  }

  setState<VISITED_VERTICES_INDEX, VISITED_VERTICES_BITS>(parentVisitedVertices); // Has to be before setting current vertex
  setState<DISTANCE_INDEX, DISTANCE_BITS>(parentDistance + extraDistance);
  setCurrentVertex(vertexIndex);
}

void GraphPath::initializeAsCopy(const GraphPath *sourcePath, unsigned long int parentPathIndex) {
  this->parentPathIndex = parentPathIndex;
  state = sourcePath->state;
  bestEndDistance = sourcePath->bestEndDistance;
}

void GraphPath::setFocusedSubPath(unsigned long int index) {
  this->focusedSubPathIndex = index;
  setState<FOCUSED_SUB_PATH_SET_INDEX, FOCUSED_SUB_PATH_SET_BITS>(1);
}

unsigned long int GraphPath::getFocusedSubPath() const {
  return focusedSubPathIndex;
}

bool GraphPath::hasSetSubPath() const {
  return getState<FOCUSED_SUB_PATH_SET_INDEX, FOCUSED_SUB_PATH_SET_BITS>() > 0;
}

void GraphPath::setNextPath(unsigned long int index) {
  nextPathIndex = index;
}

unsigned long int GraphPath::getNextPath() const {
  return nextPathIndex;
}

void GraphPath::setPreviousPath(unsigned long int index) {
  previousPathIndex = index;
}

unsigned long int GraphPath::getPreviousPath() const {
  return previousPathIndex;
}

void GraphPath::setHasStateMax(bool hasMax) {
  setState<HAS_STATE_MAX_INDEX, HAS_STATE_MAX_BITS>(hasMax ? 1 : 0);
}

bool GraphPath::hasStateMax() const {
  return getState<HAS_STATE_MAX_INDEX, HAS_STATE_MAX_BITS>() > 0;
}

char GraphPath::getCurrentVertex() const {
  return getState<CURRENT_VERTEX_INDEX, CURRENT_VERTEX_BITS>();
}

bool GraphPath::isExhausted() const {
  return focusedSubPathIndex == 0;
}

bool GraphPath::isFinished() const {
  return meetsCondition(ALL_VERTICES_STATE_MASK);
}

State GraphPath::getUniqueState() const {
  return getState<VISITED_VERTICES_INDEX, VISITED_VERTICES_BITS>() | (getState<CURRENT_VERTEX_INDEX, CURRENT_VERTEX_BITS>() << VISITED_VERTICES_BITS);
}

unsigned char GraphPath::getDistance() const {
  return getState<DISTANCE_INDEX, DISTANCE_BITS>();
}

State GraphPath::getVisitedVerticesState() const {
  return getState<VISITED_VERTICES_INDEX, VISITED_VERTICES_BITS>();
}

bool GraphPath::meetsCondition(const State &condition) const {
  return (getState<VISITED_VERTICES_INDEX, VISITED_VERTICES_BITS>() & condition) == condition;
}

bool GraphPath::hasVisitedVertex(char vertexIndex) const {
  if (vertexIndex == 0) {
    return true;
  }

  // We always reached start, so the visited vertices state bits have been shifted
  return meetsCondition(1ULL << (vertexIndex - 1));
}

void GraphPath::maybeSetBestEndDistance(GraphPathPool *pool, unsigned char distance) {
  if (distance < bestEndDistance) {
    bestEndDistance = distance;

    if (parentPathIndex > 0) {
      pool->getGraphPath(parentPathIndex)->maybeSetBestEndDistance(pool, distance);
    }
  }
}

unsigned char GraphPath::getBestEndDistance() const {
  return bestEndDistance;
}

std::vector<char> GraphPath::getRoute(const GraphPathPool *pool) const {
  if (parentPathIndex == 0) {
    return std::vector<char>{getCurrentVertex()};
  } else {
    auto partialRoute = pool->getGraphPath(parentPathIndex)->getRoute(pool);
    partialRoute.push_back(getCurrentVertex());
    return partialRoute;
  }
}

void GraphPath::serialize(std::ostream &outstream) const {
  outstream.write(reinterpret_cast<const char *>(&state), sizeof(state));
  outstream.write(reinterpret_cast<const char *>(&bestEndDistance), sizeof(bestEndDistance));
  outstream.write(reinterpret_cast<const char *>(&parentPathIndex), sizeof(parentPathIndex));
  outstream.write(reinterpret_cast<const char *>(&previousPathIndex), sizeof(previousPathIndex));
  outstream.write(reinterpret_cast<const char *>(&nextPathIndex), sizeof(nextPathIndex));
  outstream.write(reinterpret_cast<const char *>(&focusedSubPathIndex), sizeof(focusedSubPathIndex));
}

void GraphPath::deserialize(std::istream &instream) {
  instream.read(reinterpret_cast<char *>(&state), sizeof(state));
  instream.read(reinterpret_cast<char *>(&bestEndDistance), sizeof(bestEndDistance));
  instream.read(reinterpret_cast<char *>(&parentPathIndex), sizeof(parentPathIndex));
  instream.read(reinterpret_cast<char *>(&previousPathIndex), sizeof(previousPathIndex));
  instream.read(reinterpret_cast<char *>(&nextPathIndex), sizeof(nextPathIndex));
  instream.read(reinterpret_cast<char *>(&focusedSubPathIndex), sizeof(focusedSubPathIndex));
}

void GraphPath::cleanUp() {
  state = 0;
}

void GraphPath::setCurrentVertex(char vertexIndex) {
  setState<CURRENT_VERTEX_INDEX, CURRENT_VERTEX_BITS>(vertexIndex);

  if (vertexIndex > 0) {
    // We always reached start, so the visited vertices state bits have been shifted
    state |= (1ULL << (vertexIndex - 1));
  }
}

#include "skipper-skeeto-path-finder/graph_path.h"

#include "skipper-skeeto-path-finder/graph_path_pool.h"
#include "skipper-skeeto-path-finder/info.h"

#define FOCUSED_SUB_PATH_SET_BITS 1
#define HAS_STATE_MAX_BITS 1

#define CURRENT_VERTEX_INDEX 0
#define DISTANCE_INDEX (CURRENT_VERTEX_INDEX + CURRENT_VERTEX_BITS)
#define BEST_END_DISTANCE_INDEX (DISTANCE_INDEX + DISTANCE_BITS)
#define FOCUSED_SUB_PATH_SET_INDEX (BEST_END_DISTANCE_INDEX + DISTANCE_BITS)
#define HAS_STATE_MAX_INDEX (FOCUSED_SUB_PATH_SET_INDEX + FOCUSED_SUB_PATH_SET_BITS)
static_assert(sizeof(State) < (HAS_STATE_MAX_INDEX + HAS_STATE_MAX_BITS), "Bits does not fit in state");

void GraphPath::initialize(char vertexIndex, unsigned long int parentPathIndex, const GraphPath *parentPath, char extraDistance) {
  this->parentPathIndex = parentPathIndex;
  state.clear();

  unsigned char parentDistance = 0;
  if (parentPath != nullptr) {
    parentDistance = parentPath->getDistance();
  }

  state.setBits<DISTANCE_INDEX, DISTANCE_BITS>(parentDistance + extraDistance);
  state.setBits<BEST_END_DISTANCE_INDEX, DISTANCE_BITS>((1 << DISTANCE_BITS) - 1);
  state.setBits<CURRENT_VERTEX_INDEX, CURRENT_VERTEX_BITS>(vertexIndex);
}

void GraphPath::initializeAsCopy(const GraphPath *sourcePath, unsigned long int parentPathIndex) {
  this->parentPathIndex = parentPathIndex;
  state = sourcePath->state;
}

void GraphPath::setFocusedSubPath(unsigned long int index) {
  this->focusedSubPathIndex = index;
  state.setBits<FOCUSED_SUB_PATH_SET_INDEX, FOCUSED_SUB_PATH_SET_BITS>(1);
}

unsigned long int GraphPath::getFocusedSubPath() const {
  return focusedSubPathIndex;
}

bool GraphPath::hasSetSubPath() const {
  return state.meetsCondition<FOCUSED_SUB_PATH_SET_INDEX, FOCUSED_SUB_PATH_SET_BITS>(1);
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
  state.setBits<HAS_STATE_MAX_INDEX, HAS_STATE_MAX_BITS>(hasMax ? 1 : 0);
}

bool GraphPath::hasStateMax() const {
  return state.meetsCondition<HAS_STATE_MAX_INDEX, HAS_STATE_MAX_BITS>(1);
}

char GraphPath::getCurrentVertex() const {
  return state.getBits<CURRENT_VERTEX_INDEX, CURRENT_VERTEX_BITS>();
}

bool GraphPath::isExhausted() const {
  return focusedSubPathIndex == 0;
}

unsigned char GraphPath::getDistance() const {
  return state.getBits<DISTANCE_INDEX, DISTANCE_BITS>();
}

void GraphPath::maybeSetBestEndDistance(GraphPathPool *pool, unsigned char distance) {
  if (distance < getBestEndDistance()) {
    state.setBits<BEST_END_DISTANCE_INDEX, DISTANCE_BITS>(distance);

    if (parentPathIndex > 0) {
      pool->getGraphPath(parentPathIndex)->maybeSetBestEndDistance(pool, distance);
    }
  }
}

unsigned char GraphPath::getBestEndDistance() const {
  return state.getBits<BEST_END_DISTANCE_INDEX, DISTANCE_BITS>();
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
  outstream.write(reinterpret_cast<const char *>(&parentPathIndex), sizeof(parentPathIndex));
  outstream.write(reinterpret_cast<const char *>(&previousPathIndex), sizeof(previousPathIndex));
  outstream.write(reinterpret_cast<const char *>(&nextPathIndex), sizeof(nextPathIndex));
  outstream.write(reinterpret_cast<const char *>(&focusedSubPathIndex), sizeof(focusedSubPathIndex));
}

void GraphPath::deserialize(std::istream &instream) {
  instream.read(reinterpret_cast<char *>(&state), sizeof(state));
  instream.read(reinterpret_cast<char *>(&parentPathIndex), sizeof(parentPathIndex));
  instream.read(reinterpret_cast<char *>(&previousPathIndex), sizeof(previousPathIndex));
  instream.read(reinterpret_cast<char *>(&nextPathIndex), sizeof(nextPathIndex));
  instream.read(reinterpret_cast<char *>(&focusedSubPathIndex), sizeof(focusedSubPathIndex));
}

void GraphPath::cleanUp() {
  state.clear();
}

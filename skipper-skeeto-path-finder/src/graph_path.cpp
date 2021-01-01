#include "skipper-skeeto-path-finder/graph_path.h"

#include "skipper-skeeto-path-finder/graph_path_pool.h"
#include "skipper-skeeto-path-finder/info.h"

#define POOL_INDEX_BITS 25
#define CURRENT_VERTEX_BITS 6
#define DISTANCE_BITS 7
#define FOCUSED_SUB_PATH_SET_BITS 1
#define HAS_STATE_MAX_BITS 1
#define SUB_PATH_ITERATION_COUNT_BITS 5

static_assert((1ULL << POOL_INDEX_BITS) > POOL_COUNT, "Too many items allowed in graph path pools. Reduce POOL_TOTAL_BYTES, increase THREAD_COUNT or restructure GraphPath so POOL_INDEX_BITS can be biggger");

// State A
#define PARENT_PATH_STATE stateA
#define FOCUSED_SUB_PATH_STATE stateA
#define MINIMUM_END_DISTANCE_STATE stateA
#define BEST_END_DISTANCE_STATE stateA
#define PARENT_PATH_INDEX 0
#define FOCUSED_SUB_PATH_INDEX (PARENT_PATH_INDEX + POOL_INDEX_BITS)
#define MINIMUM_END_DISTANCE_INDEX (FOCUSED_SUB_PATH_INDEX + POOL_INDEX_BITS)
#define BEST_END_DISTANCE_INDEX (MINIMUM_END_DISTANCE_INDEX + DISTANCE_BITS)
static_assert(sizeof(State) * 8 >= (BEST_END_DISTANCE_INDEX + DISTANCE_BITS), "Bits does not fit in state A");

// State B
#define NEXT_PATH_STATE stateB
#define PREVIOUS_PATH_STATE stateB
#define CURRENT_VERTEX_STATE stateB
#define FOCUSED_SUB_PATH_SET_STATE stateB
#define HAS_STATE_MAX_STATE stateB
#define SUB_PATH_ITERATION_COUNT_STATE stateB
#define NEXT_PATH_INDEX 0
#define PREVIOUS_SUB_PATH_INDEX (NEXT_PATH_INDEX + POOL_INDEX_BITS)
#define CURRENT_VERTEX_INDEX (PREVIOUS_SUB_PATH_INDEX + POOL_INDEX_BITS)
#define FOCUSED_SUB_PATH_SET_INDEX (CURRENT_VERTEX_INDEX + CURRENT_VERTEX_BITS)
#define HAS_STATE_MAX_INDEX (FOCUSED_SUB_PATH_SET_INDEX + FOCUSED_SUB_PATH_SET_BITS)
#define SUB_PATH_ITERATION_COUNT_INDEX (HAS_STATE_MAX_INDEX + HAS_STATE_MAX_BITS)
static_assert(sizeof(State) * 8 >= (SUB_PATH_ITERATION_COUNT_INDEX + SUB_PATH_ITERATION_COUNT_BITS), "Bits does not fit in state B");

const unsigned char GraphPath::MAX_DISTANCE = (1 << DISTANCE_BITS) - 1;

void GraphPath::initialize(char vertexIndex, unsigned long int parentPathIndex, unsigned char minimumEndDistance) {
  stateA.clear();
  stateB.clear();

  PARENT_PATH_STATE.setBits<PARENT_PATH_INDEX, POOL_INDEX_BITS>(parentPathIndex);
  MINIMUM_END_DISTANCE_STATE.setBits<MINIMUM_END_DISTANCE_INDEX, DISTANCE_BITS>(minimumEndDistance);
  BEST_END_DISTANCE_STATE.setBits<BEST_END_DISTANCE_INDEX, DISTANCE_BITS>(MAX_DISTANCE);
  CURRENT_VERTEX_STATE.setBits<CURRENT_VERTEX_INDEX, CURRENT_VERTEX_BITS>(vertexIndex);
}

void GraphPath::initializeAsCopy(const GraphPath *sourcePath, unsigned long int parentPathIndex) {
  stateA = sourcePath->stateA;
  stateB = sourcePath->stateB;

  PARENT_PATH_STATE.setBits<PARENT_PATH_INDEX, POOL_INDEX_BITS>(parentPathIndex);
}

void GraphPath::updateFocusedSubPath(unsigned long int index, unsigned char iterationCount) {
  FOCUSED_SUB_PATH_STATE.setBits<FOCUSED_SUB_PATH_INDEX, POOL_INDEX_BITS>(index);
  FOCUSED_SUB_PATH_SET_STATE.setBits<FOCUSED_SUB_PATH_SET_INDEX, FOCUSED_SUB_PATH_SET_BITS>(1);

  SUB_PATH_ITERATION_COUNT_STATE.setBits<SUB_PATH_ITERATION_COUNT_INDEX, SUB_PATH_ITERATION_COUNT_BITS>(iterationCount);
}

unsigned long int GraphPath::getFocusedSubPath() const {
  return FOCUSED_SUB_PATH_STATE.getBits<FOCUSED_SUB_PATH_INDEX, POOL_INDEX_BITS>();
}

unsigned char GraphPath::getSubPathIterationCount() const {
  return SUB_PATH_ITERATION_COUNT_STATE.getBits<SUB_PATH_ITERATION_COUNT_INDEX, SUB_PATH_ITERATION_COUNT_BITS>();
}

bool GraphPath::hasSetSubPath() const {
  return FOCUSED_SUB_PATH_SET_STATE.meetsCondition<FOCUSED_SUB_PATH_SET_INDEX, FOCUSED_SUB_PATH_SET_BITS>(1);
}

void GraphPath::setNextPath(unsigned long int index) {
  NEXT_PATH_STATE.setBits<NEXT_PATH_INDEX, POOL_INDEX_BITS>(index);
}

unsigned long int GraphPath::getNextPath() const {
  return NEXT_PATH_STATE.getBits<NEXT_PATH_INDEX, POOL_INDEX_BITS>();
}

void GraphPath::setPreviousPath(unsigned long int index) {
  PREVIOUS_PATH_STATE.setBits<PREVIOUS_SUB_PATH_INDEX, POOL_INDEX_BITS>(index);
}

unsigned long int GraphPath::getPreviousPath() const {
  return PREVIOUS_PATH_STATE.getBits<PREVIOUS_SUB_PATH_INDEX, POOL_INDEX_BITS>();
}

void GraphPath::setHasStateMax(bool hasMax) {
  HAS_STATE_MAX_STATE.setBits<HAS_STATE_MAX_INDEX, HAS_STATE_MAX_BITS>(hasMax ? 1 : 0);
}

bool GraphPath::hasStateMax() const {
  return HAS_STATE_MAX_STATE.meetsCondition<HAS_STATE_MAX_INDEX, HAS_STATE_MAX_BITS>(1);
}

char GraphPath::getCurrentVertex() const {
  return CURRENT_VERTEX_STATE.getBits<CURRENT_VERTEX_INDEX, CURRENT_VERTEX_BITS>();
}

bool GraphPath::isExhausted() const {
  return getFocusedSubPath() == 0;
}

unsigned char GraphPath::getMinimumEndDistance() const {
  return MINIMUM_END_DISTANCE_STATE.getBits<MINIMUM_END_DISTANCE_INDEX, DISTANCE_BITS>();
}

void GraphPath::maybeSetBestEndDistance(GraphPathPool *pool, unsigned char distance) {
  if (distance < getBestEndDistance()) {
    BEST_END_DISTANCE_STATE.setBits<BEST_END_DISTANCE_INDEX, DISTANCE_BITS>(distance);

    auto parentPathIndex = PARENT_PATH_STATE.getBits<PARENT_PATH_INDEX, POOL_INDEX_BITS>();
    if (parentPathIndex > 0) {
      pool->getGraphPath(parentPathIndex)->maybeSetBestEndDistance(pool, distance);
    }
  }
}

unsigned char GraphPath::getBestEndDistance() const {
  return BEST_END_DISTANCE_STATE.getBits<BEST_END_DISTANCE_INDEX, DISTANCE_BITS>();
}

std::vector<char> GraphPath::getRoute(const GraphPathPool *pool) const {
  auto parentPathIndex = PARENT_PATH_STATE.getBits<PARENT_PATH_INDEX, POOL_INDEX_BITS>();

  if (parentPathIndex == 0) {
    return std::vector<char>{getCurrentVertex()};
  } else {
    auto partialRoute = pool->getGraphPath(parentPathIndex)->getRoute(pool);
    partialRoute.push_back(getCurrentVertex());
    return partialRoute;
  }
}

void GraphPath::serialize(std::ostream &outstream) const {
  outstream.write(reinterpret_cast<const char *>(&stateA), sizeof(stateA));
  outstream.write(reinterpret_cast<const char *>(&stateB), sizeof(stateB));
}

void GraphPath::deserialize(std::istream &instream) {
  instream.read(reinterpret_cast<char *>(&stateA), sizeof(stateA));
  instream.read(reinterpret_cast<char *>(&stateB), sizeof(stateB));
}

void GraphPath::cleanUp() {
  stateA.clear();
  stateB.clear();
}

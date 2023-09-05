#include "skipper-skeeto-path-finder/graph_path_pool.h"

unsigned long int GraphPathPool::generateNewIndex() {
  return nextAvailableIndex++;
}

GraphPath *GraphPathPool::getGraphPath(unsigned long int index) {
  return &pool->at(index);
}

const GraphPath *GraphPathPool::getGraphPath(unsigned long int index) const {
  return &pool->at(index);
}

bool GraphPathPool::isFull() const {
  return nextAvailableIndex >= pool->size();
}

void GraphPathPool::serialize(std::ostream &outstream) const {
  for (unsigned long int index = 1; index < nextAvailableIndex; ++index) {
    pool->at(index).serialize(outstream);
  }
}

void GraphPathPool::serializeAndClear(std::ostream &outstream) {
  for (unsigned long int index = 1; index < nextAvailableIndex; ++index) {
    auto &path = pool->at(index);
    path.serialize(outstream);
    path.cleanUp();
  }
}

void GraphPathPool::deserialize(std::istream &instream, bool clearWaitingState) {
  reset();
  while (instream.peek() != EOF) {
    auto &path = pool->at(generateNewIndex());
    path.deserialize(instream);

#ifdef FOUND_BEST_DISTANCE
    if (clearWaitingState) {
      path.setIsWaitingForResult(false);
    }
#endif // FOUND_BEST_DISTANCE
  }
}

void GraphPathPool::reset() {
  nextAvailableIndex = 1;
}

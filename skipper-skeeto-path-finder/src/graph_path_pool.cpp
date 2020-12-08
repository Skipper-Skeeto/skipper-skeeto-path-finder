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

void GraphPathPool::deserialize(std::istream &instream) {
  nextAvailableIndex = 1;
  while (instream.peek() != EOF) {
    pool->at(generateNewIndex()).deserialize(instream);
  }
}

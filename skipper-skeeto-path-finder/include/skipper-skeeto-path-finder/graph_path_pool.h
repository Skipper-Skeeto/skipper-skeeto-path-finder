#pragma once

#include "skipper-skeeto-path-finder/graph_path.h"
#include "skipper-skeeto-path-finder/info.h"

#include <array>
#include <memory>

class GraphPathPool {
public:
  unsigned long int generateNewIndex();

  GraphPath *getGraphPath(unsigned long int index);

  const GraphPath *getGraphPath(unsigned long int index) const;

  bool isFull() const;

  void serialize(std::ostream &outstream) const;

  void deserialize(std::istream &instream);

  void reset();

private:
  std::unique_ptr<std::array<GraphPath, POOL_COUNT>> pool = std::make_unique<std::array<GraphPath, POOL_COUNT>>();
  unsigned long int nextAvailableIndex = 1; // 0 index in graph paths means "nullptr"
};

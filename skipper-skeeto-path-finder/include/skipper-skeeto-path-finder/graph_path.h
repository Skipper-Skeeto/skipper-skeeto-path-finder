#pragma once

#include "skipper-skeeto-path-finder/state.h"

#include <iostream>
#include <vector>

class GraphPathPool;

class GraphPath {
public:
  static const unsigned char MAX_DISTANCE;

  static void swap(GraphPath *pathA, GraphPath *pathB);

  void initialize(char vertexIndex, unsigned long int parentPathIndex, unsigned char minimumEndDistance);

  char getCurrentVertex() const;

  void updateFocusedSubPath(unsigned long int index, unsigned char iterationCount);

  void setParentPath(unsigned long int index);

  unsigned long int getParentPath() const;

  unsigned long int getFocusedSubPath() const;

  unsigned char getSubPathIterationCount() const;

  bool hasSetSubPath() const;

  void setNextPath(unsigned long int index);

  unsigned long int getNextPath() const;

  void setPreviousPath(unsigned long int index);

  unsigned long int getPreviousPath() const;

  void setHasStateMax(bool hasMax);

  bool hasStateMax() const;

  bool isExhausted() const;

  unsigned char getMinimumEndDistance() const;

  void maybeSetBestEndDistance(GraphPathPool *pool, unsigned char distance);

  bool hasFoundDistance() const;

  unsigned char getBestEndDistance() const;

  std::vector<char> getRoute(const GraphPathPool *pool) const;

  void serialize(std::ostream &outstream) const;

  void deserialize(std::istream &instream);

  void cleanUp();

  bool isClean() const;

  GraphPath() = default;

private:
  State stateA{};
  State stateB{};
};
#pragma once

#include "skipper-skeeto-path-finder/state.h"

#include <iostream>
#include <vector>

class GraphPathPool;

class GraphPath {
public:
  void initialize(char vertexIndex, unsigned long int parentPathIndex, const GraphPath *parentPath, char extraDistance);

  void initializeAsCopy(const GraphPath *sourcePath, unsigned long int parentPathIndex);

  char getCurrentVertex() const;

  void setFocusedSubPath(unsigned long int index);

  unsigned long int getFocusedSubPath() const;

  bool hasSetSubPath() const;

  void setNextPath(unsigned long int index);

  unsigned long int getNextPath() const;

  void setPreviousPath(unsigned long int index);

  unsigned long int getPreviousPath() const;

  void setHasStateMax(bool hasMax);

  bool hasStateMax() const;

  bool isExhausted() const;

  unsigned char getDistance() const;

  void maybeSetBestEndDistance(GraphPathPool *pool, unsigned char distance);

  unsigned char getBestEndDistance() const;

  std::vector<char> getRoute(const GraphPathPool *pool) const;

  void serialize(std::ostream &outstream) const;

  void deserialize(std::istream &instream);

  void cleanUp();

  GraphPath() = default;

private:
  State state{};
  unsigned char bestEndDistance = 255;
  unsigned long int parentPathIndex = 0;
  unsigned long int previousPathIndex = 0;
  unsigned long int nextPathIndex = 0;
  unsigned long int focusedSubPathIndex = 0;
};
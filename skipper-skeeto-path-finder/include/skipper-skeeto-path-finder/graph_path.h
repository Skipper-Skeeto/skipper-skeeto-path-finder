#pragma once

#include <iostream>
#include <vector>

using State = unsigned long long int;

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

  bool isFinished() const;

  State getUniqueState() const;

  unsigned char getDistance() const;

  State getVisitedVerticesState() const;

  bool meetsCondition(const State &condition) const;

  bool hasVisitedVertex(char vertexIndex) const;

  std::vector<char> getRoute(const GraphPathPool *pool) const;

  void serialize(std::ostream &outstream) const;

  void deserialize(std::istream &instream);

  void cleanUp();

  GraphPath() = default;

private:
  void setCurrentVertex(char vertexIndex);

  template <size_t StartIndex, size_t Count>
  void setState(State newState) {
    state &= (~(((1ULL << Count) - 1) << StartIndex));
    state |= (newState << StartIndex);
  };

  template <size_t StartIndex, size_t Count>
  State getState() const {
    return (state >> StartIndex) & ((1ULL << Count) - 1);
  };

  State state{};
  unsigned long int parentPathIndex = 0;
  unsigned long int previousPathIndex = 0;
  unsigned long int nextPathIndex = 0;
  unsigned long int focusedSubPathIndex = 0;
};
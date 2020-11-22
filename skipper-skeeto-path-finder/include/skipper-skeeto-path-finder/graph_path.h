#pragma once

#include <iostream>
#include <vector>

using State = unsigned long long int;

class GraphPath {
public:
  GraphPath(char startVertexIndex);

  GraphPath(char vertexIndex, GraphPath *previousPath, char extraDistance);

  void initialize(const std::vector<GraphPath> &paths);

  char getCurrentVertex() const;

  bool isInitialized() const;

  bool isExhausted() const;

  bool isFinished() const;

  unsigned char getDistance() const;

  State getState() const;

  bool meetsCondition(const State &condition) const;

  bool hasVisitedVertex(char vertexIndex) const;

  std::vector<GraphPath>::iterator getFocusedNextPath();

  void eraseNextPath(const std::vector<GraphPath>::iterator &pathIterator);

  void bumpFocusedNextPath();

  int getNextPathsCount() const;

  std::vector<char> getRoute() const;

  void serialize(std::ostream &outstream) const;

  void deserialize(std::istream &instream, const GraphPath *previousPath);

  void cleanUp();

private:
  static const State ALL_VERTICES_STATE_MASK;

  GraphPath() = default;

  void setCurrentVertex(char vertexIndex);

  unsigned char distance = 0;
  State state{};

  const GraphPath *previousPath = nullptr;
  std::vector<GraphPath> nextPaths;
  char focusedNextPathIndex = -1; // -1 means not initialized
};
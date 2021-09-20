#pragma once

#include "skipper-skeeto-path-finder/graph_common_state.h"

class GraphData;
class GraphPathPool;

class GraphController {
public:
  GraphController(const GraphData *data, const std::string &resultDir);

  void start();

  std::vector<std::array<char, VERTICES_COUNT>> getResult() const;

private:
  struct PathReferences {
    GraphPath *parentPath = nullptr;
    GraphPath *nextPath = nullptr;
    GraphPath *previousPath = nullptr;
    std::vector<GraphPath *> subPaths;
  };

  static const unsigned long long int ALL_VERTICES_STATE_MASK;

  void setupStartRunner();

  bool moveOnDistributed(GraphPathPool *pool, RunnerInfo *runnerInfo, unsigned long int pathIndex, GraphPath *path, unsigned long long int visitedVerticesState, int depth, bool forceFinish);

  bool initializePath(GraphPathPool *pool, unsigned long int pathIndex, GraphPath *path, unsigned long long int visitedVerticesState, int depth);

  bool meetsCondition(unsigned long long int visitedVerticesState, unsigned long long int condition);

  bool hasVisitedVertex(unsigned long long int visitedVerticesState, unsigned char vertexIndex);

  unsigned char sortSubPaths(GraphPathPool *pool, unsigned long int pathIndex, GraphPath *path);

  void logRemovedSubPaths(GraphPathPool *pool, GraphPath *path, int depth);

  void splitAndRemove(GraphPathPool *pool, RunnerInfo *runnerInfo);

  unsigned long int groupPathTree(GraphPathPool *pool, unsigned long int originalPathIndex);

  std::string getPoolFileName(unsigned int runnerInfoIdentifier) const;

  bool serializePool(GraphPathPool *pool, RunnerInfo *runnerInfo, bool clearSerializedPaths);

  bool serializePool(GraphPathPool *pool, unsigned int runnerInfoIdentifier, bool clearSerializedPaths);

  void deserializePool(GraphPathPool *pool, RunnerInfo *runnerInfo);

  void deletePoolFile(const RunnerInfo *runnerInfo) const;

  void swap(GraphPathPool *pool, unsigned long int pathIndexA, unsigned long int pathIndexB);

  PathReferences getReferences(GraphPathPool *pool, const GraphPath *path) const;

  void setNewIndex(GraphPathPool *pool, const PathReferences &pathReferences, unsigned long int newIndex);

  void printAndDump();

  GraphCommonState commonState;
  const GraphData *data;
};
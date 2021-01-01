#pragma once

#include "skipper-skeeto-path-finder/graph_common_state.h"
#include "skipper-skeeto-path-finder/graph_path_pool.h"

class GraphData;

class GraphController {
public:
  GraphController(const GraphData *data);

  void start();

private:
  static const char *MEMORY_DUMP_DIR;
  static const unsigned long long int ALL_VERTICES_STATE_MASK;

  void setupStartRunner();

  bool moveOnDistributed(GraphPathPool *pool, RunnerInfo *runnerInfo, unsigned long int pathIndex, GraphPath *path, unsigned long long int visitedVerticesState, int depth);

  bool initializePath(GraphPathPool *pool, unsigned long int pathIndex, GraphPath *path, unsigned long long int visitedVerticesState, int depth);

  bool meetsCondition(unsigned long long int visitedVerticesState, unsigned long long int condition);

  bool hasVisitedVertex(unsigned long long int visitedVerticesState, unsigned char vertexIndex);

  unsigned char sortSubPaths(GraphPathPool *pool, unsigned long int pathIndex, GraphPath *path);

  void logRemovedSubPaths(GraphPathPool *pool, GraphPath *path, int depth);

  void splitAndRemove(GraphPathPool *pool, RunnerInfo *runnerInfo);

  std::pair<unsigned long int, GraphPath *> movePathData(GraphPathPool *sourcePool, GraphPathPool *destinationPool, unsigned long int sourcePathIndex, unsigned long int destinationParentPathIndex);

  std::string getPoolFileName(unsigned int runnerInfoIdentifier) const;

  void serializePool(GraphPathPool *pool, RunnerInfo *runnerInfo);

  void serializePool(GraphPathPool *pool, unsigned int runnerInfoIdentifier);

  void deserializePool(GraphPathPool *pool, RunnerInfo *runnerInfo);

  void deletePoolFile(const RunnerInfo *runnerInfo) const;

  GraphCommonState commonState;
  const GraphData *data;
  std::string resultDirName;

  mutable std::mutex tempPoolMutex;
  GraphPathPool tempPool;
};
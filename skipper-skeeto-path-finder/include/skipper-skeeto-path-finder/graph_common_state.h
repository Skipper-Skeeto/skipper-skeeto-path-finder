#pragma once

#include "skipper-skeeto-path-finder/graph_path.h"
#include "skipper-skeeto-path-finder/info.h"
#include "skipper-skeeto-path-finder/memory_mapped_file_allocator.h"
#include "skipper-skeeto-path-finder/runner_info.h"

#include <parallel_hashmap/phmap.h>

#include <array>
#include <list>
#include <string>
#include <vector>

#define LOG_PATH_COUNT_MAX 10

class GraphPathPool;

class GraphCommonState {
public:
  GraphCommonState(const std::string &resultDir, int maxActiveRunners);

  bool makesSenseToInitialize(const RunnerInfo *runnerInfo, const GraphPath *path) const;

  bool makesSenseToKeep(const RunnerInfo *runnerInfo, GraphPath *path, unsigned long long int visitedVerticesState);

  void handleFinishedPath(const GraphPathPool *pool, RunnerInfo *runnerInfo, const GraphPath *path);

  void updateLocalMax(RunnerInfo *runnerInfo);

  void dumpGoodOnes();

  std::pair<std::vector<std::array<char, VERTICES_COUNT>>, int> getGoodOnes() const;

  void printStatus();

  void addRunnerInfos(std::list<RunnerInfo> runnerInfos);

  RunnerInfo *getNextRunnerInfo(RunnerInfo *currentInfo, bool preferBest);

  void removeActiveRunnerInfo(RunnerInfo *runnerInfo);

  void splitAndRemoveActiveRunnerInfo(RunnerInfo *parentRunnerInfo, std::list<RunnerInfo> childRunnerInfos);

  int runnerInfoCount() const;

  void registerAddedPaths(int depth, int count);

  void registerStartedPath(int depth);

  void registerRemovedPath(int depth);

  bool appliesForLogging(int depth) const;

  void registerPoolDumpFailed(int runnerInfoIdentifier);

  bool shouldStop() const;

  bool shouldPause(int index) const;

  void setMaxActiveRunners(int count);

private:
  bool checkForDuplicateState(GraphPath *path, unsigned long long int visitedVerticesState);

  const std::string resultDir;

  mutable std::mutex finalStateMutex;
  mutable std::mutex runnerInfoMutex;
  mutable std::mutex stopMutex;
  mutable std::mutex maxActiveRunnersMutex;
  mutable std::mutex pathCountMutex;
  mutable std::mutex printMutex;

  unsigned char maxDistance = GraphPath::MAX_DISTANCE;
  phmap::parallel_flat_hash_map<
      unsigned long long int,
      unsigned char,
      phmap::priv::hash_default_hash<unsigned long long int>,
      phmap::priv::hash_default_eq<unsigned long long int>,
      MemoryMappedFileAllocator<phmap::priv::Pair<const unsigned long long int, unsigned char>>, // alias for std::allocator
      6,                                                                                         // 2^N submaps, default is 4
      std::mutex>
      distanceForState{};
  std::vector<std::array<char, VERTICES_COUNT>> goodOnes;
  int dumpedGoodOnes = 0;

  std::list<RunnerInfo> activeRunners;
  std::list<RunnerInfo> passiveRunners;

  bool stopping = false;
  int maxActiveRunners;

  std::array<int, LOG_PATH_COUNT_MAX> addedPathsCount{};
  std::array<int, LOG_PATH_COUNT_MAX> startedPathsCount{};
  std::array<int, LOG_PATH_COUNT_MAX> splittedPathsCount{};
  std::array<int, LOG_PATH_COUNT_MAX> removedPathsCount{};
};

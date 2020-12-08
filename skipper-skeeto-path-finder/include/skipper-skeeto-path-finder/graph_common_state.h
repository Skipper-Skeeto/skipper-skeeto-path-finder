#pragma once

#include "skipper-skeeto-path-finder/graph_path.h"
#include "skipper-skeeto-path-finder/info.h"
#include "skipper-skeeto-path-finder/runner_info.h"

#include <parallel_hashmap/phmap.h>

#include <array>
#include <list>
#include <string>
#include <vector>

class GraphPathPool;

class GraphCommonState {
public:
  bool makesSenseToInitialize(const GraphPath *path) const;

  bool makesSenseToKeep(GraphPath *path);

  void maybeAddNewGoodOne(const GraphPathPool *pool, RunnerInfo *runnerInfo, const GraphPath *path);

  void dumpGoodOnes(const std::string &dirName);

  void printStatus();

  void addRunnerInfos(std::vector<RunnerInfo> runnerInfos);

  RunnerInfo *getNextRunnerInfo(RunnerInfo *currentInfo, bool preferBest);

  void removeActiveRunnerInfo(RunnerInfo *runnerInfo);

  void splitAndRemoveActiveRunnerInfo(RunnerInfo *parentRunnerInfo, std::vector<RunnerInfo> childRunnerInfos);

  int runnerInfoCount() const;

private:
  static const char *DUMPED_GOOD_ONES_BASE_DIR;

  unsigned char getMaxDistance() const;

  bool checkForDuplicateState(GraphPath *path);

  mutable std::mutex finalStateMutex;
  mutable std::mutex distanceStateMutex;
  mutable std::mutex runnerInfoMutex;
  mutable std::mutex printMutex;

  unsigned char maxDistance = 255;
  phmap::parallel_flat_hash_map<State, unsigned char> distanceForState{};
  std::vector<std::array<char, VERTICES_COUNT>> goodOnes;
  int dumpedGoodOnes = 0;

  std::list<RunnerInfo> activeRunners;
  std::list<RunnerInfo> passiveRunners;
};

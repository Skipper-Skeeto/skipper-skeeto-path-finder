#pragma once

#include "skipper-skeeto-path-finder/graph_path.h"
#include "skipper-skeeto-path-finder/info.h"
#include "skipper-skeeto-path-finder/thread_info.h"

#include <parallel_hashmap/phmap.h>

#include <array>
#include <list>
#include <string>
#include <vector>

class GraphCommonState {
public:
  bool makesSenseToInitialize(const GraphPath *path) const;

  bool makesSenseToKeep(const GraphPath *path);

  void maybeAddNewGoodOne(const GraphPath *path);

  void dumpGoodOnes(const std::string &dirName);

  void printStatus();

  void addThread(std::thread &&thread, unsigned char identifier);

  ThreadInfo *getCurrentThread();

  void updateThreads();

  bool hasThreads() const;

private:
  static const char *DUMPED_GOOD_ONES_BASE_DIR;

  unsigned char getMaxDistance() const;

  int checkForDuplicateState(const State &state, unsigned char distance);

  mutable std::mutex finalStateMutex;
  mutable std::mutex distanceStateMutex;
  mutable std::mutex threadInfoMutex;
  mutable std::mutex printMutex;

  unsigned char maxDistance = 255;
  phmap::parallel_flat_hash_map<State, unsigned char> distanceForState{};
  std::vector<std::array<char, VERTICES_COUNT>> goodOnes;
  int dumpedGoodOnes = 0;
  int lastRunningExtraThread = -1;

  std::list<ThreadInfo> threadInfos;
};

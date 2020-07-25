#pragma once

#include "skipper-skeeto-path-finder/path.h"
#include "skipper-skeeto-path-finder/thread_info.h"

#include <parallel_hashmap/phmap.h>

#include <array>
#include <chrono>
#include <list>
#include <mutex>
#include <vector>

class SubPath;

class CommonState {
public:
  std::vector<std::vector<const Action *>> getGoodOnes() const;

  bool makesSenseToPerformActions(const Path *originPath, const SubPath *subPath);

  bool makesSenseToStartNewSubPath(const Path *path);

  bool makesSenseToExpandSubPath(const Path *originPath, const SubPath *subPath);

  void addNewGoodOnes(const std::vector<std::vector<const Action *>> &stepsOfSteps, int visitedRoomsCount);

  void printStatus();

  void dumpGoodOnes(const std::string &dirName);

  void addThread(std::thread &&thread, unsigned char identifier);

  ThreadInfo *getCurrentThread();

  void updateThreads();

  bool hasThreads() const;

private:
  static const char *DUMPED_GOOD_ONES_BASE_DIR;

  unsigned char getMaxVisitedRoomsCount() const;

  bool checkForDuplicateState(const State &state, unsigned char visitedRoomsCount);

  size_t getWorkingSetBytes();

  mutable std::mutex finalStateMutex;
  mutable std::mutex stepStageMutex;
  mutable std::mutex threadInfoMutex;
  mutable std::mutex statisticsMutex;
  mutable std::mutex printMutex;
  mutable std::mutex memoryMutex;

  unsigned char maxVisitedRoomsCount = 255;
  phmap::parallel_flat_hash_map<State, unsigned char> stepsForState{};
  std::vector<std::vector<const Action *>> goodOnes;
  int dumpedGoodOnes = 0;
  int lastRunningExtraThread = -1;

  std::list<ThreadInfo> threadInfos;

  int tooManyGeneralStepsCount = 0;
  int tooManyGeneralStepsDepthTotal = 0;
  int tooManyStateStepsCount = 0;
  int tooManyStateStepsDepthTotal = 0;
};

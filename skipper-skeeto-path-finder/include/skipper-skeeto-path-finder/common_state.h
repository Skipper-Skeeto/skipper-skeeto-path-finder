#pragma once

#include "skipper-skeeto-path-finder/path.h"

#include <chrono>
#include <mutex>
#include <unordered_map>

class CommonState {
public:
  std::vector<Path> getGoodOnes() const;

  bool makesSenseToPerformActions(const Path *originPath, const std::vector<const Room *> &subPath);

  bool makesSenseToStartNewSubPath(const Path *path);

  bool makesSenseToExpandSubPath(const Path *originPath, const std::vector<const Room *> &subPath);

  bool submitIfDone(const Path *path);

  void stop();

  bool shouldStop() const;

  void printStatus();

  void dumpGoodOnes(const std::string &dirName);

private:
  static const char *DUMPED_GOOD_ONES_BASE_DIR;

  int getMaxVisitedRoomsCount() const;

  bool checkForDuplicateState(const State &state, const Room *room, int visitedRoomsCount);

  void addNewGoodOne(const Path *path);

  mutable std::mutex finalStateMutex;
  mutable std::mutex stepStageMutex;
  mutable std::mutex statisticsMutex;
  mutable std::mutex stopMutex;
  mutable std::mutex printMutex;

  int maxVisitedRoomsCount = 5000;
  std::unordered_map<const Room *, std::unordered_map<std::vector<bool>, int>> stepsForStage;
  std::vector<Path> goodOnes;
  int dumpedGoodOnes = 0;

  int tooManyGeneralStepsCount = 0;
  int tooManyGeneralStepsDepthTotal = 0;
  int tooManyStateStepsCount = 0;
  int tooManyStateStepsDepthTotal = 0;

  bool stopNow = false;
};

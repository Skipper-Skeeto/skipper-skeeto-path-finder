#pragma once

#include "skipper-skeeto-path-finder/path.h"

#include <chrono>
#include <mutex>
#include <unordered_map>

class CommonState {
public:
  std::vector<Path> getGoodOnes() const;

  bool makesSenseToPerformActions(const Path *path);

  bool makesSenseToMoveOn(const Path *path);

  bool submitIfDone(const Path *path);

  void stop();

  bool shouldStop() const;

  void printStatus();

private:
  int getMaxVisitedRoomsCount() const;

  bool checkForDuplicateState(const Path *path);

  void addNewGoodOne(const Path *path);

  mutable std::mutex finalStateMutex;
  mutable std::mutex stepStageMutex;
  mutable std::mutex statisticsMutex;
  mutable std::mutex stopMutex;
  mutable std::mutex printMutex;

  int maxVisitedRoomsCount = 5000;
  std::unordered_map<const Room *, std::unordered_map<std::vector<bool>, int>> stepsForStage;
  std::vector<Path> goodOnes;

  int tooManyGeneralStepsCount = 0;
  int tooManyGeneralStepsDepthTotal = 0;
  int tooManyStateStepsCount = 0;
  int tooManyStateStepsDepthTotal = 0;

  bool stopNow = false;
};

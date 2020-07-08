#pragma once

#include "skipper-skeeto-path-finder/info.h"
#include "skipper-skeeto-path-finder/path.h"

#include <parallel_hashmap/phmap.h>

#include <array>
#include <chrono>
#include <mutex>
#include <vector>

class SubPath;

class CommonState {
public:
  std::vector<std::vector<const Action *>> getGoodOnes() const;

  bool makesSenseToPerformActions(const Path *originPath, const SubPath *subPath);

  bool makesSenseToStartNewSubPath(const Path *path);

  bool makesSenseToExpandSubPath(const Path *originPath, const SubPath *subPath);

  bool submitIfDone(const Path *path);

  void stop();

  bool shouldStop() const;

  void printStatus();

  void dumpGoodOnes(const std::string &dirName);

private:
  static const char *DUMPED_GOOD_ONES_BASE_DIR;

  unsigned char getMaxVisitedRoomsCount() const;

  bool checkForDuplicateState(const State &state, const Room *room, unsigned char visitedRoomsCount);

  void addNewGoodOne(const Path *path);

  mutable std::mutex finalStateMutex;
  mutable std::mutex stepStageMutex;
  mutable std::mutex statisticsMutex;
  mutable std::mutex stopMutex;
  mutable std::mutex printMutex;

  unsigned char maxVisitedRoomsCount = 255;
  std::array<phmap::parallel_flat_hash_map<State, unsigned char>, ROOM_COUNT> stepsForStage{};
  std::vector<std::vector<const Action *>> goodOnes;
  int dumpedGoodOnes = 0;

  int tooManyGeneralStepsCount = 0;
  int tooManyGeneralStepsDepthTotal = 0;
  int tooManyStateStepsCount = 0;
  int tooManyStateStepsDepthTotal = 0;

  bool stopNow = false;
};

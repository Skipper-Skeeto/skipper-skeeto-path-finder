#include "skipper-skeeto-path-finder/common_state.h"

#include "skipper-skeeto-path-finder/action.h"
#include "skipper-skeeto-path-finder/path.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#endif

const char *CommonState::DUMPED_GOOD_ONES_BASE_DIR = "results";

std::vector<std::vector<const Action *>> CommonState::getGoodOnes() const {
  std::lock_guard<std::mutex> guard(finalStateMutex);
  return goodOnes;
}

bool CommonState::makesSenseToPerformActions(const Path *originPath, const std::vector<const Room *> &subPath) {
  unsigned char visitedRoomsCount = originPath->getVisitedRoomsCount() + subPath.size();

  // Even if count has reached max, the actions might complete the path and thus we won't exceed max
  if (visitedRoomsCount > getMaxVisitedRoomsCount()) {
    std::lock_guard<std::mutex> guard(statisticsMutex);
    tooManyGeneralStepsCount += 1;
    tooManyGeneralStepsDepthTotal += originPath->depth;

    return false;
  }

  if (!checkForDuplicateState(originPath->getState(), subPath.back(), visitedRoomsCount)) {
    std::lock_guard<std::mutex> guard(statisticsMutex);
    tooManyStateStepsCount += 1;
    tooManyStateStepsDepthTotal += originPath->depth;

    return false;
  }

  return true;
}

bool CommonState::makesSenseToStartNewSubPath(const Path *path) {

  // If we move on, the count would exceed max - that's why we check for equality too
  if (path->getVisitedRoomsCount() >= getMaxVisitedRoomsCount()) {
    std::lock_guard<std::mutex> guard(statisticsMutex);
    tooManyGeneralStepsCount += 1;
    tooManyGeneralStepsDepthTotal += path->depth;

    return false;
  }

  if (!checkForDuplicateState(path->getState(), path->getCurrentRoom(), path->getVisitedRoomsCount())) {
    std::lock_guard<std::mutex> guard(statisticsMutex);
    tooManyStateStepsCount += 1;
    tooManyStateStepsDepthTotal += path->depth;

    return false;
  }

  return true;
}

bool CommonState::makesSenseToExpandSubPath(const Path *originPath, const std::vector<const Room *> &subPath) {
  unsigned char visitedRoomsCount = originPath->getVisitedRoomsCount() + subPath.size();

  // If we move on, the count would exceed max - that's why we check for equality too
  if (visitedRoomsCount >= getMaxVisitedRoomsCount()) {
    std::lock_guard<std::mutex> guard(statisticsMutex);
    tooManyGeneralStepsCount += 1;
    tooManyGeneralStepsDepthTotal += originPath->depth;

    return false;
  }

  // Note that we don't check for duplicate state, since it has been done in makesSenseToPerformActions().
  // After that check we only get here if no actions was done - and thus it would be the same state!

  return true;
}

bool CommonState::submitIfDone(const Path *path) {
  if (!path->isDone()) {
    return false;
  }

  addNewGoodOne(path);

  std::lock_guard<std::mutex> guard(printMutex);
  std::cout << "Found new good with " << int(path->getVisitedRoomsCount()) << " rooms (depth " << int(path->depth) << ")" << std::endl;

  return true;
}

void CommonState::stop() {
  std::lock_guard<std::mutex> guardStatistics(stopMutex);
  stopNow = true;
}

bool CommonState::shouldStop() const {
  std::lock_guard<std::mutex> guardStatistics(stopMutex);
  return stopNow;
}

void CommonState::printStatus() {
  std::lock_guard<std::mutex> guardStatistics(statisticsMutex);
  std::lock_guard<std::mutex> guardFinalState(finalStateMutex);
  std::lock_guard<std::mutex> guardStepsStage(stepStageMutex);

  double depthKillGeneralAvg = 0;
  if (tooManyGeneralStepsCount != 0) {
    depthKillGeneralAvg = (double)tooManyGeneralStepsDepthTotal / tooManyGeneralStepsCount;
  }

  double depthKillStateAvg = 0;
  if (tooManyStateStepsCount != 0) {
    depthKillStateAvg = (double)tooManyStateStepsDepthTotal / tooManyStateStepsCount;
  }

  std::lock_guard<std::mutex> guardPrint(printMutex);
  std::cout << "good: " << goodOnes.size() << "; max: " << int(maxVisitedRoomsCount)
            << "; general: " << std::setw(8) << tooManyGeneralStepsCount << "=" << std::fixed << std::setprecision(8) << depthKillGeneralAvg
            << "; state: " << std::setw(8) << tooManyStateStepsCount << "=" << std::fixed << std::setprecision(8) << depthKillStateAvg
            << std::endl;
}

void CommonState::dumpGoodOnes(const std::string &dirName) {
  std::lock_guard<std::mutex> guardFinalState(finalStateMutex);
  if (dumpedGoodOnes >= goodOnes.size()) {
    return;
  }

  int newDumpedCount = goodOnes.size() - dumpedGoodOnes;

  std::string dirPath = std::string(DUMPED_GOOD_ONES_BASE_DIR) + "/" + dirName;
  std::string fileName = dirPath + "/" + std::to_string(maxVisitedRoomsCount) + ".txt";

#ifdef _WIN32
  !CreateDirectoryA(DUMPED_GOOD_ONES_BASE_DIR, nullptr);
  !CreateDirectoryA(dirPath.c_str(), nullptr);
#else
#error "Creating directory was not implemented for this platform"
#endif

  std::ofstream dumpFile(fileName);
  for (int index = 0; index < goodOnes.size(); ++index) {
    dumpFile << "PATH #" << index + 1 << "(of " << goodOnes.size() << "):" << std::endl;
    for (const auto &action : goodOnes[index]) {
      dumpFile << action->getStepDescription() << std::endl;
    }

    dumpFile << std::endl
             << std::endl;
  }

  dumpedGoodOnes = goodOnes.size();

  std::lock_guard<std::mutex> guardPrint(printMutex);
  std::cout << "Dumped " << newDumpedCount << " new good one(s) to " << fileName << std::endl;
}

unsigned char CommonState::getMaxVisitedRoomsCount() const {
  std::lock_guard<std::mutex> guard(finalStateMutex);
  return maxVisitedRoomsCount;
}

bool CommonState::checkForDuplicateState(const State &state, const Room *room, unsigned char visitedRoomsCount) {
  std::lock_guard<std::mutex> guard(stepStageMutex);
  auto &roomStates = stepsForStage[room];
  auto stepsIterator = roomStates.find(state);
  if (stepsIterator != roomStates.end()) {

    // TODO: If directions etc. ever gets to be weighted (e.g. left->left vs. left->up), this should maybe be count < count
    if (stepsIterator->second <= visitedRoomsCount) {
      return false;
    }
  }

  // TODO: Doesn't need to be set if count == count
  roomStates[state] = visitedRoomsCount;

  return true;
}

void CommonState::addNewGoodOne(const Path *path) {
  std::lock_guard<std::mutex> guard(finalStateMutex);
  if (path->getVisitedRoomsCount() < maxVisitedRoomsCount) {
    maxVisitedRoomsCount = path->getVisitedRoomsCount();
    goodOnes.clear();
    dumpedGoodOnes = 0;
  }

  goodOnes.push_back(path->getSteps());
}

#include "skipper-skeeto-path-finder/common_state.h"

#include <iomanip>
#include <iostream>

std::vector<Path> CommonState::getGoodOnes() const {
  std::lock_guard<std::mutex> guard(finalStateMutex);
  return goodOnes;
}

bool CommonState::makesSenseToPerformActions(const Path *path) {

  // Even if count has reached max, the actions might complete the path and thus we won't exceed max
  if (path->getVisitedRoomsCount() > getMaxVisitedRoomsCount()) {
    std::lock_guard<std::mutex> guard(statisticsMutex);
    tooManyGeneralStepsCount += 1;
    tooManyGeneralStepsDepthTotal += path->depth;

    return false;
  }

  if (!checkForDuplicateState(path)) {
    std::lock_guard<std::mutex> guard(statisticsMutex);
    tooManyStateStepsCount += 1;
    tooManyStateStepsDepthTotal += path->depth;

    return false;
  }

  return true;
}

bool CommonState::makesSenseToMoveOn(const Path *path) {

  // If we move on, the count would exceed max - that's why we check for equality too
  if (path->getVisitedRoomsCount() >= getMaxVisitedRoomsCount()) {
    std::lock_guard<std::mutex> guard(statisticsMutex);
    tooManyGeneralStepsCount += 1;
    tooManyGeneralStepsDepthTotal += path->depth;

    return false;
  }

  if (!checkForDuplicateState(path)) {
    std::lock_guard<std::mutex> guard(statisticsMutex);
    tooManyStateStepsCount += 1;
    tooManyStateStepsDepthTotal += path->depth;

    return false;
  }

  return true;
}

bool CommonState::submitIfDone(const Path *path) {
  if (!path->isDone()) {
    return false;
  }

  addNewGoodOne(path);

  std::lock_guard<std::mutex> guard(printMutex);
  std::cout << "Found new good with " << path->getVisitedRoomsCount() << " rooms (depth " << path->depth << ")" << std::endl;

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
  std::cout << "good: " << goodOnes.size() << "; max: " << maxVisitedRoomsCount
            << "; general: " << std::setw(8) << tooManyGeneralStepsCount << "=" << std::fixed << std::setprecision(8) << depthKillGeneralAvg
            << "; state: " << std::setw(8) << tooManyStateStepsCount << "=" << std::fixed << std::setprecision(8) << depthKillStateAvg
            << std::endl;
}

int CommonState::getMaxVisitedRoomsCount() const {
  std::lock_guard<std::mutex> guard(finalStateMutex);
  return maxVisitedRoomsCount;
}

bool CommonState::checkForDuplicateState(const Path *path) {

  auto state = path->getState();

  std::lock_guard<std::mutex> guard(stepStageMutex);
  auto &roomStates = stepsForStage[path->getCurrentRoom()];
  auto stepsIterator = roomStates.find(state);
  if (stepsIterator != roomStates.end()) {

    // TODO: If direction ever get an impact (e.g. double-left is good), this might has to check count <= count
    if (stepsIterator->second.first < path->getVisitedRoomsCount()) {
      return false;
    }
  }

  // TODO: Doesn't need to be set if count == count
  roomStates[state] = std::make_pair(path->getVisitedRoomsCount(), path->depth);

  return true;
}

void CommonState::addNewGoodOne(const Path *path) {
  std::lock_guard<std::mutex> guard(finalStateMutex);
  if (path->getVisitedRoomsCount() < maxVisitedRoomsCount) {
    maxVisitedRoomsCount = path->getVisitedRoomsCount();
    goodOnes.clear();
  }

  goodOnes.push_back(*path);
}

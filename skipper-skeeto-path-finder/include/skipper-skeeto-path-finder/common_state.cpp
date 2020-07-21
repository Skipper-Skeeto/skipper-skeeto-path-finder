#include "skipper-skeeto-path-finder/common_state.h"

#include "skipper-skeeto-path-finder/action.h"
#include "skipper-skeeto-path-finder/path.h"
#include "skipper-skeeto-path-finder/room.h"
#include "skipper-skeeto-path-finder/sub_path.h"

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

bool CommonState::makesSenseToPerformActions(const Path *originPath, const SubPath *subPath) {
  unsigned char visitedRoomsCount = originPath->getVisitedRoomsCount() + subPath->visitedRoomsCount();

  // Even if count has reached max, the actions might complete the path and thus we won't exceed max
  if (visitedRoomsCount > getMaxVisitedRoomsCount()) {
    std::lock_guard<std::mutex> guard(statisticsMutex);
    tooManyGeneralStepsCount += 1;
    tooManyGeneralStepsDepthTotal += originPath->depth;

    return false;
  }

  auto newState = Path::getStateWithRoom(originPath->getState(), subPath->getLastRoom());
  if (!checkForDuplicateState(newState, visitedRoomsCount)) {
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

  if (!checkForDuplicateState(path->getState(), path->getVisitedRoomsCount())) {
    std::lock_guard<std::mutex> guard(statisticsMutex);
    tooManyStateStepsCount += 1;
    tooManyStateStepsDepthTotal += path->depth;

    return false;
  }

  return true;
}

bool CommonState::makesSenseToExpandSubPath(const Path *originPath, const SubPath *subPath) {
  unsigned char visitedRoomsCount = originPath->getVisitedRoomsCount() + subPath->visitedRoomsCount();

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

void CommonState::printStatus(const std::vector<unsigned char> &runningThreads) {
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
  std::cout << "Threads (" << runningThreads.size() << "): ";
  for (auto identifier : runningThreads) {
    std::cout << identifier;
  }

  std::cout << std::endl
            << "good: " << goodOnes.size() << "; max: " << int(maxVisitedRoomsCount)
            << "; general: " << std::setw(8) << tooManyGeneralStepsCount << "=" << std::fixed << std::setprecision(8) << depthKillGeneralAvg
            << "; state: " << std::setw(8) << tooManyStateStepsCount << "=" << std::fixed << std::setprecision(8) << depthKillStateAvg
            << std::endl;

  std::cout << "Paths for depths (1/2): ";
  auto totalFinished = SubPathInfo::getTotalAndFinishedPathsCount();
  for (int depth = 0; depth < totalFinished.size(); ++depth) {
    int total = totalFinished[depth].first;
    int finished = totalFinished[depth].second;

    if (depth == totalFinished.size() / 2) {
      std::cout << std::endl
                << "Paths for depths (2/2): ";
    }

    if (total > 0) {
      std::cout << depth << ": " << finished << "/" << total << " (" << std::setprecision(2) << (double)finished * 100 / total << "%); ";
    }
  }
  std::cout << std::endl;
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
  CreateDirectoryA(DUMPED_GOOD_ONES_BASE_DIR, nullptr);
  CreateDirectoryA(dirPath.c_str(), nullptr);
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

bool CommonState::checkForDuplicateState(const State &state, unsigned char visitedRoomsCount) {
  std::lock_guard<std::mutex> guard(stepStageMutex);
  auto stepsIterator = stepsForState.find(state);
  if (stepsIterator != stepsForState.end()) {

    // TODO: If directions etc. ever gets to be weighted (e.g. left->left vs. left->up), this should maybe be count < count
    if (stepsIterator->second <= visitedRoomsCount) {
      return false;
    }
  }

  // TODO: Doesn't need to be set if count == count
  stepsForState[state] = visitedRoomsCount;

  return true;
}

void CommonState::addNewGoodOnes(const std::vector<std::vector<const Action *>> &stepsOfSteps, int visitedRoomsCount) {
  std::lock_guard<std::mutex> guardFinalState(finalStateMutex);
  if (visitedRoomsCount < maxVisitedRoomsCount) {
    maxVisitedRoomsCount = visitedRoomsCount;
    goodOnes.clear();
    dumpedGoodOnes = 0;
  }

  for (const auto &steps : stepsOfSteps) {
    goodOnes.push_back(steps);
  }

  std::lock_guard<std::mutex> guardPrint(printMutex);
  std::cout << "Found " << stepsOfSteps.size() << " new good one(s) with " << visitedRoomsCount << " rooms" << std::endl;
}
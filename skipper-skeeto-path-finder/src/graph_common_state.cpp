#include "skipper-skeeto-path-finder/graph_common_state.h"

#include "skipper-skeeto-path-finder/file_helper.h"
#include "skipper-skeeto-path-finder/graph_path.h"
#include "skipper-skeeto-path-finder/memory_mapped_file_pool.h"

#include <fstream>
#include <iomanip>
#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#include <psapi.h>
#endif

const char *GraphCommonState::DUMPED_GOOD_ONES_BASE_DIR = "results";

GraphCommonState::GraphCommonState(const std::string &tempDir)
    : distanceForState((MemoryMappedFilePool::setAllocationDir(tempDir), MemoryMappedFileAllocator<phmap::priv::Pair<const unsigned long long int, unsigned char>>())) {
}

bool GraphCommonState::makesSenseToInitialize(const RunnerInfo *runnerInfo, const GraphPath *path) const {
  // If we're sure we won't get a better one, there's no reason to start at all
  return path->getMinimumEndDistance() < runnerInfo->getLocalMaxDistance();
}

bool GraphCommonState::makesSenseToKeep(const RunnerInfo *runnerInfo, GraphPath *path, unsigned long long int visitedVerticesState) {
  if (path->getMinimumEndDistance() >= runnerInfo->getLocalMaxDistance()) {
    // If we're sure we won't get a better one, there's no reason to keep it
    return false;
  }

  return checkForDuplicateState(path, visitedVerticesState);
}

void GraphCommonState::handleFinishedPath(const GraphPathPool *pool, RunnerInfo *runnerInfo, const GraphPath *path) {
  auto distance = path->getMinimumEndDistance();

  if (distance < runnerInfo->getHighScore()) {
    runnerInfo->setHighScore(distance);
  }

  std::lock_guard<std::mutex> guardFinalState(finalStateMutex);
  if (distance > maxDistance) {
    return;
  }

  if (distance < maxDistance) {
    maxDistance = distance;
    goodOnes.clear();
    dumpedGoodOnes = 0;
  }

  auto runnerRoute = runnerInfo->getRoute();
  auto pathRoute = path->getRoute(pool);

  std::array<char, VERTICES_COUNT> route;
  for (unsigned char index = 0; index < route.size(); ++index) {
    if (index < runnerRoute.size()) {
      route[index] = runnerRoute[index];
    } else {
      route[index] = pathRoute[index - runnerRoute.size()];
    }
  }

  goodOnes.push_back(route);

  std::lock_guard<std::mutex> guardPrint(printMutex);
  std::cout << "Found new good one with distance " << +distance << " in runner " << runnerInfo->getIdentifier() << std::endl;
}

void GraphCommonState::updateLocalMax(RunnerInfo *runnerInfo) {
  std::lock_guard<std::mutex> guard(finalStateMutex);
  runnerInfo->setLocalMaxDistance(maxDistance);
}

void GraphCommonState::dumpGoodOnes(const std::string &dirName) {
  std::lock_guard<std::mutex> guardFinalState(finalStateMutex);
  if (dumpedGoodOnes >= goodOnes.size()) {
    return;
  }

  int newDumpedCount = goodOnes.size() - dumpedGoodOnes;

  std::string dirPath = std::string(DUMPED_GOOD_ONES_BASE_DIR) + "/" + dirName;
  std::string fileName = dirPath + "/" + std::to_string(maxDistance) + ".txt";

  FileHelper::createDir(DUMPED_GOOD_ONES_BASE_DIR);
  FileHelper::createDir(dirPath.c_str());

  std::ofstream dumpFile(fileName);
  for (int index = 0; index < goodOnes.size(); ++index) {
    dumpFile << "PATH #" << index + 1 << "(of " << goodOnes.size() << "):" << std::endl;
    for (const auto &vertexIndex : goodOnes[index]) {
      dumpFile << +vertexIndex << std::endl;
    }

    dumpFile << std::endl
             << std::endl;
  }

  dumpedGoodOnes = goodOnes.size();

  std::lock_guard<std::mutex> guardPrint(printMutex);
  std::cout << "Dumped " << newDumpedCount << " new good one(s) to " << fileName << std::endl;
}

void GraphCommonState::printStatus() {
  std::lock_guard<std::mutex> runnerInfoGuard(runnerInfoMutex);
  std::lock_guard<std::mutex> guardFinalState(finalStateMutex);
  std::lock_guard<std::mutex> guardPathCount(pathCountMutex);
  std::lock_guard<std::mutex> guardPrint(printMutex);

  std::cout << std::endl;

  std::cout << "Status: " << (passiveRunners.size() + activeRunners.size()) << " runners, found " << goodOnes.size() << " at distance " << +maxDistance << std::endl;

  std::cout << "Active runners:";
  for (const auto &info : activeRunners) {
    std::cout << " " << info.getIdentifier() << " (" << +info.getHighScore() << ")";
  }
  std::cout << std::endl;

  std::cout << "Paths [ Removed" << std::setw(25) << "| Splitted" << std::setw(24) << "| Started" << std::setw(23) << "| Added" << std::setw(6) << "]" << std::endl;

  for (int index = 0; index < LOG_PATH_COUNT_MAX; ++index) {
    std::cout << " - " << std::setw(2) << index << std::fixed << std::setprecision(2) << " ["
              << std::setw(12) << removedPathsCount[index] << " (" << std::setw(6) << (addedPathsCount[index] > 0 ? (double)removedPathsCount[index] * 100 / addedPathsCount[index] : 0) << "%) | "
              << std::setw(12) << splittedPathsCount[index] << " (" << std::setw(6) << (addedPathsCount[index] > 0 ? (double)splittedPathsCount[index] * 100 / addedPathsCount[index] : 0) << "%) | "
              << std::setw(12) << startedPathsCount[index] << " (" << std::setw(6) << (addedPathsCount[index] > 0 ? (double)startedPathsCount[index] * 100 / addedPathsCount[index] : 0) << "%) | "
              << std::setw(10) << addedPathsCount[index] << "]" << std::endl;
  }

  std::cout << std::endl;
}

void GraphCommonState::addRunnerInfos(std::list<RunnerInfo> runnerInfos) {
  std::lock_guard<std::mutex> runnerInfoGuard(runnerInfoMutex);
  std::lock_guard<std::mutex> guardPrint(printMutex);

  std::cout << "Adding runners:";
  for (const auto &info : runnerInfos) {
    std::cout << " " << info.getIdentifier();
  }
  std::cout << std::endl;

  passiveRunners.splice(passiveRunners.end(), runnerInfos);
}

RunnerInfo *GraphCommonState::getNextRunnerInfo(RunnerInfo *currentInfo, bool preferBest) {
  std::lock_guard<std::mutex> runnerInfoGuard(runnerInfoMutex);

  if (passiveRunners.empty()) {
    return currentInfo;
  }

  auto newRunnerIterator = passiveRunners.begin();
  if (preferBest) {
    auto minIterator = std::min_element(passiveRunners.begin(), passiveRunners.end(), [](const RunnerInfo &a, const RunnerInfo &b) {
      return a.getHighScore() < b.getHighScore();
    });

    if (minIterator != passiveRunners.end()) {
      if (currentInfo == nullptr || minIterator->getHighScore() <= currentInfo->getHighScore()) {
        newRunnerIterator = minIterator;
      } else {
        return currentInfo;
      }
    }
  }

  if (currentInfo != nullptr) {
    auto currentInfoIterator = std::find_if(activeRunners.begin(), activeRunners.end(), [currentInfo](auto &runnerInfo) {
      return currentInfo == &runnerInfo;
    });

    if (currentInfoIterator == activeRunners.end()) {
      std::lock_guard<std::mutex> guardPrint(printMutex);
      std::cout << "ERROR: Runner with id " << currentInfo->getIdentifier() << " not found in active runners" << std::endl;
    } else {
      passiveRunners.splice(passiveRunners.end(), activeRunners, currentInfoIterator);
    }
  }

  activeRunners.splice(activeRunners.end(), passiveRunners, newRunnerIterator);

  return &activeRunners.back();
}

void GraphCommonState::removeActiveRunnerInfo(RunnerInfo *runnerInfo) {
  std::lock_guard<std::mutex> runnerInfoGuard(runnerInfoMutex);
  std::lock_guard<std::mutex> guardPrint(printMutex);

  std::cout << "Removing runner " << runnerInfo->getIdentifier() << std::endl;

  activeRunners.remove_if([runnerInfo](auto &activeRunnerInfo) {
    return runnerInfo == &activeRunnerInfo;
  });
}

void GraphCommonState::splitAndRemoveActiveRunnerInfo(RunnerInfo *parentRunnerInfo, std::list<RunnerInfo> childRunnerInfos) {
  std::lock_guard<std::mutex> runnerInfoGuard(runnerInfoMutex);
  std::lock_guard<std::mutex> guardPathCount(pathCountMutex);
  std::lock_guard<std::mutex> guardPrint(printMutex);

  auto depth = parentRunnerInfo->getVisitedVerticesCount();

  std::cout << "Splitting and removing runner " << parentRunnerInfo->getIdentifier() << " into runners at depth " << depth << ":";
  for (const auto &info : childRunnerInfos) {
    std::cout << " " << info.getIdentifier() << " (" << +info.getHighScore() << ")";
  }
  std::cout << std::endl;

  passiveRunners.splice(passiveRunners.end(), childRunnerInfos);

  activeRunners.remove_if([parentRunnerInfo](auto &activeRunnerInfo) {
    return parentRunnerInfo == &activeRunnerInfo;
  });

  if (appliesForLogging(depth)) {
    ++splittedPathsCount[depth];
  }
}

int GraphCommonState::runnerInfoCount() const {
  std::lock_guard<std::mutex> runnerInfoGuard(runnerInfoMutex);

  return activeRunners.size() + passiveRunners.size();
}

void GraphCommonState::logAddedPaths(int depth, int number) {
  if (!appliesForLogging(depth)) {
    return;
  }

  std::lock_guard<std::mutex> pathCountGuard(pathCountMutex);

  addedPathsCount[depth] += number;
}

void GraphCommonState::logStartedPath(int depth) {
  if (!appliesForLogging(depth)) {
    return;
  }

  std::lock_guard<std::mutex> pathCountGuard(pathCountMutex);

  ++startedPathsCount[depth];
}

void GraphCommonState::logRemovePath(int depth) {
  if (!appliesForLogging(depth)) {
    return;
  }

  std::lock_guard<std::mutex> pathCountGuard(pathCountMutex);

  ++removedPathsCount[depth];
}

bool GraphCommonState::appliesForLogging(int depth) const {
  return depth < LOG_PATH_COUNT_MAX;
}

void GraphCommonState::logPoolDumpFailed(int runnerInfoIdentifier) {
  std::lock_guard<std::mutex> stopGuard(stopMutex);
  std::lock_guard<std::mutex> guardPrint(printMutex);

  stopping = true;

  std::cout << "Stopping after pool dump failed for runner " << runnerInfoIdentifier << std::endl;
}

bool GraphCommonState::shouldStop() const {
  if (runnerInfoCount() == 0) {
    return true;
  }

  std::lock_guard<std::mutex> stopGuard(stopMutex);

  return stopping;
}

bool GraphCommonState::checkForDuplicateState(GraphPath *path, unsigned long long int visitedVerticesState) {
  auto uniqueState = visitedVerticesState | ((unsigned long long int)path->getCurrentVertex() << VERTICES_COUNT);
  auto distance = path->getMinimumEndDistance();

  // Should default to true since lambda won't be called in case the value wasn't present
  bool isGood = true;

  distanceForState.try_emplace_l(
      uniqueState,
      [path, distance, &isGood](unsigned char &value) {
        int result = distance - value;

        if (result > 0) {
          isGood = false;
        } else if (result == 0) {
          isGood = path->hasStateMax();
        } else {
          value = distance;
        }
      },
      distance);

  path->setHasStateMax(isGood);

  return isGood;
}

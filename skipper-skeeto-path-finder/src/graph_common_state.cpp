#include "skipper-skeeto-path-finder/graph_common_state.h"

#include "skipper-skeeto-path-finder/graph_path.h"
#include "skipper-skeeto-path-finder/memory_mapped_file_pool.h"

#include <fstream>
#include <iomanip>
#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#include <psapi.h>
#endif

GraphCommonState::GraphCommonState(const std::string &resultDir, int maxActiveRunners) : resultDir(resultDir), maxActiveRunners(maxActiveRunners) {
}

GraphRouteResult GraphCommonState::makeInitialCheck(const RunnerInfo *runnerInfo, GraphPath *path, unsigned long long int visitedVerticesState) {
  if (!isAcceptableDistance(path->getMinimumEndDistance(), runnerInfo->getLocalMaxDistance())) {
    // If we're sure we won't get an acceptable one, there's no reason to start at all
    return GraphRouteResult::Stop;
  }

  return checkForDuplicateState(path, visitedVerticesState);
}

bool GraphCommonState::makesSenseToKeep(const RunnerInfo *runnerInfo, const GraphPath *path, unsigned long long int visitedVerticesState) const {
  if (!isAcceptableDistance(path->getMinimumEndDistance(), runnerInfo->getLocalMaxDistance())) {
    // If we're sure we won't get an acceptable one, there's no reason to keep it
    return false;
  }

  return hasBestState(path, visitedVerticesState);
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

#ifdef FOUND_BEST_DISTANCE
    finishedStateRoutes.clear();
#endif // FOUND_BEST_DISTANCE
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

#ifdef FOUND_BEST_DISTANCE
  unsigned long long int visitedVerticesState = 0;
  for (auto vertexIndex : route) {
    visitedVerticesState |= (1ULL << vertexIndex);
    finishedStateRoutes[createUniqueState(vertexIndex, visitedVerticesState)].push_back(route);
    // Note that updating distanceForState will be done elsewhere
  }
#endif // FOUND_BEST_DISTANCE

  std::lock_guard<std::mutex> guardPrint(printMutex);
  std::cout << "Found new good one with distance " << +distance << " in runner " << runnerInfo->getIdentifier() << std::endl;
}

#ifdef FOUND_BEST_DISTANCE
bool GraphCommonState::handleWaitingPath(const GraphPathPool *pool, RunnerInfo *runnerInfo, const GraphPath *path, unsigned long long int visitedVerticesState) {
  auto state = distanceForState.at(createUniqueState(path, visitedVerticesState));
  if (!isFinalizedFromState(state)){
    return false;
  }

  auto bestDistance = distanceFromState(state);
  if (bestDistance < path->getMinimumEndDistance()) {
    return true; // We "handled" it by stating we don't need it anymore
  }

  std::lock_guard<std::mutex> guardFinalState(finalStateMutex);

  auto routeIterator = finishedStateRoutes.find(createUniqueState(path, visitedVerticesState));
  if (routeIterator == finishedStateRoutes.end()) {
    return true; // Of all possible routes from this unique vertex state, no routes finised
  }

  runnerInfo->setHighScore(maxDistance);

  auto runnerRoute = runnerInfo->getRoute();
  auto pathRoute = path->getRoute(pool);

  // Important that this is a copy and not a reference, as we want to alter the copy!
  for (auto finishedRoute : routeIterator->second) {
    for (unsigned char index = 0; index < runnerRoute.size() + runnerRoute.size(); ++index) {
      if (index < runnerRoute.size()) {
        finishedRoute[index] = runnerRoute[index];
      } else {
        finishedRoute[index] = pathRoute[index - runnerRoute.size()];
      }
    }

    goodOnes.push_back(finishedRoute);
  }

  std::lock_guard<std::mutex> guardPrint(printMutex);
  std::cout << "Found " << routeIterator->second.size() << " new good ones for existing best distance in runner " << runnerInfo->getIdentifier() << std::endl;

  return true;
}
#endif // FOUND_BEST_DISTANCE

void GraphCommonState::dumpGoodOnes() {
  std::lock_guard<std::mutex> guardFinalState(finalStateMutex);
  if (dumpedGoodOnes >= goodOnes.size()) {
    return;
  }

  int newDumpedCount = goodOnes.size() - dumpedGoodOnes;

  std::string fileName = resultDir + "/" + std::to_string(maxDistance) + ".txt";

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

std::pair<std::vector<std::array<char, VERTICES_COUNT>>, int> GraphCommonState::getGoodOnes() const {
  std::lock_guard<std::mutex> guardFinalState(finalStateMutex);

  return std::make_pair(goodOnes, maxDistance);
}

void GraphCommonState::printStatus() {
  std::lock_guard<std::mutex> runnerInfoGuard(runnerInfoMutex);
  std::lock_guard<std::mutex> guardFinalState(finalStateMutex);
  std::lock_guard<std::mutex> guardPathCount(pathCountMutex);
  std::lock_guard<std::mutex> guardPrint(printMutex);

  std::cout << std::endl;

  std::cout << "Status: " << (passiveRunners.size() + activeRunners.size()) << " relevant runners, found " << goodOnes.size() << " at distance " << +maxDistance << std::endl;

  std::cout << "On top of that, " << waitingRunners.size() << " runners are waiting for result - " << (consumingWaiting ? "and they are currently being consumed" : "and they are not being consumed") << std::endl;

  std::cout << "Active runners:";
  for (const auto &info : activeRunners) {
    std::cout << " " << info.getIdentifier() << " (" << +info.getHighScore() << "/" << (info.shouldWaitForResults() ? "w" : "a") << ")";
  }
  std::cout << std::endl;

  std::cout << "Paths [ Added" << std::setw(14)
            << "| Started" << std::setw(25)
            << "| Removed" << std::setw(25)
            << "| Waiting" << std::setw(26)
            << "| Splitted" << std::setw(35)
            << "| Remove + wait + split" << std::setw(6)
            << "]" << std::endl;

  for (int index = 0; index < LOG_PATH_COUNT_MAX; ++index) {
    std::cout << " - " << std::setw(2) << index << std::fixed << std::setprecision(2) << " ["
              << std::setw(10) << addedPathsCount[index] << " | "
              << std::setw(12) << startedPathsCount[index] << " (" << std::setw(6) << (addedPathsCount[index] > 0 ? (double)startedPathsCount[index] * 100 / addedPathsCount[index] : 0) << "%) | "
              << std::setw(12) << removedPathsCount[index] << " (" << std::setw(6) << (addedPathsCount[index] > 0 ? (double)removedPathsCount[index] * 100 / addedPathsCount[index] : 0) << "%) | "
              << std::setw(12) << waitingPathsCount[index] << " (" << std::setw(6) << (addedPathsCount[index] > 0 ? (double)waitingPathsCount[index] * 100 / addedPathsCount[index] : 0) << "%) | "
              << std::setw(12) << splittedPathsCount[index] << " (" << std::setw(6) << (addedPathsCount[index] > 0 ? (double)splittedPathsCount[index] * 100 / addedPathsCount[index] : 0) << "%) | "
              << std::setw(12) << removedPathsCount[index] + waitingPathsCount[index] + splittedPathsCount[index] << " (" << std::setw(6) << (addedPathsCount[index] > 0 ? (double)(removedPathsCount[index] + waitingPathsCount[index] + splittedPathsCount[index]) * 100 / addedPathsCount[index] : 0) << "%) "
              << "]" << std::endl;
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

  if (currentInfo != nullptr) {
    auto currentInfoIterator = std::find_if(activeRunners.begin(), activeRunners.end(), [currentInfo](auto &runnerInfo) {
      return currentInfo == &runnerInfo;
    });

    if (currentInfoIterator == activeRunners.end()) {
      std::lock_guard<std::mutex> guardPrint(printMutex);
      std::cout << "ERROR: Runner with id " << currentInfo->getIdentifier() << " not found in active runners" << std::endl;
    } else if (currentInfo->shouldWaitForResults()) {
      waitingRunners.splice(waitingRunners.end(), activeRunners, currentInfoIterator);
    } else {
      passiveRunners.splice(passiveRunners.end(), activeRunners, currentInfoIterator);
    }
  }

  if (passiveRunners.empty()) {
    if (activeRunners.empty()) {
      consumingWaiting = true;
    } else {
      return nullptr;
    }
  }

  if (consumingWaiting){
    if (waitingRunners.empty()) {
      return nullptr;
    }

    auto newRunnerIterator = waitingRunners.begin();

    activeRunners.splice(activeRunners.end(), waitingRunners, newRunnerIterator);
  } else {
    auto newRunnerIterator = passiveRunners.begin();
    if (preferBest) {
      newRunnerIterator = std::min_element(passiveRunners.begin(), passiveRunners.end(), [](const RunnerInfo &a, const RunnerInfo &b) {
        return a.getHighScore() < b.getHighScore();
      });
    }

    activeRunners.splice(activeRunners.end(), passiveRunners, newRunnerIterator);
  }

  auto newRunner = &activeRunners.back();
  newRunner->setHandleWaiting(consumingWaiting);

  std::lock_guard<std::mutex> guard(finalStateMutex);
  newRunner->setLocalMaxDistance(maxDistance);

  return newRunner;
}

void GraphCommonState::removeActiveRunnerInfo(RunnerInfo *runnerInfo) {
  std::lock_guard<std::mutex> runnerInfoGuard(runnerInfoMutex);
  std::lock_guard<std::mutex> guardPrint(printMutex);

  auto depth = runnerInfo->getVisitedVerticesCount();

  std::cout << "Removing runner " << runnerInfo->getIdentifier() << " at depth " << depth << std::endl;

  activeRunners.remove_if([runnerInfo](auto &activeRunnerInfo) {
    return runnerInfo == &activeRunnerInfo;
  });

  if (activeRunners.empty() && passiveRunners.empty()) {
      consumingWaiting = true;
  }
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

  return activeRunners.size() + passiveRunners.size() + waitingRunners.size();
}

void GraphCommonState::registerAddedPaths(int depth, int number) {
  if (!appliesForLogging(depth)) {
    return;
  }

  std::lock_guard<std::mutex> pathCountGuard(pathCountMutex);

  addedPathsCount[depth] += number;
}

void GraphCommonState::registerStartedPath(int depth) {
  if (!appliesForLogging(depth)) {
    return;
  }

  std::lock_guard<std::mutex> pathCountGuard(pathCountMutex);

  ++startedPathsCount[depth];
}

void GraphCommonState::registerWaitingPath(int depth) {
  if (!appliesForLogging(depth)) {
    return;
  }
  
  std::lock_guard<std::mutex> runnerInfoGuard(runnerInfoMutex);
  if (consumingWaiting) {
    // No more can be starting at this point (and it's just easier to catch that here at the moment)
    return;
  }

  std::lock_guard<std::mutex> pathCountGuard(pathCountMutex);

  ++waitingPathsCount[depth];
}

void GraphCommonState::registerRemovedPath(const GraphPath *path, unsigned long long int visitedVerticesState, int depth) {
  if (path->hasStateMax()) {
    // It would have been nice to set this for only failed paths to keep other logic simpler, but this function is for (other) simplicity also
    // called for successful paths at the moment
    finalizeDistanceState(path, visitedVerticesState);
  }

  if (!appliesForLogging(depth)) {
    return;
  }

  std::lock_guard<std::mutex> pathCountGuard(pathCountMutex);

  ++removedPathsCount[depth];

  std::lock_guard<std::mutex> runnerInfoGuard(runnerInfoMutex);
  if (consumingWaiting) {
    // At this point we know all remaining are waiting
    --waitingPathsCount[depth];
  }
}

bool GraphCommonState::appliesForLogging(int depth) const {
  return depth < LOG_PATH_COUNT_MAX;
}

void GraphCommonState::registerPoolDumpFailed(int runnerInfoIdentifier) {
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

bool GraphCommonState::shouldPause(int index) const {
  std::lock_guard<std::mutex> maxActiveRunnersGuard(maxActiveRunnersMutex);

  return maxActiveRunners <= index;
}

void GraphCommonState::setMaxActiveRunners(int count) {
  std::lock_guard<std::mutex> maxActiveRunnersGuard(maxActiveRunnersMutex);

  maxActiveRunners = count;
}

 unsigned long long int GraphCommonState::createUniqueState(const GraphPath *path, unsigned long long int visitedVerticesState) {
  return createUniqueState(path->getCurrentVertex(), visitedVerticesState);
 }

 unsigned long long int GraphCommonState::createUniqueState(char currentVertex, unsigned long long int visitedVerticesState) {
   return visitedVerticesState | ((unsigned long long int)currentVertex << VERTICES_COUNT);
 }

 unsigned char GraphCommonState::createDistanceState(unsigned char distance, bool isFinalized) {
   return (distance << 1) | (isFinalized ? 1 : 0);
 }

 unsigned char GraphCommonState::distanceFromState(unsigned char state) {
   return state >> 1;
 }

 bool GraphCommonState::isFinalizedFromState(unsigned char state) {
   return (state & 1) == 1;
 }

GraphRouteResult GraphCommonState::checkForDuplicateState(GraphPath *path, unsigned long long int visitedVerticesState) {
  auto distance = path->getMinimumEndDistance();

  // Should default to true since lambda won't be called in case the value wasn't present
  GraphRouteResult result = GraphRouteResult::Continue;

  distanceForState.try_emplace_l(
      createUniqueState(path, visitedVerticesState),
      [path, distance, &result](unsigned char &value) {
        int difference = distance - distanceFromState(value);

        if (difference > 0) {
          result = GraphRouteResult::Stop;
        } else if (difference == 0) {
          if (path->hasStateMax()) {
            // This can happen even for initialize as we might not have fully initialized previously do to a full pool
            result = GraphRouteResult::Continue;
          } else {
#ifdef FOUND_BEST_DISTANCE
            result = GraphRouteResult::WaitForResult;
#else
            result = GraphRouteResult::Stop;
#endif // FOUND_BEST_DISTANCE
          }
        } else {
          value = createDistanceState(distance, false);
        }
      },
      createDistanceState(distance, false)
      );

  path->setHasStateMax(result == GraphRouteResult::Continue);

#ifdef FOUND_BEST_DISTANCE
  path->setIsWaitingForResult(result == GraphRouteResult::WaitForResult);
#endif // FOUND_BEST_DISTANCE

  return result;
}

bool GraphCommonState::hasBestState(const GraphPath *path, unsigned long long int visitedVerticesState) const {
  auto state = distanceForState.at(createUniqueState(path, visitedVerticesState));
  auto bestDistance = distanceFromState(state);

  return bestDistance >= path->getMinimumEndDistance();
}

void GraphCommonState::finalizeDistanceState(const GraphPath *path, unsigned long long int visitedVerticesState) {
  auto distance = path->getMinimumEndDistance();

  distanceForState.try_emplace_l(
      createUniqueState(path, visitedVerticesState),
      [distance](unsigned char &value) {
        int difference = distance - distanceFromState(value);

        if (difference <= 0) {
          value = createDistanceState(distance, true);
        }
      },
      createDistanceState(distance, true)
  );
}

bool GraphCommonState::isAcceptableDistance(unsigned char distance, unsigned char maxDistance) const {
#ifdef FOUND_BEST_DISTANCE
  // We allow more to get all possible paths
  return distance <= maxDistance;
#else
  // We allow less to make sure we find a best path as quickly as possible
  return distance < maxDistance;
#endif // FOUND_BEST_DISTANCE
}

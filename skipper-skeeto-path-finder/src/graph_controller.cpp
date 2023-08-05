#include "skipper-skeeto-path-finder/graph_controller.h"

#include "skipper-skeeto-path-finder/file_helper.h"
#include "skipper-skeeto-path-finder/graph_common_state.h"
#include "skipper-skeeto-path-finder/graph_data.h"
#include "skipper-skeeto-path-finder/graph_path.h"
#include "skipper-skeeto-path-finder/graph_path_pool.h"
#include "skipper-skeeto-path-finder/info.h"
#include "skipper-skeeto-path-finder/path_controller.h"
#include "skipper-skeeto-path-finder/runner_info.h"
#include "skipper-skeeto-path-finder/vertex.h"

#include <bitset>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <list>
#include <thread>

const unsigned long long int GraphController::ALL_VERTICES_STATE_MASK = (1ULL << VERTICES_COUNT) - 1;

#define FORCE_FINISH_THRESHOLD_DEPTH 10

GraphController::GraphController(const PathController *pathController, const GraphData *data, const std::string &resultDir)
    : commonState(resultDir, THREAD_COUNT), data(data), pathController(pathController) {
}

void GraphController::start() {
  std::cout << "Starting with thread count " << THREAD_COUNT << " and pool size " << POOL_COUNT << " (Path Graph size being " << sizeof(GraphPath) << " bytes)" << std::endl;

  FileHelper::createDir(TEMP_DIR);
  FileHelper::createDir(TEMP_PATHS_DIR);

  auto threadFunction = [this](int threadIndex) {
    bool preferBest = (threadIndex % 2 == 0);

    GraphPathPool pool;

    RunnerInfo *runnerInfo = nullptr;
    while (!commonState.shouldStop()) {
      if(commonState.shouldPause(threadIndex)) {
        if(runnerInfo != nullptr) {
          serializePool(&pool, runnerInfo->getIdentifier(), false);
        }
        break;
      }

      int oldRunnerInfoIdentifier = -1;
      if (runnerInfo != nullptr) {
        oldRunnerInfoIdentifier = runnerInfo->getIdentifier();
      }

      runnerInfo = commonState.getNextRunnerInfo(runnerInfo, preferBest);
      if (runnerInfo == nullptr) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        continue;
      }

      if (runnerInfo->getIdentifier() != oldRunnerInfoIdentifier) {
        if (oldRunnerInfoIdentifier != -1) {
          bool success = serializePool(&pool, oldRunnerInfoIdentifier, false);
          if (!success) {
            break;
          }
        }

        deserializePool(&pool, runnerInfo);
      }

      auto pathIndex = 1; // This should always be the root
      auto path = pool.getGraphPath(pathIndex);

      unsigned long long int visitedVerticesState = 0;
      auto route = runnerInfo->getRoute();
      route.push_back(path->getCurrentVertex());
      for (auto vertexIndex : route) {
        visitedVerticesState |= (1ULL << vertexIndex);
      }

      auto depth = runnerInfo->getVisitedVerticesCount();

      while (!pool.isFull()) {
        commonState.updateLocalMax(runnerInfo);

        bool forceFinished = (path->getBestEndDistance() == GraphPath::MAX_DISTANCE && depth < FORCE_FINISH_THRESHOLD_DEPTH);
        bool continueWork = moveOnDistributed(&pool, runnerInfo, pathIndex, path, visitedVerticesState, depth, forceFinished);
        if (!continueWork) {
          deletePoolFile(runnerInfo);
          commonState.removeActiveRunnerInfo(runnerInfo);
          runnerInfo = nullptr;
          break;
        }
      }

      if (runnerInfo != nullptr && (pool.isFull() || commonState.runnerInfoCount() < THREAD_COUNT)) {
        deletePoolFile(runnerInfo);
        splitAndRemove(&pool, runnerInfo);
        runnerInfo = nullptr;
      }
    }
  };

  setupStartRunner();

  std::array<std::thread, THREAD_COUNT> threads;
  for (int index = 0; index < threads.size(); ++index) {
    threads[index] = std::thread(threadFunction, index);
  }

  std::thread cinThread(
    [this]() {
      while(!commonState.shouldStop()) {
        int newThreadCount;
	std::cin >> newThreadCount;
	if(newThreadCount == 0) {
	  newThreadCount = THREAD_COUNT;
	} else if (newThreadCount = 1000) {
	  newThreadCount = 0;
	}
	if(newThreadCount <= THREAD_COUNT) {
	  std::cout << "Updating thread count to " << newThreadCount << std::endl;
	  commonState.setMaxActiveRunners(newThreadCount);
	}
        std::this_thread::sleep_for(std::chrono::seconds(5));
      }
    }
  );

  int lastIndex = THREAD_COUNT - 1;
  while (!commonState.shouldStop()) {
    printAndDump();

    while(lastIndex >= 0 && commonState.shouldPause(lastIndex)) {
      threads[lastIndex].join();
      std::cout << "JOINED THREAD #" << lastIndex << std::endl;
      --lastIndex;
    }
    while(lastIndex < THREAD_COUNT && !commonState.shouldPause(lastIndex+1)) {
      ++lastIndex;
      threads[lastIndex] = std::thread(threadFunction, lastIndex);
      std::cout << "STARTED THREAD #" << lastIndex << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
  }

  for (int index = 0; index < threads.size(); ++index) {
    threads[index].join();
  }

  printAndDump();

  std::cout << "We're now close to done. Enter a random number and press enter: ";
  cinThread.join();
}

void GraphController::setupStartRunner() {
  GraphPathPool tempPool;

  commonState.registerAddedPaths(0, 1);

  auto startPathIndex = tempPool.generateNewIndex();
  auto startPath = tempPool.getGraphPath(startPathIndex);

  auto startVertexIndex = data->getStartIndex();

  unsigned char minimumEndDistance = data->getStartLength();
  for (int index = 0; index < VERTICES_COUNT; ++index) {
    if (index == startVertexIndex) {
      // We already visited this so it shouldn't affect minimum distance
      continue;
    }

    minimumEndDistance += data->getMinimumEntryDistance(index);
  }

  startPath->initialize(startVertexIndex, 0, minimumEndDistance);

  // This shouldn't make a difference, but just to be sure
  startPath->setPreviousPath(startPathIndex);
  startPath->setNextPath(startPathIndex);

  RunnerInfo startRunnerInfo(std::vector<char>{});

  serializePool(&tempPool, &startRunnerInfo, false);

  std::list<RunnerInfo> runnerInfos{startRunnerInfo};
  commonState.addRunnerInfos(runnerInfos);
}

bool GraphController::moveOnDistributed(GraphPathPool *pool, RunnerInfo *runnerInfo, unsigned long int pathIndex, GraphPath *path, unsigned long long int visitedVerticesState, int depth, bool forceFinish) {
  if (!path->hasSetSubPath()) {
    if (visitedVerticesState == ALL_VERTICES_STATE_MASK) {
      path->maybeSetBestEndDistance(pool, path->getMinimumEndDistance());

      commonState.handleFinishedPath(pool, runnerInfo, path);

      return false;
    }

    if (!forceFinish && !commonState.makesSenseToInitialize(runnerInfo, path)) {
      commonState.registerStartedPath(depth);

      return false;
    }

    if (!initializePath(pool, pathIndex, path, visitedVerticesState, depth)) {
      return true; // Should just be because of full pool, so we want to continue
    }
  }

  if (path->isExhausted()) {
    return false;
  }

  if (!forceFinish && !commonState.makesSenseToKeep(runnerInfo, path, visitedVerticesState)) {
    logRemovedSubPaths(pool, path, depth);

    return false;
  }

  auto subPathIterationCount = path->getSubPathIterationCount();
  if (subPathIterationCount == 0) {
    subPathIterationCount = sortSubPaths(pool, pathIndex, path);
  }

  auto focusedSubPathIndex = path->getFocusedSubPath();
  auto focusedSubPath = pool->getGraphPath(focusedSubPathIndex);
  auto subDepth = depth + 1;
  unsigned long long subPathVisitedVerticesState = visitedVerticesState | (1ULL << focusedSubPath->getCurrentVertex());
  bool subForceFinish = forceFinish || (path->getBestEndDistance() == GraphPath::MAX_DISTANCE && depth < FORCE_FINISH_THRESHOLD_DEPTH);

  bool continueWork = moveOnDistributed(pool, runnerInfo, focusedSubPathIndex, focusedSubPath, subPathVisitedVerticesState, subDepth, subForceFinish);
  if (continueWork) {
    if (pool->isFull()) {
      // We want to continue with the same focus next time. It just didn't finish becasue pool was full
      return true;
    }

    path->updateFocusedSubPath(focusedSubPath->getNextPath(), subPathIterationCount - 1);

    return true;
  } else {
    auto nextSubPathIndex = focusedSubPath->getNextPath();
    auto nextSubPath = pool->getGraphPath(nextSubPathIndex);
    if (nextSubPath == focusedSubPath) {
      path->updateFocusedSubPath(0, 0);
    } else {
      auto previousSubPathIndex = focusedSubPath->getPreviousPath();
      auto previousSubPath = pool->getGraphPath(previousSubPathIndex);

      previousSubPath->setNextPath(nextSubPathIndex);
      nextSubPath->setPreviousPath(previousSubPathIndex);

      path->updateFocusedSubPath(nextSubPathIndex, subPathIterationCount - 1);
    }

    commonState.registerRemovedPath(subDepth);

    focusedSubPath->cleanUp();

    return !path->isExhausted();
  }
}

bool GraphController::initializePath(GraphPathPool *pool, unsigned long int pathIndex, GraphPath *path, unsigned long long int visitedVerticesState, int depth) {
  std::array<char, VERTICES_COUNT> nextVertices{};
  nextVertices.fill(-1);

  std::array<char, VERTICES_COUNT> visitedVertices{};
  visitedVertices.fill(-1);

  auto startVertex = path->getCurrentVertex();
  visitedVertices[startVertex] = 0;

  char unresovedVertices = 1;
  while (unresovedVertices > 0) {
    unresovedVertices = 0;

    for (char vertexIndex = 0; vertexIndex < VERTICES_COUNT; ++vertexIndex) {
      auto currentDistance = visitedVertices[vertexIndex];
      if (currentDistance < 0) {
        continue;
      }

      for (const auto &edge : data->getEdgesForVertex(vertexIndex)) {
        auto newDistance = currentDistance + edge->length;
        auto endVertexIndex = edge->endVertexIndex;

        if (nextVertices[endVertexIndex] >= 0) {
          if (newDistance < nextVertices[endVertexIndex]) {
            nextVertices[endVertexIndex] = newDistance;
          }
        } else if (hasVisitedVertex(visitedVerticesState, endVertexIndex)) {
          if (data->getVertex(endVertexIndex)->isOneTime) {
            continue;
          }

          if (visitedVertices[endVertexIndex] < 0 || newDistance < visitedVertices[endVertexIndex]) {
            visitedVertices[endVertexIndex] = newDistance;
            ++unresovedVertices;
          }
        } else if (meetsCondition(visitedVerticesState, edge->condition)) {
          nextVertices[endVertexIndex] = newDistance;
        }
      }
    }
  }

  // Insert sub paths into list - note that we sort the shortest first
  std::vector<std::pair<unsigned long int, GraphPath *>> subPathsSorted;
  for (char vertexIndex = 0; vertexIndex < VERTICES_COUNT; ++vertexIndex) {
    auto distance = nextVertices[vertexIndex];
    if (distance < 0) {
      continue;
    }

    unsigned char minimumEndDistance = path->getMinimumEndDistance() - data->getMinimumEntryDistance(vertexIndex) + distance;
    if (minimumEndDistance > GraphPath::MAX_DISTANCE) {
      continue;
    }

    if (pool->isFull()) {
      return false;
    }

    auto newPathIndex = pool->generateNewIndex();
    auto newPath = pool->getGraphPath(newPathIndex);

    newPath->initialize(vertexIndex, pathIndex, minimumEndDistance);

    const auto &longerLengthIterator = std::upper_bound(
        subPathsSorted.begin(),
        subPathsSorted.end(),
        newPath,
        [](const auto &subPath, const auto &otherPathPair) {
          return subPath->getMinimumEndDistance() < otherPathPair.second->getMinimumEndDistance();
        });
    subPathsSorted.insert(longerLengthIterator, std::make_pair(newPathIndex, newPath));
  }

  for (auto iterator = subPathsSorted.begin(); iterator != subPathsSorted.end(); ++iterator) {
    if (iterator == subPathsSorted.begin()) {
      iterator->second->setPreviousPath(subPathsSorted.back().first);
    } else {
      iterator->second->setPreviousPath((iterator - 1)->first);
    }

    auto nextPathIterator = iterator + 1;
    if (nextPathIterator == subPathsSorted.end()) {
      iterator->second->setNextPath(subPathsSorted.front().first);
    } else {
      iterator->second->setNextPath(nextPathIterator->first);
    }
  }

  commonState.registerStartedPath(depth);

  if (subPathsSorted.empty()) {
    path->updateFocusedSubPath(0, 0);
  } else {
    path->updateFocusedSubPath(subPathsSorted.front().first, subPathsSorted.size());
    commonState.registerAddedPaths(depth + 1, subPathsSorted.size());
  }

  return true;
}

bool GraphController::meetsCondition(unsigned long long int visitedVerticesState, unsigned long long int condition) {
  return (visitedVerticesState & condition) == condition;
}

bool GraphController::hasVisitedVertex(unsigned long long int visitedVerticesState, unsigned char vertexIndex) {
  return meetsCondition(visitedVerticesState, (1ULL << vertexIndex));
}

unsigned char GraphController::sortSubPaths(GraphPathPool *pool, unsigned long int pathIndex, GraphPath *path) {
  auto focusedIndex = path->getFocusedSubPath();
  auto endIndex = pool->getGraphPath(focusedIndex)->getPreviousPath();
  auto currentIndex = focusedIndex;

  unsigned char itemCount = 0;
  while (true) {
    ++itemCount;

    auto currentPath = pool->getGraphPath(currentIndex);
    auto nextIndex = currentPath->getNextPath();

    auto potentialNewIndex = currentPath->getPreviousPath();
    auto newIndex = currentIndex;
    while (potentialNewIndex != endIndex) {
      auto potentialNewPath = pool->getGraphPath(potentialNewIndex);

      if (currentPath->getBestEndDistance() < potentialNewPath->getBestEndDistance()) {
        newIndex = potentialNewIndex;
      } else {
        break;
      }

      potentialNewIndex = potentialNewPath->getPreviousPath();
    }

    if (newIndex != currentIndex) {
      pool->getGraphPath(currentPath->getPreviousPath())->setNextPath(currentPath->getNextPath());
      pool->getGraphPath(currentPath->getNextPath())->setPreviousPath(currentPath->getPreviousPath());

      auto newNextPath = pool->getGraphPath(newIndex);
      auto newPreviousPathIndex = newNextPath->getPreviousPath();
      auto newPreviousPath = pool->getGraphPath(newPreviousPathIndex);

      currentPath->setNextPath(newIndex);
      currentPath->setPreviousPath(newPreviousPathIndex);
      newNextPath->setPreviousPath(currentIndex);
      newPreviousPath->setNextPath(currentIndex);
    }

    if (potentialNewIndex == endIndex) {
      focusedIndex = currentIndex;
    }

    if (currentIndex == endIndex) {
      break;
    } else {
      currentIndex = nextIndex;
    }
  }

  path->updateFocusedSubPath(focusedIndex, itemCount);

  return itemCount;
}

void GraphController::logRemovedSubPaths(GraphPathPool *pool, GraphPath *path, int depth) {
  auto subDepth = depth + 1;

  if (!commonState.appliesForLogging(subDepth)) {
    return;
  }

  auto initialSubPathIndex = path->getFocusedSubPath();
  auto nextSubPathIndex = initialSubPathIndex;
  while (true) {

    auto subPath = pool->getGraphPath(nextSubPathIndex);
    if (subPath->hasSetSubPath()) {
      logRemovedSubPaths(pool, subPath, subDepth);
    } else {
      commonState.registerStartedPath(subDepth);
    }

    commonState.registerRemovedPath(subDepth);

    nextSubPathIndex = subPath->getNextPath();
    if (nextSubPathIndex == initialSubPathIndex) {
      break;
    }
  }
}

void GraphController::splitAndRemove(GraphPathPool *pool, RunnerInfo *runnerInfo) {
  auto rootPath = pool->getGraphPath(1);

  std::list<RunnerInfo> runnerInfos;
  auto subRootPathIndex = rootPath->getFocusedSubPath();
  while (true) {
    auto subRunnerInfo = runnerInfo->makeSubRunner(rootPath->getCurrentVertex());
    subRunnerInfo.setHighScore(pool->getGraphPath(subRootPathIndex)->getBestEndDistance());

    pool->reset();
    auto newSubPathIndex = groupPathTree(pool, subRootPathIndex);
    auto newSubPath = pool->getGraphPath(newSubPathIndex);

    auto originalParentPathIndex = newSubPath->getParentPath();
    auto originalNextPathIndex = newSubPath->getNextPath();
    auto originalPreviousPathIndex = newSubPath->getPreviousPath();

    // Important to set parent and siblings to "default" since this will be our root next
    // time it's deserialized. This is important next time we get to splitting up this
    // sub path because the swap function needs to fix references and in that case it's
    // really important they're not referencing an (then) invalid parent/siblings paths
    newSubPath->setParentPath(0);
    newSubPath->setNextPath(newSubPathIndex);
    newSubPath->setPreviousPath(newSubPathIndex);

    // Note that we clear the pahts since there's no need to deal with their data when
    // swapping paths in next subpath iteration. We will however setup this subpath again
    // right under here since it's still going to be used in the loop
    runnerInfos.push_back(subRunnerInfo);
    bool success = serializePool(pool, &subRunnerInfo, true);
    if (!success) {
      break;
    }

    // Set back parent and siblings which is important when dealing with the remaining
    // sub paths (since espcially the swap function otherwise gets totally confused)
    newSubPath->setParentPath(originalParentPathIndex);
    newSubPath->setNextPath(originalNextPathIndex);
    newSubPath->setPreviousPath(originalPreviousPathIndex);

    subRootPathIndex = originalNextPathIndex;
    rootPath = pool->getGraphPath(originalParentPathIndex);
    if (subRootPathIndex == rootPath->getFocusedSubPath()) {
      break;
    }
  }

  // Wait until here, since the runnerInfos are available to all threads after this
  commonState.splitAndRemoveActiveRunnerInfo(runnerInfo, runnerInfos);
}

unsigned long int GraphController::groupPathTree(GraphPathPool *pool, unsigned long int originalPathIndex) {
  auto newPathIndex = pool->generateNewIndex();

  swap(pool, originalPathIndex, newPathIndex);

  GraphPath *path = pool->getGraphPath(newPathIndex);
  if (path->hasSetSubPath()) {
    auto currentSubPathIndex = path->getFocusedSubPath();
    while (true) {
      auto newSubPathIndex = groupPathTree(pool, currentSubPathIndex);

      currentSubPathIndex = pool->getGraphPath(newSubPathIndex)->getNextPath();
      if (currentSubPathIndex == path->getFocusedSubPath()) {
        break;
      }
    }
  }

  return newPathIndex;
}

std::string GraphController::getPoolFileName(unsigned int runnerInfoIdentifier) const {
  return std::string(TEMP_PATHS_DIR) + "/" + std::to_string(runnerInfoIdentifier) + ".dat";
}

bool GraphController::serializePool(GraphPathPool *pool, RunnerInfo *runnerInfo, bool clearSerializedPaths) {
  return serializePool(pool, runnerInfo->getIdentifier(), clearSerializedPaths);
}

bool GraphController::serializePool(GraphPathPool *pool, unsigned int runnerInfoIdentifier, bool clearSerializedPaths) {
  std::string fileName = getPoolFileName(runnerInfoIdentifier);
  std::ofstream dumpFile(fileName, std::ios::binary | std::ios::trunc);

  if (clearSerializedPaths) {
    pool->serializeAndClear(dumpFile);
  } else {
    pool->serialize(dumpFile);
  }

  bool dumpSuccess = !dumpFile.fail();

  if (!dumpSuccess) {
    commonState.registerPoolDumpFailed(runnerInfoIdentifier);
  }

  return dumpSuccess;
}

void GraphController::deserializePool(GraphPathPool *pool, RunnerInfo *runnerInfo) {
  std::string fileName = getPoolFileName(runnerInfo->getIdentifier());
  std::ifstream dumpFile(fileName, std::ios::binary);
  pool->deserialize(dumpFile);
}

void GraphController::deletePoolFile(const RunnerInfo *runnerInfo) const {
  std::string fileName = getPoolFileName(runnerInfo->getIdentifier());
  std::remove(fileName.c_str());
}

void GraphController::swap(GraphPathPool *pool, unsigned long int pathIndexA, unsigned long int pathIndexB) {
  if (pathIndexA == pathIndexB) {
    return;
  }

  auto pathA = pool->getGraphPath(pathIndexA);
  auto pathB = pool->getGraphPath(pathIndexB);

  // Important to get BOTH before we start alterting either, otherwise important info might be overriden
  auto referencesA = getReferences(pool, pathA);
  auto referencesB = getReferences(pool, pathB);

  setNewIndex(pool, referencesA, pathIndexB);
  setNewIndex(pool, referencesB, pathIndexA);

  GraphPath::swap(pathA, pathB);
}

GraphController::PathReferences GraphController::getReferences(GraphPathPool *pool, const GraphPath *path) const {
  PathReferences pathReferences;

  if (path->isClean()) {
    return pathReferences;
  }

  pathReferences.nextPath = pool->getGraphPath(path->getNextPath());
  pathReferences.previousPath = pool->getGraphPath(path->getPreviousPath());

  auto parentIndex = path->getParentPath();
  if (parentIndex > 0) {
    auto parent = pool->getGraphPath(parentIndex);
    if (pool->getGraphPath(parent->getFocusedSubPath()) == path) {
      pathReferences.parentPath = parent;
    }
  }

  if (path->hasSetSubPath()) {
    auto firstSubPathIndex = path->getFocusedSubPath();
    auto currentSubPathIndex = firstSubPathIndex;

    while (true) {
      auto currentSubPath = pool->getGraphPath(currentSubPathIndex);
      pathReferences.subPaths.push_back(currentSubPath);

      currentSubPathIndex = currentSubPath->getNextPath();
      if (currentSubPathIndex == firstSubPathIndex) {
        break;
      }
    }
  }

  return pathReferences;
}

void GraphController::setNewIndex(GraphPathPool *pool, const PathReferences &pathReferences, unsigned long int newIndex) {
  if (pathReferences.parentPath != nullptr) {
    pathReferences.parentPath->updateFocusedSubPath(newIndex, pathReferences.parentPath->getSubPathIterationCount());
  }

  if (pathReferences.nextPath != nullptr) {
    pathReferences.nextPath->setPreviousPath(newIndex);
  }

  if (pathReferences.previousPath != nullptr) {
    pathReferences.previousPath->setNextPath(newIndex);
  }

  for (auto path : pathReferences.subPaths) {
    path->setParentPath(newIndex);
  }
}

void GraphController::printAndDump() {
  commonState.printStatus();

  commonState.dumpGoodOnes();

  std::vector<std::array<char, VERTICES_COUNT>> graphResults;
  int graphLength;
  std::tie(graphResults, graphLength) = commonState.getGoodOnes();

  std::vector<std::array<const Vertex *, VERTICES_COUNT>> vertexBasedGraphResult;
  for (const auto &graphResult : graphResults) {
    std::array<const Vertex *, VERTICES_COUNT> sceneResult;
    for (int index = 0; index < VERTICES_COUNT; ++index) {
      sceneResult[index] = data->getVertex(graphResult[index]);
    };
    vertexBasedGraphResult.push_back(sceneResult);
  }

  pathController->resolveAndDumpResults(vertexBasedGraphResult, graphLength);
}

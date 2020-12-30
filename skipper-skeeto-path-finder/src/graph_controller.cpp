#include "skipper-skeeto-path-finder/graph_controller.h"

#include "skipper-skeeto-path-finder/file_helper.h"
#include "skipper-skeeto-path-finder/graph_common_state.h"
#include "skipper-skeeto-path-finder/graph_data.h"
#include "skipper-skeeto-path-finder/graph_path.h"
#include "skipper-skeeto-path-finder/graph_path_pool.h"
#include "skipper-skeeto-path-finder/info.h"
#include "skipper-skeeto-path-finder/runner_info.h"

#include <bitset>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <list>
#include <sstream>
#include <thread>

const char *GraphController::MEMORY_DUMP_DIR = "temp_memory_dump";

const unsigned long long int GraphController::ALL_VERTICES_STATE_MASK = (1ULL << VERTICES_COUNT) - 1;

GraphController::GraphController(const GraphData *data) {
  this->data = data;

  // Do not use localtime(), see https://stackoverflow.com/a/38034148/2761541
  std::time_t currentTime = std::time(nullptr);
  std::ostringstream stringStream;
  std::tm localTime{};
#ifdef _WIN32
  localtime_s(&localTime, &currentTime);
#else
  localtime_r(&currentTime, &localTime);
#endif
  stringStream << std::put_time(&localTime, "%Y%m%d-%H%M%S");
  resultDirName = stringStream.str();
}

void GraphController::start() {
  std::cout << "Starting with thread count " << THREAD_COUNT << " and pool size " << POOL_COUNT << " (Path Graph size being " << sizeof(GraphPath) << " bytes)" << std::endl;

  FileHelper::createDir(MEMORY_DUMP_DIR);

  auto threadFunction = [this](bool preferBest) {
    GraphPathPool pool;

    RunnerInfo *runnerInfo = nullptr;
    while (true) {
      int oldRunnerInfoIdentifier = -1;
      if (runnerInfo != nullptr) {
        oldRunnerInfoIdentifier = runnerInfo->getIdentifier();
      }

      runnerInfo = commonState.getNextRunnerInfo(runnerInfo, preferBest);
      if (runnerInfo == nullptr) {
        if (commonState.runnerInfoCount() == 0) {
          break;
        } else {
          std::this_thread::sleep_for(std::chrono::seconds(10));
          continue;
        }
      }

      if (runnerInfo->getIdentifier() != oldRunnerInfoIdentifier) {
        if (oldRunnerInfoIdentifier != -1) {
          serializePool(&pool, oldRunnerInfoIdentifier);
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

      while (!pool.isFull()) {
        bool continueWork = moveOnDistributed(&pool, runnerInfo, pathIndex, path, visitedVerticesState, runnerInfo->getVisitedVerticesCount());
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
    bool preferBest = (index < (threads.size() / 2));
    threads[index] = std::thread(threadFunction, preferBest);
  }

  while (commonState.runnerInfoCount() > 0) {
    commonState.printStatus();

    commonState.dumpGoodOnes(resultDirName);

    std::this_thread::sleep_for(std::chrono::seconds(5));
  }

  for (int index = 0; index < threads.size(); ++index) {
    threads[index].join();
  }

  commonState.dumpGoodOnes(resultDirName);

  commonState.printStatus();
}

void GraphController::setupStartRunner() {
  GraphPathPool tempPool;

  commonState.logAddedPaths(0, 1);

  auto startPathIndex = tempPool.generateNewIndex();
  auto startPath = tempPool.getGraphPath(startPathIndex);

  // Note that we start at index 1 since start (index 0) already has been visited
  unsigned char minimumEndDistance = 0;
  for (int index = 1; index < VERTICES_COUNT; ++index) {
    minimumEndDistance += data->getMinimumEntryDistance(index);
  }

  startPath->initialize(0, 0, minimumEndDistance);

  // This shouldn't make a difference, but just to be sure
  startPath->setPreviousPath(startPathIndex);
  startPath->setNextPath(startPathIndex);

  RunnerInfo startRunnerInfo(std::vector<char>{});

  serializePool(&tempPool, &startRunnerInfo);

  std::list<RunnerInfo> runnerInfos{startRunnerInfo};
  commonState.addRunnerInfos(runnerInfos);
}

bool GraphController::moveOnDistributed(GraphPathPool *pool, RunnerInfo *runnerInfo, unsigned long int pathIndex, GraphPath *path, unsigned long long int visitedVerticesState, int depth) {
  if (!path->hasSetSubPath()) {
    if (visitedVerticesState == ALL_VERTICES_STATE_MASK) {
      path->maybeSetBestEndDistance(pool, path->getMinimumEndDistance());

      commonState.handleFinishedPath(pool, runnerInfo, path);

      return false;
    }

    if (!commonState.makesSenseToInitialize(path)) {
      commonState.logStartedPath(depth);

      return false;
    }

    if (!initializePath(pool, pathIndex, path, visitedVerticesState, depth)) {
      return true; // Should just be because of full pool, so we want to continue
    }
  }

  if (path->isExhausted()) {
    return false;
  }

  if (!commonState.makesSenseToKeep(path, visitedVerticesState)) {
    logRemovedSubPaths(pool, path, depth);

    return false;
  }

  auto focusedSubPathIndex = path->getFocusedSubPath();
  auto focusedSubPath = pool->getGraphPath(focusedSubPathIndex);
  auto subDepth = depth + 1;
  unsigned long long subPathVisitedVerticesState = visitedVerticesState | (1ULL << focusedSubPath->getCurrentVertex());

  bool continueWork = moveOnDistributed(pool, runnerInfo, focusedSubPathIndex, focusedSubPath, subPathVisitedVerticesState, subDepth);
  if (continueWork) {
    if (pool->isFull()) {
      // We want to continue with the same focus next time. It just didn't finish becasue pool was full
      return true;
    }

    path->setFocusedSubPath(focusedSubPath->getNextPath());

    return true;
  } else {
    auto nextSubPathIndex = focusedSubPath->getNextPath();
    auto nextSubPath = pool->getGraphPath(nextSubPathIndex);
    if (nextSubPath == focusedSubPath) {
      path->setFocusedSubPath(0);
    } else {
      auto previousSubPathIndex = focusedSubPath->getPreviousPath();
      auto previousSubPath = pool->getGraphPath(previousSubPathIndex);

      previousSubPath->setNextPath(nextSubPathIndex);
      nextSubPath->setPreviousPath(previousSubPathIndex);

      path->setFocusedSubPath(nextSubPathIndex);
    }

    commonState.logRemovePath(subDepth);

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

        if (nextVertices[endVertexIndex] > 0) {
          if (newDistance < nextVertices[endVertexIndex]) {
            nextVertices[endVertexIndex] = newDistance;
          }
        } else if (hasVisitedVertex(visitedVerticesState, endVertexIndex)) {
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

  commonState.logStartedPath(depth);

  if (subPathsSorted.empty()) {
    path->setFocusedSubPath(0);
  } else {
    path->setFocusedSubPath(subPathsSorted.front().first);
    commonState.logAddedPaths(depth + 1, subPathsSorted.size());
  }

  return true;
}

bool GraphController::meetsCondition(unsigned long long int visitedVerticesState, unsigned long long int condition) {
  return (visitedVerticesState & condition) == condition;
}

bool GraphController::hasVisitedVertex(unsigned long long int visitedVerticesState, unsigned char vertexIndex) {
  return meetsCondition(visitedVerticesState, (1ULL << vertexIndex));
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
      commonState.logStartedPath(subDepth);
    }

    commonState.logRemovePath(subDepth);

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
    std::lock_guard<std::mutex> tempPoolGuard(tempPoolMutex);
    tempPool.reset();

    auto subRunnerInfo = runnerInfo->makeSubRunner(rootPath->getCurrentVertex());
    subRunnerInfo.setHighScore(pool->getGraphPath(subRootPathIndex)->getBestEndDistance());

    movePathData(pool, &tempPool, subRootPathIndex, 0);

    runnerInfos.push_back(subRunnerInfo);
    serializePool(&tempPool, &subRunnerInfo);

    subRootPathIndex = pool->getGraphPath(subRootPathIndex)->getNextPath();
    if (subRootPathIndex == rootPath->getFocusedSubPath()) {
      break;
    }
  }

  // Wait until here, since the runnerInfos are available to all threads after this
  commonState.splitAndRemoveActiveRunnerInfo(runnerInfo, runnerInfos);
}

std::pair<unsigned long int, GraphPath *> GraphController::movePathData(GraphPathPool *sourcePool, GraphPathPool *destinationPool, unsigned long int sourcePathIndex, unsigned long int destinationParentPathIndex) {
  auto sourcePath = sourcePool->getGraphPath(sourcePathIndex);

  auto destinationPathIndex = destinationPool->generateNewIndex();
  auto destinationPath = destinationPool->getGraphPath(destinationPathIndex);

  destinationPath->initializeAsCopy(sourcePath, destinationParentPathIndex);

  if (sourcePath->hasSetSubPath()) {

    auto sourceSubPathStartIndex = sourcePath->getFocusedSubPath();

    auto sourceSubPathIndex = sourceSubPathStartIndex;
    unsigned long int destinationSubPathStartIndex, destinationSubPathPreviousIndex, destinationSubPathCurrentIndex;
    GraphPath *destinationSubPathStart = nullptr, *destinationSubPathPrevious = nullptr, *destinationSubPathCurrent;
    while (true) {
      std::tie(destinationSubPathCurrentIndex, destinationSubPathCurrent) = movePathData(sourcePool, destinationPool, sourceSubPathIndex, destinationPathIndex);

      if (destinationSubPathStart == nullptr) {
        destinationSubPathStart = destinationSubPathCurrent;
        destinationSubPathStartIndex = destinationSubPathCurrentIndex;

        destinationPath->setFocusedSubPath(destinationSubPathCurrentIndex);
      }

      if (destinationSubPathPrevious != nullptr) {
        destinationSubPathCurrent->setPreviousPath(destinationSubPathPreviousIndex);
        destinationSubPathPrevious->setNextPath(destinationSubPathCurrentIndex);
      }

      destinationSubPathPrevious = destinationSubPathCurrent;
      destinationSubPathPreviousIndex = destinationSubPathCurrentIndex;

      // Note: If there's only one subpath next and previous will be the same as current - that's why we don't check in the while-condition
      sourceSubPathIndex = sourcePool->getGraphPath(sourceSubPathIndex)->getNextPath();
      if (sourceSubPathStartIndex == sourceSubPathIndex) {
        destinationSubPathCurrent->setNextPath(destinationSubPathStartIndex);
        destinationSubPathStart->setPreviousPath(destinationSubPathCurrentIndex);
        break;
      }
    }
  }

  return std::make_pair(destinationPathIndex, destinationPath);
}

std::string GraphController::getPoolFileName(unsigned int runnerInfoIdentifier) const {
  return std::string(MEMORY_DUMP_DIR) + "/" + std::to_string(runnerInfoIdentifier) + ".dat";
}

void GraphController::serializePool(GraphPathPool *pool, RunnerInfo *runnerInfo) {
  serializePool(pool, runnerInfo->getIdentifier());
}

void GraphController::serializePool(GraphPathPool *pool, unsigned int runnerInfoIdentifier) {
  std::string fileName = getPoolFileName(runnerInfoIdentifier);
  std::ofstream dumpFile(fileName, std::ios::binary | std::ios::trunc);
  pool->serialize(dumpFile);
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

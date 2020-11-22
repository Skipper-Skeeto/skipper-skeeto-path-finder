#include "skipper-skeeto-path-finder/graph_controller.h"

#include "skipper-skeeto-path-finder/file_helper.h"
#include "skipper-skeeto-path-finder/graph_common_state.h"
#include "skipper-skeeto-path-finder/graph_data.h"
#include "skipper-skeeto-path-finder/graph_path.h"
#include "skipper-skeeto-path-finder/info.h"

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

const char *GraphController::MEMORY_DUMP_DIR = "temp_memory_dump";

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
  FileHelper::createDir(MEMORY_DUMP_DIR);

  GraphPath startPath(0);
  unsigned char visitedVertices = 1;

  std::vector<GraphPath *> nextPaths{&startPath};
  while (visitedVertices < THREAD_DISTRIBUTE_LEVEL) {
    auto parentPaths = nextPaths;
    nextPaths.clear();

    for (const auto &path : parentPaths) {
      initializePath(path);

      if (path->isExhausted() || !commonState.makesSenseToKeep(path)) {
        continue;
      }

      for (int index = 0; index < path->getNextPathsCount(); ++index) {
        nextPaths.push_back(&(*path->getFocusedNextPath()));
        path->bumpFocusedNextPath();
      }
    }

    ++visitedVertices;
  }

  std::cout << "Spawning " << nextPaths.size() << " threads with paths." << std::endl
            << std::endl;

  std::mutex controllerMutex;
  controllerMutex.lock();

  auto threadFunction = [this, &controllerMutex, visitedVertices](GraphPath *path) {
    // Make sure everything is set up correctly before we start computing
    controllerMutex.lock();
    controllerMutex.unlock();

    auto threadInfo = commonState.getCurrentThread();

    while (moveOnDistributed(path, visitedVertices)) {
      if (threadInfo->isPaused()) {
        std::string fileName = std::string(MEMORY_DUMP_DIR) + "/" + std::to_string(threadInfo->getIdentifier()) + ".dat";
        {
          std::ofstream dumpFile(fileName, std::ios::binary | std::ios::trunc);
          path->serialize(dumpFile);
          path->cleanUp();
        }
        threadInfo->waitForUnpaused();
        {
          std::ifstream dumpFile(fileName, std::ios::binary);
          path->deserialize(dumpFile, nullptr);
        }
      }
    }

    commonState.getCurrentThread()->setDone();
  };

  unsigned char nextIdentifier = '0';
  for (const auto &path : nextPaths) {
    std::thread thread(threadFunction, path);
    commonState.addThread(std::move(thread), nextIdentifier);

    if (nextIdentifier == '9') {
      nextIdentifier = 'A';
    } else if (nextIdentifier == 'Z') {
      nextIdentifier = 'a';
    } else if (nextIdentifier == 'z') {
      nextIdentifier = '!';
    } else if (nextIdentifier == '/') {
      nextIdentifier = ':';
    } else if (nextIdentifier == '>') {
      std::cout << "There wasn't enough identifiers for the threads..." << std::endl
                << std::endl;
      return;
    } else if (nextIdentifier != '*') {
      ++nextIdentifier;
    }
  }

  controllerMutex.unlock();

  while (commonState.hasThreads()) {
    commonState.updateThreads();

    commonState.printStatus();

    commonState.dumpGoodOnes(resultDirName);

    std::this_thread::sleep_for(std::chrono::seconds(5));
  }

  commonState.dumpGoodOnes(resultDirName);
}

bool GraphController::moveOnDistributed(GraphPath *path, unsigned char visitedVertices) {
  if (!path->isInitialized()) {
    if (path->isFinished()) {
      commonState.maybeAddNewGoodOne(path);

      return false;
    }

    if (!commonState.makesSenseToInitialize(path)) {
      return false;
    }

    initializePath(path);
  }

  if (path->isExhausted() || !commonState.makesSenseToKeep(path)) {
    return false;
  }

  auto nextVisitedVertices = visitedVertices + 1;

  auto nextPathIterator = path->getFocusedNextPath();
  bool continueWork = moveOnDistributed(&(*nextPathIterator), nextVisitedVertices);
  if (continueWork) {
    if (visitedVertices < DISTRIBUTION_LEVEL_LIMIT) {
      path->bumpFocusedNextPath();
    }

    return true;
  } else {
    path->eraseNextPath(nextPathIterator);

    return !path->isExhausted();
  }
}

void GraphController::initializePath(GraphPath *path) {
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
        } else if (path->hasVisitedVertex(endVertexIndex)) {
          if (visitedVertices[endVertexIndex] < 0 || newDistance < visitedVertices[endVertexIndex]) {
            visitedVertices[endVertexIndex] = newDistance;
            ++unresovedVertices;
          }
        } else if (path->meetsCondition(edge->condition)) {
          nextVertices[endVertexIndex] = newDistance;
        }
      }
    }
  }

  // Insert vertices into list - note that we sort the shortest first
  std::vector<GraphPath> nextVerticesSorted;
  for (char vertexIndex = 0; vertexIndex < VERTICES_COUNT; ++vertexIndex) {
    auto distance = nextVertices[vertexIndex];
    if (distance < 0) {
      continue;
    }

    GraphPath newPath(vertexIndex, path, distance);

    const auto &longerLengthIterator = std::upper_bound(
        nextVerticesSorted.begin(),
        nextVerticesSorted.end(),
        newPath,
        [](const auto &path, const auto &otherPath) {
          return path.getDistance() < otherPath.getDistance();
        });
    nextVerticesSorted.insert(longerLengthIterator, newPath);
  }

  path->initialize(nextVerticesSorted);
}

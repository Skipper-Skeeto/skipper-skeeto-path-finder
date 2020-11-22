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

  while (moveOnDistributed(&startPath)) {
  }
}

bool GraphController::moveOnDistributed(GraphPath *path) {
  if (!path->isInitialized()) {
    if (path->isFinished()) {
      if (commonState.maybeAddNewGoodOne(path)) {
        commonState.dumpGoodOnes(resultDirName);
      }

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

  auto nextPathIterator = path->getFocusedNextPath();
  bool continueWork = moveOnDistributed(&(*nextPathIterator));
  if (continueWork) {
    path->bumpFocusedNextPath();

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

#include "skipper-skeeto-path-finder/graph_common_state.h"

#include "skipper-skeeto-path-finder/file_helper.h"
#include "skipper-skeeto-path-finder/graph_path.h"

#include <fstream>
#include <iomanip>
#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#include <psapi.h>
#endif

const char *GraphCommonState::DUMPED_GOOD_ONES_BASE_DIR = "results";

bool GraphCommonState::makesSenseToInitialize(const GraphPath *path) const {
  // We won't get to here if it's finished so only chance we have is if the missing edges are with zero distance
  return path->getDistance() <= getMaxDistance();
}

bool GraphCommonState::makesSenseToKeep(const GraphPath *path) {
  if (path->getDistance() > getMaxDistance()) {
    // We won't get to here if it's finished so only chance we have is if the missing edges are with zero distance
    return false;
  }

  if (checkForDuplicateState(path->getState(), path->getDistance()) > 0) {
    // If larger than zero, another path was shorter. Note that we could be checking up against our own state
    return false;
  }

  return true;
}

bool GraphCommonState::maybeAddNewGoodOne(const GraphPath *path) {
  auto distance = path->getDistance();

  std::lock_guard<std::mutex> guardFinalState(finalStateMutex);
  if (distance > maxDistance) {
    return false;
  }

  if (distance < maxDistance) {
    maxDistance = distance;
    goodOnes.clear();
    dumpedGoodOnes = 0;
  }

  auto rawRoute = path->getRoute();

  std::array<char, VERTICES_COUNT> route;
  for (unsigned char index = 0; index < rawRoute.size(); ++index) {
    route[index] = rawRoute[index];
  }

  goodOnes.push_back(route);

  std::lock_guard<std::mutex> guardPrint(printMutex);
  std::cout << "Found " << 1 << " new good one(s) with distance " << +distance << std::endl;

  return true;
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

unsigned char GraphCommonState::getMaxDistance() const {
  std::lock_guard<std::mutex> guard(finalStateMutex);
  return maxDistance;
}

int GraphCommonState::checkForDuplicateState(const State &state, unsigned char distance) {
  std::lock_guard<std::mutex> guard(distanceStateMutex);
  int result = -1;

  auto distanceIterator = distanceForState.find(state);
  if (distanceIterator != distanceForState.end()) {
    result = distance - distanceIterator->second;

    if (result >= 0) {
      return result;
    }
  }

  distanceForState[state] = distance;

  return result;
}

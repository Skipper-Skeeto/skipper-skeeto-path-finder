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

void GraphCommonState::maybeAddNewGoodOne(const GraphPath *path) {
  auto distance = path->getDistance();

  std::lock_guard<std::mutex> guardFinalState(finalStateMutex);
  if (distance > maxDistance) {
    return;
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

  auto threadInfo = getCurrentThread();

  threadInfo->setHighScore(distance);

  std::lock_guard<std::mutex> guardPrint(printMutex);
  std::cout << "Found new good one with distance " << +distance << " in thread " << +threadInfo->getIdentifier() << std::endl;
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
  std::lock_guard<std::mutex> threadInfoGuard(threadInfoMutex);
  std::lock_guard<std::mutex> guardFinalState(finalStateMutex);
  std::lock_guard<std::mutex> guardPrint(printMutex);

  std::string isSupposedToRunThreads;
  std::string notYetStartedThreads;
  std::string notYetStoppedThreads;
  for (const auto &info : threadInfos) {
    auto isPaused = info.isPaused();
    auto isWaiting = info.isWaiting();
    if (!isPaused) {
      isSupposedToRunThreads += std::to_string(info.getIdentifier()) + " ";
    }

    if (isPaused != isWaiting) {
      if (isPaused) {
        notYetStoppedThreads += std::to_string(info.getIdentifier()) + " ";
      } else {
        notYetStartedThreads += std::to_string(info.getIdentifier()) + " ";
      }
    }
  }

  std::cout << "Status: " << threadInfos.size() << " threads, found " << goodOnes.size() << " at distance " << +maxDistance << std::endl;
  std::cout << "Expected to run threads: " << isSupposedToRunThreads << "; Not yet started: " << notYetStartedThreads << "; Not yet stopped: " << notYetStoppedThreads << std::endl;
}

void GraphCommonState::addThread(std::thread &&thread, unsigned char identifier) {
  std::lock_guard<std::mutex> threadInfoGuard(threadInfoMutex);
  threadInfos.emplace_back(std::move(thread), identifier);
}

ThreadInfo *GraphCommonState::getCurrentThread() {
  std::lock_guard<std::mutex> threadInfoGuard(threadInfoMutex);
  for (auto &threadInfo : threadInfos) {
    if (threadInfo.getThreadIdentifier() == std::this_thread::get_id()) {
      return &threadInfo;
    }
  }

  std::cout << "Current thread not found" << std::endl;

  return nullptr;
}

void GraphCommonState::updateThreads() {
  std::lock_guard<std::mutex> threadInfoGuard(threadInfoMutex);

  threadInfos.remove_if([](auto &threadInfo) {
    return threadInfo.joinIfDone();
  });

  int allowedThreads = 8;
  int allowedBestThreads = allowedThreads / 2;

  // Note that from a start all threads has the same score so all will be added
  std::list<ThreadInfo *> pickedThreads;
  int worstScore = 0;
  for (auto &threadInfo : threadInfos) {
    auto distance = threadInfo.getHighScore();
    if (pickedThreads.size() < allowedBestThreads) {
      pickedThreads.push_back(&threadInfo);
      if (distance > worstScore) {
        worstScore = distance;
      }
    } else if (distance == worstScore) {
      pickedThreads.push_back(&threadInfo);
    } else if (distance < worstScore) {
      pickedThreads.remove_if([worstScore](ThreadInfo *threadInfo) {
        return threadInfo->getHighScore() == worstScore;
      });

      pickedThreads.push_back(&threadInfo);
      worstScore = 0;
      for (auto threadInfo : pickedThreads) {
        if (threadInfo->getHighScore() > worstScore) {
          worstScore = threadInfo->getHighScore();
        }
      }
    }
  }

  if (++lastRunningExtraThread >= threadInfos.size()) {
    lastRunningExtraThread = 0;
  }

  auto extraIndex = lastRunningExtraThread;
  while (pickedThreads.size() < allowedThreads && pickedThreads.size() < threadInfos.size()) {
    auto threadInfo = &(*std::next(threadInfos.begin(), extraIndex));
    if (std::find(pickedThreads.begin(), pickedThreads.end(), threadInfo) == pickedThreads.end()) {
      pickedThreads.push_back(threadInfo);
    }

    if (++extraIndex >= threadInfos.size()) {
      extraIndex = 0;
    }
  }

  for (auto &threadInfo : threadInfos) {
    if (std::find(pickedThreads.begin(), pickedThreads.end(), &threadInfo) != pickedThreads.end()) {
      threadInfo.setPaused(false);
    } else {
      threadInfo.setPaused(true);
    }
  }
}

bool GraphCommonState::hasThreads() const {
  std::lock_guard<std::mutex> threadInfoGuard(threadInfoMutex);

  return !threadInfos.empty();
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

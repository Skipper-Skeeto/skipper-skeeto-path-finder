#include "skipper-skeeto-path-finder/common_state.h"

#include "skipper-skeeto-path-finder/action.h"
#include "skipper-skeeto-path-finder/file_helper.h"
#include "skipper-skeeto-path-finder/path.h"
#include "skipper-skeeto-path-finder/room.h"
#include "skipper-skeeto-path-finder/sub_path.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#include <psapi.h>
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

  auto newState = Path::getStateWithRoom(originPath->getState(), subPath->getLastRoomIndex());
  if (checkForDuplicateState(newState, visitedRoomsCount) >= 0) {
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

  if (checkForDuplicateState(path->getState(), path->getVisitedRoomsCount()) >= 0) {
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

bool CommonState::makesSenseToContinueExistingPath(const Path *path) {
  if (checkForDuplicateState(path->getState(), path->getVisitedRoomsCount()) > 0) {
    std::lock_guard<std::mutex> guard(statisticsMutex);
    tooManyStateStepsCount += 1;
    tooManyStateStepsDepthTotal += path->depth;
    return false;
  }

  return true;
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

  std::lock_guard<std::mutex> threadInfoGuard(threadInfoMutex);
  std::vector<unsigned char> runningThreads;
  std::vector<unsigned char> pausedThreads;

  for (const auto &threadInfo : threadInfos) {
    if (threadInfo.isPaused()) {
      pausedThreads.push_back(threadInfo.getIdentifier());
    } else {
      runningThreads.push_back(threadInfo.getIdentifier());
    }
  }

  std::lock_guard<std::mutex> guardPrint(printMutex);

  std::cout << "Threads; Running (" << runningThreads.size() << "): ";
  for (auto threadIdentifier : runningThreads) {
    std::cout << threadIdentifier;
  }
  std::cout << " --- Paused (" << pausedThreads.size() << "): ";
  for (auto threadIdentifier : pausedThreads) {
    std::cout << threadIdentifier;
  }

  std::cout << std::endl
            << "good: " << goodOnes.size() << "; max: " << int(maxVisitedRoomsCount)
            << "; general: " << std::setw(8) << tooManyGeneralStepsCount << "=" << std::fixed << std::setprecision(8) << depthKillGeneralAvg
            << "; state: " << std::setw(8) << tooManyStateStepsCount << "=" << std::fixed << std::setprecision(8) << depthKillStateAvg
            << "; memory: " << getWorkingSetBytes()
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

  FileHelper::createDir(DUMPED_GOOD_ONES_BASE_DIR);
  FileHelper::createDir(dirPath.c_str());

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

void CommonState::addThread(std::thread &&thread, unsigned char identifier) {
  std::lock_guard<std::mutex> threadInfoGuard(threadInfoMutex);
  threadInfos.emplace_back(std::move(thread), identifier);
}

ThreadInfo *CommonState::getCurrentThread() {
  std::lock_guard<std::mutex> threadInfoGuard(threadInfoMutex);
  for (auto &threadInfo : threadInfos) {
    if (threadInfo.getThreadIdentifier() == std::this_thread::get_id()) {
      return &threadInfo;
    }
  }

  std::cout << "Current thread not found" << std::endl;

  return nullptr;
}

void CommonState::updateThreads() {
  std::lock_guard<std::mutex> threadInfoGuard(threadInfoMutex);

  threadInfos.remove_if([](auto &threadInfo) {
    return threadInfo.joinIfDone();
  });

  auto memory = getWorkingSetBytes();

  if (memory < 1000000000) {
    return;
  }

  int allowedThreads = 8;
  if (memory > 8000000000) {
    allowedThreads = 4;
  } else if (memory > 4000000000) {
    allowedThreads = 6;
  }

  int allowedBestThreads = allowedThreads / 2;

  // Note that from a start all threads has the same score so all will be added
  std::list<ThreadInfo *> pickedThreads;
  int worstScore = 0;
  for (auto &threadInfo : threadInfos) {
    auto visitedRoomsCount = threadInfo.getHighScore();
    if (pickedThreads.size() < allowedBestThreads) {
      pickedThreads.push_back(&threadInfo);
      if (visitedRoomsCount > worstScore) {
        worstScore = visitedRoomsCount;
      }
    } else if (visitedRoomsCount == worstScore) {
      pickedThreads.push_back(&threadInfo);
    } else if (visitedRoomsCount < worstScore) {
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

bool CommonState::hasThreads() const {
  std::lock_guard<std::mutex> threadInfoGuard(threadInfoMutex);

  return !threadInfos.empty();
}

unsigned char CommonState::getMaxVisitedRoomsCount() const {
  std::lock_guard<std::mutex> guard(finalStateMutex);
  return maxVisitedRoomsCount;
}

int CommonState::checkForDuplicateState(const State &state, unsigned char visitedRoomsCount) {
  std::lock_guard<std::mutex> guard(stepStageMutex);
  int result = -1;

  auto stepsIterator = stepsForState.find(state);
  if (stepsIterator != stepsForState.end()) {
    result = visitedRoomsCount - stepsIterator->second;

    if (result >= 0) {
      return result;
    }
  }

  stepsForState[state] = visitedRoomsCount;

  return result;
}

void CommonState::addNewGoodOnes(const std::vector<std::vector<const Action *>> &stepsOfSteps, int visitedRoomsCount) {
  std::lock_guard<std::mutex> guardFinalState(finalStateMutex);
  if (visitedRoomsCount > maxVisitedRoomsCount) {
    return;
  }

  if (visitedRoomsCount < maxVisitedRoomsCount) {
    maxVisitedRoomsCount = visitedRoomsCount;
    goodOnes.clear();
    dumpedGoodOnes = 0;
  }

  for (const auto &steps : stepsOfSteps) {
    goodOnes.push_back(steps);
  }

  auto threadInfo = getCurrentThread();

  threadInfo->setHighScore(visitedRoomsCount);

  std::lock_guard<std::mutex> guardPrint(printMutex);
  std::cout << "Found " << stepsOfSteps.size() << " new good one(s) with " << visitedRoomsCount << " rooms in thread " << threadInfo->getIdentifier() << std::endl;
}

size_t CommonState::getWorkingSetBytes() {
#ifdef _WIN32
  PROCESS_MEMORY_COUNTERS_EX pMemCountr;
  if (GetProcessMemoryInfo(GetCurrentProcess(),
                           (PPROCESS_MEMORY_COUNTERS)&pMemCountr,
                           sizeof(PROCESS_MEMORY_COUNTERS))) {

    return pMemCountr.WorkingSetSize;
  }
#endif

  return 0;
}
#include "skipper-skeeto-path-finder/path_controller.h"

#include "skipper-skeeto-path-finder/common_state.h"
#include "skipper-skeeto-path-finder/file_helper.h"
#include "skipper-skeeto-path-finder/sub_path.h"
#include "skipper-skeeto-path-finder/sub_path_info_remaining.h"
#include "skipper-skeeto-path-finder/thread_info.h"

#include <iostream>

#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <thread>

const char *PathController::MEMORY_DUMP_DIR = "temp_memory_dump";

PathController::PathController(const Data *data) {
  this->data = data;
  this->commonState = new CommonState();

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

PathController::~PathController() {
  delete this->commonState;
}

void PathController::start() {
  FileHelper::createDir(MEMORY_DUMP_DIR);

  Path startPath(data->getStartRoom());

  performPossibleActions(&startPath);

  auto threadFunction = [this](Path *path) {
    auto threadInfo = commonState->getCurrentThread();

    //moveOnRecursive(path);
    while (moveOnDistributed(path)) {
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
          path->deserialize(dumpFile);
        }
      }
    }
  };

  distributeToThreads({&startPath}, {}, threadFunction);
}

void PathController::printResult() const {

  auto goodOnes = commonState->getGoodOnes();
  for (int index = 0; index < goodOnes.size(); ++index) {
    std::cout << std::endl
              << "PATH #" << index + 1 << ":" << std::endl;
    for (const auto &step : goodOnes[index]) {
      std::cout << step << std::endl;
    }
    std::cout << std::endl;
  }
}

void PathController::moveOnRecursive(Path *path) {
  while (findNewPath(path)) {
    auto nextPathIterator = path->subPathInfo.getNextPath();
    moveOnRecursive(*nextPathIterator);
    path->subPathInfo.erase(nextPathIterator);
  }
}

bool PathController::moveOnDistributed(Path *path) {
  if (!path->subPathInfo.empty() && !commonState->makesSenseToContinueExistingPath(path)) {
    return false;
  }

  bool foundNew = findNewPath(path);

  if (foundNew && path->depth <= SubPathInfo::MAX_DEPTH) {

    // In case it's one of the depths we are monitoring,
    // we want to get the total paths as fast as possible
    while (findNewPath(path)) {
    }

    if (path->depth == SubPathInfo::MAX_DEPTH) {
      return true;
    }
  }

  if (path->subPathInfo.empty()) {
    return false;
  }

  auto nextPathIterator = path->subPathInfo.getNextPath();
  bool continueWork = moveOnDistributed(*nextPathIterator);
  if (!continueWork) {
    path->subPathInfo.erase(nextPathIterator);
  }

  return true;
}

void PathController::distributeToThreads(const std::vector<Path *> paths, std::vector<Path *> parentPaths, const std::function<void(Path *)> &threadFunction) {
  if (paths.size() < 20) {
    std::vector<Path *> newPaths;

    for (const auto &path : paths) {
      while (findNewPath(path)) {
        newPaths.push_back(*path->subPathInfo.getNextPath());
      }

      parentPaths.push_back(path);
    }

    distributeToThreads(newPaths, parentPaths, threadFunction);

    return;
  }

  std::cout << "Spawning " << paths.size() << " threads with paths." << std::endl
            << std::endl;

  std::mutex controllerMutex;
  auto handleParentThreadFunction = [this, threadFunction, &parentPaths, &controllerMutex](Path *path) {

    // Make sure everything is set up correctly before we start computing
    controllerMutex.lock();
    controllerMutex.unlock();

    threadFunction(path);

    std::lock_guard<std::mutex> parentGuard(controllerMutex);
    auto nodePath = path;
    while (!parentPaths.empty() && nodePath->subPathInfo.empty()) {
      auto parentIterator = std::find(parentPaths.begin(), parentPaths.end(), nodePath);
      if (parentIterator != parentPaths.end()) {
        parentPaths.erase(parentIterator);
      }
      for (auto potentialParent : parentPaths) {
        bool erased = potentialParent->subPathInfo.erase(nodePath);
        if (erased) {
          nodePath = potentialParent;
          break;
        }
      }
    }

    commonState->getCurrentThread()->setDone();
  };

  controllerMutex.lock();

  unsigned char nextIdentifier = '0';
  for (const auto &path : paths) {
    std::thread thread(handleParentThreadFunction, path);
    commonState->addThread(std::move(thread), nextIdentifier);

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

  while (commonState->hasThreads()) {
    commonState->updateThreads();

    commonState->printStatus();

    commonState->dumpGoodOnes(resultDirName);

    std::this_thread::sleep_for(std::chrono::seconds(5));
  }

  commonState->dumpGoodOnes(resultDirName);
}

bool PathController::findNewPath(Path *originPath) {
  if (originPath->subPathInfo.remaining == nullptr) {
    return false;
  }

  while (originPath->subPathInfo.remaining->remainingUnfinishedSubPaths.size() > 0) {
    auto subPath = originPath->subPathInfo.remaining->remainingUnfinishedSubPaths.back();
    if (originPath->subPathInfo.remaining->nextRoomIndexesForFirstSubPath.empty()) {
      const Room *currentRoom;
      if (subPath.isEmpty()) {
        auto currentRoomIndex = originPath->getCurrentRoomIndex();

        currentRoom = data->getRoom(currentRoomIndex);

        if (originPath->hasPostRoom()) {
          auto tasks = data->getTasksForRoom(currentRoom);
          for (const auto &task : tasks) {
            if (task->postRoom != nullptr) {
              currentRoom = task->postRoom;
            }
          }
        }

        originPath->subPathInfo.remaining->unavailableRooms[currentRoomIndex] = true;
      } else {
        currentRoom = data->getRoom(subPath.getLastRoomIndex());
      }

      originPath->subPathInfo.remaining->nextRoomIndexesForFirstSubPath = currentRoom->getNextRoomIndexes();
    }

    auto nextRoomIndexIterator = originPath->subPathInfo.remaining->nextRoomIndexesForFirstSubPath.begin();
    const Room *nextRoom = data->getRoom(*nextRoomIndexIterator);
    originPath->subPathInfo.remaining->nextRoomIndexesForFirstSubPath.erase(nextRoomIndexIterator);

    if (originPath->subPathInfo.remaining->nextRoomIndexesForFirstSubPath.empty()) {

      // After this iteration we should skip to next subPath
      originPath->subPathInfo.remaining->remainingUnfinishedSubPaths.pop_back();
    }

    if (originPath->subPathInfo.remaining->unavailableRooms[nextRoom->roomIndex]) {
      // Room already reached or cannot be entered.
      // We cannot have gotten here faster than the other steps getting here.
      continue;
    }

    originPath->subPathInfo.remaining->unavailableRooms[nextRoom->roomIndex] = true;

    auto enterRoomResult = canEnterRoom(originPath, nextRoom);
    if (enterRoomResult == EnterRoomResult::CannotEnter) {
      continue;
    }

    SubPath newSubPath(nextRoom, subPath);

    if (!commonState->makesSenseToPerformActions(originPath, &newSubPath)) {
      continue;
    }

    if (enterRoomResult == EnterRoomResult::CanEnterWithTaskObstacle) {
      Path newPath = originPath->createFromSubPath(&newSubPath);
      newPath.completeTask(nextRoom->taskObstacle);

      performPossibleActions(&newPath);

      if (submitIfDone(&newPath)) {
        continue;
      }

      if (commonState->makesSenseToStartNewSubPath(&newPath)) {
        originPath->subPathInfo.push_back(std::move(newPath));
        return true;
      }

      continue;

    } else if (enterRoomResult != EnterRoomResult::CanEnter) {
      throw std::runtime_error("Unknown enter room result!");
    }

    auto possibleTasks = getPossibleTasks(originPath, nextRoom);
    if (!possibleTasks.empty()) {
      Path newPath = originPath->createFromSubPath(&newSubPath);

      performPossibleActions(&newPath, possibleTasks);

      if (submitIfDone(&newPath)) {
        continue;
      }

      if (commonState->makesSenseToStartNewSubPath(&newPath)) {
        originPath->subPathInfo.push_back(std::move(newPath));
        return true;
      }

      continue;
    }

    auto possibleItems = getPossibleItems(originPath, nextRoom);
    if (!possibleItems.empty()) {
      Path newPath = originPath->createFromSubPath(&newSubPath);
      newPath.pickUpItems(possibleItems);

      if (submitIfDone(&newPath)) {
        continue;
      }

      if (commonState->makesSenseToStartNewSubPath(&newPath)) {
        originPath->subPathInfo.push_back(std::move(newPath));
        return true;
      }

      continue;
    }

    if (!commonState->makesSenseToExpandSubPath(originPath, &newSubPath)) {
      continue;
    }

    originPath->subPathInfo.remaining->remainingUnfinishedSubPaths.insert(originPath->subPathInfo.remaining->remainingUnfinishedSubPaths.begin(), std::move(newSubPath));
  }

  delete originPath->subPathInfo.remaining;
  originPath->subPathInfo.remaining = nullptr;

  return false;
}

PathController::EnterRoomResult PathController::canEnterRoom(const Path *path, const Room *room) const {
  if (room->taskObstacle == nullptr) {
    return EnterRoomResult::CanEnter;
  }

  if (path->hasCompletedTask(room->taskObstacle)) {
    return EnterRoomResult::CanEnter;
  }

  if (canCompleteTask(path, room->taskObstacle)) {
    return EnterRoomResult::CanEnterWithTaskObstacle;
  } else {
    return EnterRoomResult::CannotEnter;
  }
}

void PathController::performPossibleActions(Path *path) {
  auto currentRoom = data->getRoom(path->getCurrentRoomIndex());
  performPossibleActions(path, getPossibleTasks(path, currentRoom));
}

void PathController::performPossibleActions(Path *path, const std::vector<const Task *> &possibleTasks) {
  std::vector<const Task *> postRoomTasks;
  for (const auto &task : possibleTasks) {
    if (task->postRoom != nullptr) {
      postRoomTasks.push_back(task);
    } else {
      path->completeTask(task);
    }
  }

  auto currentRoom = data->getRoom(path->getCurrentRoomIndex());
  path->pickUpItems(getPossibleItems(path, currentRoom));

  for (const auto &task : postRoomTasks) {
    path->completeTask(task);
    path->setPostRoomState(true);

    // We moved room, if there was more than one in postRoomTasks it would be for the wrong room!
    // TODO: To make this more generic, if there could be more than one post room, split into seperate paths
    return;
  }
}

std::vector<const Task *> PathController::getPossibleTasks(const Path *path, const Room *room) const {
  std::vector<const Task *> possibleTasks;

  for (const auto &task : data->getTasksForRoom(room)) {
    if (!path->hasCompletedTask(task) && canCompleteTask(path, task)) {
      possibleTasks.push_back(task);
    }
  }

  return possibleTasks;
}

std::vector<const Item *> PathController::getPossibleItems(const Path *path, const Room *room) const {
  std::vector<const Item *> possibleItems;

  for (const auto &item : data->getItemsForRoom(room)) {
    if (!path->hasFoundItem(item) && canPickUpItem(path, item)) {
      possibleItems.push_back(item);
    }
  }

  return possibleItems;
}

bool PathController::canCompleteTask(const Path *path, const Task *task) const {
  if (task->taskObstacle != nullptr) {
    if (!path->hasCompletedTask(task->taskObstacle)) {
      return false;
    }
  }

  return std::all_of(
      task->itemsNeeded.begin(),
      task->itemsNeeded.end(),
      [&path](Item *item) {
        return path->hasFoundItem(item);
      });
}

bool PathController::canPickUpItem(const Path *path, const Item *item) const {
  if (item->taskObstacle == nullptr) {
    return true;
  }

  return path->hasCompletedTask(item->taskObstacle);
}

bool PathController::submitIfDone(const Path *path) {
  if (!path->isDone()) {
    return false;
  }

  auto stepsOfSteps = findFinalSteps(path);
  commonState->addNewGoodOnes(stepsOfSteps, path->getVisitedRoomsCount());

  return true;
}

std::vector<std::vector<const Action *>> PathController::findFinalSteps(const Path *finalPath) const {
  std::vector<std::vector<const Action *>> stepsOfSteps{{}};
  auto route = finalPath->getRoute();
  auto lastPath = route.front();
  auto lastRoom = data->getRoom(lastPath->getCurrentRoomIndex());

  Path startPath(data->getStartRoom());
  std::tie(stepsOfSteps, lastRoom) = performFinalStepsActions(stepsOfSteps, &startPath, lastRoom);

  for (auto path : route) {
    if (path->getCurrentRoomIndex() != lastRoom->roomIndex) {
      auto targetRoom = data->getRoom(path->getCurrentRoomIndex());
      stepsOfSteps = moveToFinalStepsRoom(stepsOfSteps, lastPath, lastRoom, targetRoom);
      lastRoom = targetRoom;
    }

    std::tie(stepsOfSteps, lastRoom) = performFinalStepsActions(stepsOfSteps, lastPath, lastRoom);

    lastPath = path;
  }

  return stepsOfSteps;
}

std::vector<std::vector<const Action *>> PathController::moveToFinalStepsRoom(const std::vector<std::vector<const Action *>> &currentStepsOfSteps, const Path *currentPath, const Room *currentRoom, const Room *targetRoom) const {
  std::vector<std::vector<const Room *>> nextSubRoutes{{}};
  bool foundRoom = false;

  while (!foundRoom) {
    auto remaningSubRoutes = nextSubRoutes;
    nextSubRoutes.clear();

    while (!remaningSubRoutes.empty()) {
      auto nextRemaningSubRoute = remaningSubRoutes.back();
      remaningSubRoutes.pop_back();

      std::vector<const Room *> nextPossibleRooms;
      if (nextRemaningSubRoute.empty()) {
        nextPossibleRooms = currentRoom->getNextRooms();
      } else {
        nextPossibleRooms = nextRemaningSubRoute.back()->getNextRooms();
      }

      for (const auto &room : nextPossibleRooms) {
        auto newSubRoute = nextRemaningSubRoute;
        auto enterRoomResult = canEnterRoom(currentPath, room);
        if (room == targetRoom) {
          newSubRoute.push_back(room);
          nextSubRoutes.push_back(newSubRoute);
          foundRoom = true;
        } else if (enterRoomResult == EnterRoomResult::CanEnter) {
          newSubRoute.push_back(room);
          nextSubRoutes.push_back(newSubRoute);
        }
      }
    }
  }

  std::vector<std::vector<const Action *>> newStepsOfSteps;

  for (const auto &subRoute : nextSubRoutes) {
    if (subRoute.back() != targetRoom) {
      continue;
    }

    for (auto newSteps : currentStepsOfSteps) {
      for (const auto &room : subRoute) {
        newSteps.push_back(room);
      }

      newStepsOfSteps.push_back(newSteps);
    }
  }

  return newStepsOfSteps;
}

std::pair<std::vector<std::vector<const Action *>>, const Room *> PathController::performFinalStepsActions(const std::vector<std::vector<const Action *>> &currentStepsOfSteps, const Path *currentPath, const Room *currentRoom) const {
  auto newStepsOfSteps = currentStepsOfSteps;
  auto newRoom = currentRoom;

  if (canEnterRoom(currentPath, currentRoom) == EnterRoomResult::CanEnterWithTaskObstacle) {
    for (auto &steps : newStepsOfSteps) {
      steps.push_back(currentRoom->taskObstacle);
    }
  }

  const Task *postRoomTask = nullptr;
  for (const auto &task : getPossibleTasks(currentPath, currentRoom)) {
    if (task == currentRoom->taskObstacle) {
      continue;
    }

    if (task->postRoom != nullptr) {
      postRoomTask = task;
    } else {
      for (auto &steps : newStepsOfSteps) {
        steps.push_back(task);
      }
    }
  }

  for (const auto &item : getPossibleItems(currentPath, currentRoom)) {
    for (auto &steps : newStepsOfSteps) {
      steps.push_back(item);
    }
  }

  if (postRoomTask != nullptr) {
    for (auto &steps : newStepsOfSteps) {
      steps.push_back(postRoomTask);
      steps.push_back(postRoomTask->postRoom);
    }

    newRoom = postRoomTask->postRoom;
  }

  return std::make_pair(newStepsOfSteps, newRoom);
}

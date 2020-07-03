#include "skipper-skeeto-path-finder/path_controller.h"

#include "skipper-skeeto-path-finder/common_state.h"

#include <iostream>

#include <ctime>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <thread>

PathController::PathController(const Data *data) {
  this->data = data;
  this->commonState = new CommonState();

  std::time_t currentTime = std::time(nullptr);
  std::ostringstream stringStream;
  std::tm localTime{};
#ifdef _WIN32
  localtime_s(&localTime, &currentTime);
#else
#error "Converting to locale time was not implemented for this platform"
#endif
  stringStream << std::put_time(&localTime, "%Y%m%d-%H%M%S");
  resultDirName = stringStream.str();
}

PathController::~PathController() {
  delete this->commonState;
}

void PathController::start() {
  Path startPath(data->getItems(), data->getTasks(), data->getStartRoom());

  performPossibleActions(&startPath);

  auto threadFunction = [this](Path *path) {
    //moveOnRecursive(path);
    while (moveOnDistributed(path)) {
    }
  };

  distributeToThreads({&startPath}, threadFunction);
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
  bool foundNew = findNewPath(path);

  if (foundNew && path->depth <= SubPathInfo::MAX_DEPTH) {

    // In case it's one of the depths we are monitoring,
    // we want to get the total paths as fast as possible
    while (findNewPath(path)) {
    }

    return true;
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

void PathController::distributeToThreads(const std::vector<Path *> paths, const std::function<void(Path *)> &threadFunction) {
  if (paths.size() > 10) {
    std::cout << "Spawning " << paths.size() << " threads with paths." << std::endl
              << std::endl;

    std::vector<std::thread> threads;
    for (const auto &path : paths) {
      std::thread thread(threadFunction, path);
      threads.push_back(std::move(thread));
    }

    auto printer = [this]() {
      while (!commonState->shouldStop()) {
        commonState->printStatus();

        commonState->dumpGoodOnes(resultDirName);

        std::this_thread::sleep_for(std::chrono::seconds(5));
      }
    };
    std::thread printerThread(printer);

    for (auto &thread : threads) {
      thread.join();
    }

    commonState->stop();

    printerThread.join();

    commonState->dumpGoodOnes(resultDirName);

    return;
  }

  std::vector<Path *> newPaths;

  for (const auto &path : paths) {
    while (findNewPath(path)) {
      newPaths.push_back(*path->subPathInfo.getNextPath());
    }
  }

  distributeToThreads(newPaths, threadFunction);
}

bool PathController::findNewPath(Path *originPath) {
  while (originPath->subPathInfo.remainingUnfinishedSubPaths.size() > 0) {
    std::vector<const Room *> subPath = originPath->subPathInfo.remainingUnfinishedSubPaths.front();
    if (originPath->subPathInfo.nextRoomsForFirstSubPath.empty()) {
      const Room *currentRoom;
      if (subPath.empty()) {
        currentRoom = originPath->getCurrentRoom();

        originPath->subPathInfo.unavailableRooms[originPath->getCurrentRoom()->roomIndex] = true;
      } else {
        currentRoom = subPath.back();
      }

      originPath->subPathInfo.nextRoomsForFirstSubPath = currentRoom->getNextRooms();
    }

    const Room *nextRoom = originPath->subPathInfo.nextRoomsForFirstSubPath.front();
    originPath->subPathInfo.nextRoomsForFirstSubPath.erase(originPath->subPathInfo.nextRoomsForFirstSubPath.begin());

    if (originPath->subPathInfo.nextRoomsForFirstSubPath.empty()) {

      // After this iteration we should skip to next subPath
      originPath->subPathInfo.remainingUnfinishedSubPaths.pop_front();
    }

    if (originPath->subPathInfo.unavailableRooms[nextRoom->roomIndex]) {
      // Room already reached or cannot be entered.
      // We cannot have gotten here faster than the other steps getting here.
      continue;
    }

    originPath->subPathInfo.unavailableRooms[nextRoom->roomIndex] = true;

    auto enterRoomResult = canEnterRoom(originPath, nextRoom);
    if (enterRoomResult == EnterRoomResult::CannotEnter) {
      continue;
    }

    std::vector<const Room *> newSubPath(subPath);
    newSubPath.push_back(nextRoom);

    if (!commonState->makesSenseToPerformActions(originPath, newSubPath)) {
      continue;
    }

    if (enterRoomResult == EnterRoomResult::CanEnterWithTaskObstacle) {
      Path newPath = originPath->createFromSubPath(newSubPath);
      newPath.completeTask(nextRoom->taskObstacle);

      performPossibleActions(&newPath);

      if (commonState->submitIfDone(&newPath)) {
        continue;
      }

      if (commonState->makesSenseToStartNewSubPath(&newPath)) {
        originPath->subPathInfo.push_back(std::move(newPath));
        return true;
      }

      continue;

    } else if (enterRoomResult != EnterRoomResult::CanEnter) {
      throw std::exception("Unknown enter room result!");
    }

    auto possibleTasks = getPossibleTasks(originPath, nextRoom);
    if (!possibleTasks.empty()) {
      Path newPath = originPath->createFromSubPath(newSubPath);

      performPossibleActions(&newPath, possibleTasks);

      if (commonState->submitIfDone(&newPath)) {
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
      Path newPath = originPath->createFromSubPath(newSubPath);
      newPath.pickUpItems(possibleItems);

      if (commonState->submitIfDone(&newPath)) {
        continue;
      }

      if (commonState->makesSenseToStartNewSubPath(&newPath)) {
        originPath->subPathInfo.push_back(std::move(newPath));
        return true;
      }

      continue;
    }

    if (!commonState->makesSenseToExpandSubPath(originPath, newSubPath)) {
      continue;
    }

    originPath->subPathInfo.remainingUnfinishedSubPaths.push_back(newSubPath);
  }

  return false;
}

PathController::EnterRoomResult PathController::canEnterRoom(const Path *path, const Room *room) {
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
  performPossibleActions(path, getPossibleTasks(path, path->getCurrentRoom()));
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

  path->pickUpItems(getPossibleItems(path, path->getCurrentRoom()));

  for (const auto &task : postRoomTasks) {
    path->completeTask(task);
    path->enterRoom(task->postRoom);

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
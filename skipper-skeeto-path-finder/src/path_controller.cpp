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

  moveOnDistributeRecursive({&startPath});
  //moveOnRecursive(&startPath);
}

void PathController::printResult() const {

  auto goodOnes = commonState->getGoodOnes();
  for (int index = 0; index < goodOnes.size(); ++index) {
    std::cout << std::endl
              << "PATH #" << index + 1 << ":" << std::endl;
    for (const auto &step : goodOnes[index].getSteps()) {
      std::cout << step << std::endl;
    }
    std::cout << std::endl;
  }
}

void PathController::moveOnRecursive(Path *path) {
  while (findNewPath(path)) {
    auto nextPathIterator = path->subPathInfo.getNextPath();
    moveOnRecursive(&*nextPathIterator);
    path->subPathInfo.erase(nextPathIterator);
  }
}

bool PathController::moveOnDistributed(Path *path) {
  findNewPath(path);

  if (path->subPathInfo.empty()) {
    return false;
  }

  auto nextPathIterator = path->subPathInfo.getNextPath();
  bool continueWork = moveOnDistributed(&*nextPathIterator);
  if (!continueWork) {
    path->subPathInfo.erase(nextPathIterator);
  }

  return true;
}

void PathController::moveOnDistributeRecursive(const std::vector<Path *> paths) {
  if (paths.size() > 10) {
    std::cout << "Found " << paths.size() << " paths. Doing recursive threading" << std::endl
              << std::endl;

    std::vector<std::thread> threads;
    auto startWithRecursive = [this](Path *path) {
      moveOnRecursive(path);
      //while (moveOnDistributed(path)) {
      //}
    };
    for (const auto &path : paths) {
      std::thread thread(startWithRecursive, path);
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

    return;
  }

  std::vector<Path *> newPaths;

  for (const auto &path : paths) {
    while (findNewPath(path)) {
      newPaths.push_back(&*path->subPathInfo.getNextPath());
    }
  }

  moveOnDistributeRecursive(newPaths);
}

bool PathController::findNewPath(Path *originPath) {
  while (originPath->subPathInfo.remainingUnfinishedSubPaths.size() > 0) {
    std::vector<const Room *> subPath = originPath->subPathInfo.remainingUnfinishedSubPaths.front();
    if (originPath->subPathInfo.nextRoomsForFirstSubPath.empty()) {
      const Room *currentRoom;
      if (subPath.empty()) {
        currentRoom = originPath->getCurrentRoom();

        originPath->subPathInfo.availableRooms = data->getRooms();
        originPath->subPathInfo.availableRooms.erase(std::find(
            originPath->subPathInfo.availableRooms.begin(),
            originPath->subPathInfo.availableRooms.end(),
            originPath->getCurrentRoom()));
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

    auto availableRoomIterator = std::find(
        originPath->subPathInfo.availableRooms.begin(),
        originPath->subPathInfo.availableRooms.end(),
        nextRoom);
    if (availableRoomIterator == originPath->subPathInfo.availableRooms.end()) {
      // Room already reached or cannot be entered.
      // We cannot have gotten here faster than the other steps getting here.
      continue;
    }

    const Room *avaiableRoom = *availableRoomIterator;
    originPath->subPathInfo.availableRooms.erase(availableRoomIterator);

    auto enterRoomResult = canEnterRoom(originPath, avaiableRoom);
    if (enterRoomResult == EnterRoomResult::CannotEnter) {
      continue;
    }

    std::vector<const Room *> newSubPath(subPath);
    newSubPath.push_back(avaiableRoom);

    if (!commonState->makesSenseToPerformActions(originPath, newSubPath)) {
      continue;
    }

    if (enterRoomResult == EnterRoomResult::CanEnterWithTaskObstacle) {
      Path newPath = originPath->createFromSubPath(newSubPath);
      newPath.completeTask(avaiableRoom->taskObstacle);

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

    auto possibleTasks = getPossibleTasks(originPath, avaiableRoom);
    if (!possibleTasks.empty()) {
      Path newPath = originPath->createFromSubPath(newSubPath);
      newPath.completeTasks(possibleTasks);

      newPath.pickUpItems(getPossibleItems(&newPath, avaiableRoom));

      if (commonState->submitIfDone(&newPath)) {
        continue;
      }

      if (commonState->makesSenseToStartNewSubPath(&newPath)) {
        originPath->subPathInfo.push_back(std::move(newPath));
        return true;
      }

      continue;
    }

    auto possibleItems = getPossibleItems(originPath, avaiableRoom);
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
  path->completeTasks(getPossibleTasks(path, path->getCurrentRoom()));
  path->pickUpItems(getPossibleItems(path, path->getCurrentRoom()));
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
#include "skipper-skeeto-path-finder/path_controller.h"

#include <iomanip>
#include <iostream>

PathController::PathController(const Data *data) {
  this->data = data;
}

void PathController::start() {
  Path startPath(data->getItems(), data->getTasks(), data->getStartRoom());

  performPossibleActions(&startPath);

  moveOnRecursive(&startPath);
}

void PathController::printResult() const {

  for (int index = 0; index < goodOnes.size(); ++index) {
    std::cout << std::endl
              << "PATH #" << index + 1 << ":" << std::endl;
    for (const auto &step : goodOnes[index].getSteps()) {
      std::cout << step << std::endl;
    }
    std::cout << std::endl;
  }
}

void PathController::moveOnRecursive(const Path *path) {
  if (submitIfDone(path)) {
    return;
  }

  forkPaths(path, [this](Path path) {
    path.depth++;
    moveOnRecursive(&path);
  });
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
  for (const auto &task : path->getRemainingTasksForRoom(path->getCurrentRoom())) {
    if (canCompleteTask(path, task)) {
      path->completeTask(task);
    }
  }

  for (const auto &item : path->getRemainingItemsForRoom(path->getCurrentRoom())) {
    if (canPickUpItem(path, item)) {
      path->pickUpItem(item);
    }
  }
}

bool PathController::canCompleteTask(const Path *path, const Task *task) {
  return std::all_of(
      task->itemsNeeded.begin(),
      task->itemsNeeded.end(),
      [&path](Item *item) {
        return path->hasFoundItem(item);
      });
}

bool PathController::canPickUpItem(const Path *path, const Item *item) {
  if (item->taskObstacle == nullptr) {
    return true;
  }

  return path->hasCompletedTask(item->taskObstacle);
}

bool PathController::makesSenseToMoveOn(const Path *path) {
  if (path->isDone()) {
    return true;
  }

  if (path->getStepCount() >= maxStepCount) {
    tooManyGeneralStepsCount += 1;
    tooManyGeneralStepsDepthTotal += path->depth;

    maybePrint();

    return false;
  }

  // TODO: If direction ever get an impact (e.g. double-left is good), this might has to check count <= count
  auto state = path->getState();
  auto &roomStates = stepsForStage[path->getCurrentRoom()];
  auto stepsIterator = roomStates.find(state);
  if (stepsIterator != roomStates.end() && stepsIterator->second < path->getStepCount()) {
    tooManyStateStepsCount += 1;
    tooManyStateStepsDepthTotal += path->depth;

    maybePrint();

    return false;
  }

  // TODO: Doesn't need to be set if count == count
  roomStates[state] = path->getStepCount();

  return true;
}

bool PathController::submitIfDone(const Path *path) {
  if (!path->isDone()) {
    return false;
  }

  if (path->getStepCount() < maxStepCount) {
    maxStepCount = path->getStepCount();
    goodOnes.clear();
  }

  std::cout << "Found new good with " << path->getStepCount() << " steps (depth " << path->depth << ")" << std::endl;

  goodOnes.push_back(*path);
  return true;
}

void PathController::maybePrint() {
  auto now = std::chrono::system_clock::now();
  if (now - lastPrintTime < std::chrono::seconds(5)) {
    return;
  }

  double depthKillGeneralAvg = 0;
  if (tooManyGeneralStepsCount != 0) {
    depthKillGeneralAvg = (double)tooManyGeneralStepsDepthTotal / tooManyGeneralStepsCount;
  }

  double depthKillStateAvg = 0;
  if (tooManyStateStepsCount != 0) {
    depthKillStateAvg = (double)tooManyStateStepsDepthTotal / tooManyStateStepsCount;
  }

  std::cout << "good: " << goodOnes.size() << "; max: " << maxStepCount
            << "; general: " << std::setw(8) << tooManyGeneralStepsCount << "=" << std::fixed << std::setprecision(8) << depthKillGeneralAvg
            << "; state: " << std::setw(8) << tooManyStateStepsCount << "=" << std::fixed << std::setprecision(8) << depthKillStateAvg
            << std::endl;

  lastPrintTime = now;
}
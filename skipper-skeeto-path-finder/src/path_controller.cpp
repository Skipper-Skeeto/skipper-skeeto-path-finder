#include "skipper-skeeto-path-finder/path_controller.h"

#include "skipper-skeeto-path-finder/common_state.h"

#include <iostream>

#include <mutex>
#include <thread>

PathController::PathController(const Data *data) {
  this->data = data;
  this->commonState = new CommonState();
}

PathController::~PathController() {
  delete this->commonState;
}

void PathController::start() {
  Path startPath(data->getItems(), data->getTasks(), data->getStartRoom());

  performPossibleActions(&startPath);

  moveOnDistributeRecursive({startPath});
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

void PathController::moveOnRecursive(const Path *path) {
  forkPaths(path, [this](Path path) {
    moveOnRecursive(&path);
  });
}

void PathController::moveOnDistributeRecursive(const std::vector<Path> paths) {
  if (paths.size() > 10) {
    std::cout << "Found " << paths.size() << " paths. Doing recursive threading" << std::endl
              << std::endl;

    std::vector<std::thread> threads;
    auto startWithRecursive = [this](const Path path) {
      moveOnRecursive(&path);
    };
    for (const auto &path : paths) {
      std::thread thread(startWithRecursive, path);
      threads.push_back(std::move(thread));
    }

    auto printer = [this]() {
      while (!commonState->shouldStop()) {
        commonState->printStatus();

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

  std::vector<Path> newPaths;
  for (const auto &path : paths) {
    forkPaths(&path, [&newPaths](Path path) {
      newPaths.push_back(std::move(path));
    });
  }

  moveOnDistributeRecursive(newPaths);
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

  for (const auto &task : path->getRemainingTasksForRoom(room)) {
    if (canCompleteTask(path, task)) {
      possibleTasks.push_back(task);
    }
  }

  return possibleTasks;
}

std::vector<const Item *> PathController::getPossibleItems(const Path *path, const Room *room) const {
  std::vector<const Item *> possibleItems;

  for (const auto &item : path->getRemainingItemsForRoom(room)) {
    if (canPickUpItem(path, item)) {
      possibleItems.push_back(item);
    }
  }

  return possibleItems;
}

bool PathController::canCompleteTask(const Path *path, const Task *task) const {
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
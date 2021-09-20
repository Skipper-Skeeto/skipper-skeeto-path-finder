#include "skipper-skeeto-path-finder/path_controller.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>

const char *PathController::MEMORY_DUMP_DIR = "temp_memory_dump";

PathController::PathController(const RawData *rawData, const std::string &resultDir)
    : rawData(rawData), resultDir(resultDir) {
}

void PathController::resolveAndDumpResults(const std::vector<std::array<const Room *, VERTICES_COUNT>> &graphPaths, int expectedLength) const {
  auto startPath = std::make_shared<Path>(rawData->getStartRoom());

  performPossibleActions(startPath);

  std::vector<std::shared_ptr<const Path>> allPaths;
  for (auto graphPath : graphPaths) {
    auto paths = moveOnRecursive(startPath, graphPath, 0);
    for (const auto &path : paths) {
      allPaths.push_back(path);

      if (expectedLength != path->getVisitedRoomsCount()) {
        std::cout << "WARNING: Found path length (" << std::to_string(path->getVisitedRoomsCount()) << ") "
                  << "varies from the expected (" << expectedLength << ")! "
                  << "Found path will be dumped to the expected length." << std::endl;
      }
    }
  }

  dumpResult(allPaths, std::to_string(expectedLength) + ".txt");
}

std::vector<std::shared_ptr<const Path>> PathController::moveOnRecursive(std::shared_ptr<const Path> originPath, const std::array<const Room *, VERTICES_COUNT> &graphPath, int reachedIndex) const {
  if (reachedIndex > graphPath.size() - 2) {
    return std::vector<std::shared_ptr<const Path>>{originPath};
  }

  auto nextIndex = reachedIndex + 1;
  auto targetRoom = graphPath[nextIndex];

  std::vector<std::shared_ptr<const Path>> paths;

  auto subPaths = findPaths(originPath, targetRoom);
  for (auto subPath : subPaths) {
    auto finalPaths = moveOnRecursive(subPath, graphPath, nextIndex);
    for (const auto &finalPath : finalPaths) {
      paths.push_back(finalPath);
    }
  }

  return paths;
}

std::vector<std::shared_ptr<const Path>> PathController::findPaths(std::shared_ptr<const Path> originPath, const Room *targetRoom) const {
  if (originPath->getCurrentRoomIndex() == targetRoom->getUniqueIndex()) {
    return std::vector<std::shared_ptr<const Path>>{originPath};
  }

  std::list<std::shared_ptr<const Path>> remainingPaths;
  std::vector<std::shared_ptr<const Path>> completedPaths;

  if (originPath->hasPostRoom()) {
    auto tasks = rawData->getTasksForRoom(rawData->getRoom(originPath->getCurrentRoomIndex()));
    for (const auto &task : tasks) {
      if (task->getPostRoom() != nullptr) {
        remainingPaths.push_back(originPath->createFromNewRoom(task->getPostRoom()));
      }
    }
  } else {
    remainingPaths.push_back(originPath);
  }

  while (!remainingPaths.empty()) {
    auto path = remainingPaths.front();
    remainingPaths.pop_front();

    if (!completedPaths.empty() && completedPaths.back()->getVisitedRoomsCount() >= path->getVisitedRoomsCount()) {
      continue;
    }

    const Room *currentRoom = rawData->getRoom(path->getCurrentRoomIndex());

    auto nextRoomIndexes = currentRoom->getNextRoomIndexes();
    for (auto nextRoomIndex : nextRoomIndexes) {
      const Room *nextRoom = rawData->getRoom(nextRoomIndex);

      auto enterRoomResult = canEnterRoom(originPath, nextRoom);
      if (enterRoomResult == EnterRoomResult::CannotEnter) {
        continue;
      }

      auto newPath = path->createFromNewRoom(nextRoom);

      if (enterRoomResult == EnterRoomResult::CanEnterWithTaskObstacle) {
        newPath->completeTask(nextRoom->getTaskObstacle());
      } else if (enterRoomResult != EnterRoomResult::CanEnter) {
        throw std::runtime_error("Unknown enter room result!");
      }

      performPossibleActions(newPath);

      if (nextRoomIndex == targetRoom->getUniqueIndex()) {
        completedPaths.push_back(newPath);
      } else {
        remainingPaths.push_back(newPath);
      }
    }
  }

  return completedPaths;
}

PathController::EnterRoomResult PathController::canEnterRoom(std::shared_ptr<const Path> path, const Room *room) const {
  auto taskObstacle = room->getTaskObstacle();
  if (taskObstacle == nullptr) {
    return EnterRoomResult::CanEnter;
  }

  if (path->hasCompletedTask(taskObstacle)) {
    return EnterRoomResult::CanEnter;
  }

  if (taskObstacle->getRoom() == room && canCompleteTask(path, taskObstacle)) {
    return EnterRoomResult::CanEnterWithTaskObstacle;
  } else {
    return EnterRoomResult::CannotEnter;
  }
}

void PathController::performPossibleActions(std::shared_ptr<Path> path) const {
  auto currentRoom = rawData->getRoom(path->getCurrentRoomIndex());
  auto possibleTasks = getPossibleTasks(path, currentRoom);
  auto possibleItems = getPossibleItems(path, currentRoom);

  std::vector<const Task *> postRoomTasks;
  while (!possibleItems.empty() || (possibleTasks.size() - postRoomTasks.size()) > 0) {
    for (const auto &task : possibleTasks) {
      if (task->getPostRoom() != nullptr) {
        postRoomTasks.push_back(task);
      } else {
        path->completeTask(task);
      }
    }

    path->pickUpItems(possibleItems);

    possibleTasks = getPossibleTasks(path, currentRoom);
    possibleItems = getPossibleItems(path, currentRoom);
  }

  for (const auto &task : postRoomTasks) {
    path->completeTask(task);
    path->setPostRoomState(true);

    // We moved room, if there was more than one in postRoomTasks it would be for the wrong room!
    // TODO: To make this more generic, if there could be more than one post room, split into seperate paths
    return;
  }
}

std::vector<const Task *> PathController::getPossibleTasks(std::shared_ptr<const Path> path, const Room *room) const {
  std::vector<const Task *> possibleTasks;

  for (const auto &task : rawData->getTasksForRoom(room)) {
    if (!path->hasCompletedTask(task) && canCompleteTask(path, task)) {
      possibleTasks.push_back(task);
    }
  }

  return possibleTasks;
}

std::vector<const Item *> PathController::getPossibleItems(std::shared_ptr<const Path> path, const Room *room) const {
  std::vector<const Item *> possibleItems;

  for (const auto &item : rawData->getItemsForRoom(room)) {
    if (!path->hasFoundItem(item) && canPickUpItem(path, item)) {
      possibleItems.push_back(item);
    }
  }

  return possibleItems;
}

bool PathController::canCompleteTask(std::shared_ptr<const Path> path, const Task *task) const {
  if (task->getTaskObstacle() != nullptr) {
    if (!path->hasCompletedTask(task->getTaskObstacle())) {
      return false;
    }
  }

  const auto &itemsNeeded = task->getItemsNeeded();
  return std::all_of(
      itemsNeeded.begin(),
      itemsNeeded.end(),
      [&path](const Item *item) {
        return path->hasFoundItem(item);
      });
}

bool PathController::canPickUpItem(std::shared_ptr<const Path> path, const Item *item) const {
  if (item->getTaskObstacle() == nullptr) {
    return true;
  }

  return path->hasCompletedTask(item->getTaskObstacle());
}

void PathController::dumpResult(std::vector<std::shared_ptr<const Path>> paths, const std::string &fileName) const {
  std::string filePath = resultDir + "/" + fileName;

  std::ofstream dumpFile(filePath);
  for (int index = 0; index < paths.size(); ++index) {
    dumpFile << "PATH #" << index + 1 << "(of " << paths.size() << "):" << std::endl;

    auto actions = paths[index]->getAllActions();
    for (auto action : actions) {
      dumpFile << action->getStepDescription() << std::endl;
    }

    dumpFile << std::endl
             << std::endl;
  }

  std::cout << "Dumped " << paths.size() << " paths to " << filePath << std::endl;
}

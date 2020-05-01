#pragma once

#include "data.h"
#include "path.h"
#include "skipper-skeeto-path-finder/common_state.h"

#include <unordered_map>
#include <queue>

class PathController {
public:
  PathController(const Data *data);
  ~PathController();

  void start();

  void printResult() const;

private:
  enum class EnterRoomResult {
    CannotEnter,
    CanEnter,
    CanEnterWithTaskObstacle,
  };

  void moveOnRecursive(const Path *path);

  void moveOnDistributeRecursive(const std::vector<Path> paths);

  template <class NewPathCallback>
  void forkPaths(const Path *originPath, const NewPathCallback &newPathCallback) {
    auto availableRooms = data->getRooms();
    availableRooms.erase(std::find(availableRooms.begin(), availableRooms.end(), originPath->getCurrentRoom()));

    std::queue<std::vector<const Room *>> onGoingPaths;
    onGoingPaths.push({});
    while (onGoingPaths.size() > 0) {
      std::vector<const Room *> subPath = onGoingPaths.front();
      onGoingPaths.pop();

      const Room *currentRoom;
      if (subPath.empty()) {
        currentRoom = originPath->getCurrentRoom();
      } else {
        currentRoom = subPath.back();
      }

      for (const auto &nextRoom : currentRoom->getNextRooms()) {

        auto availableRoomIterator = std::find(availableRooms.begin(), availableRooms.end(), nextRoom);
        if (availableRoomIterator == availableRooms.end()) {
          // Room already reached or cannot be entered.
          // We cannot have gotten here faster than the other steps getting here.
          continue;
        }

        const Room *avaiableRoom = *availableRoomIterator;
        availableRooms.erase(availableRoomIterator);

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
          Path newPath(*originPath);
          newPath.enterRooms(newSubPath);
          newPath.completeTask(avaiableRoom->taskObstacle);

          performPossibleActions(&newPath);

          if (commonState->submitIfDone(&newPath)) {
            continue;
          }

          if (commonState->makesSenseToStartNewSubPath(&newPath)) {
            newPath.depth++;
            newPathCallback(std::move(newPath));
          }

          continue;

        } else if (enterRoomResult != EnterRoomResult::CanEnter) {
          throw std::exception("Unknown enter room result!");
        }

        auto possibleTasks = getPossibleTasks(originPath, avaiableRoom);
        if (!possibleTasks.empty()) {
          Path newPath(*originPath);
          newPath.enterRooms(newSubPath);
          newPath.completeTasks(possibleTasks);

          newPath.pickUpItems(getPossibleItems(&newPath, avaiableRoom));

          if (commonState->submitIfDone(&newPath)) {
            continue;
          }

          if (commonState->makesSenseToStartNewSubPath(&newPath)) {
            newPath.depth++;
            newPathCallback(std::move(newPath));
          }

          continue;
        }

        auto possibleItems = getPossibleItems(originPath, avaiableRoom);
        if (!possibleItems.empty()) {
          Path newPath(*originPath);
          newPath.enterRooms(newSubPath);
          newPath.pickUpItems(possibleItems);

          if (commonState->submitIfDone(&newPath)) {
            continue;
          }

          if (commonState->makesSenseToStartNewSubPath(&newPath)) {
            newPath.depth++;
            newPathCallback(std::move(newPath));
          }

          continue;
        }

        if (!commonState->makesSenseToExpandSubPath(originPath, newSubPath)) {
          continue;
        }

        onGoingPaths.push(newSubPath);
      }
    }
  }

  EnterRoomResult canEnterRoom(const Path *path, const Room *room);

  void performPossibleActions(Path *path);

  std::vector<const Task *> getPossibleTasks(const Path *path, const Room *room) const;

  std::vector<const Item *> getPossibleItems(const Path *path, const Room *room) const;

  bool canCompleteTask(const Path *path, const Task *task) const;

  bool canPickUpItem(const Path *path, const Item *item) const;

  CommonState *commonState;
  const Data *data;
};
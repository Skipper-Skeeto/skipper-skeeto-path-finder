#pragma once

#include "data.h"
#include "path.h"
#include "skipper-skeeto-path-finder/common_state.h"

#include <unordered_map>


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
  void forkPaths(const Path *path, const NewPathCallback &newPathCallback) {
    auto availableRooms = data->getRooms();
    availableRooms.erase(std::find(availableRooms.begin(), availableRooms.end(), path->getCurrentRoom()));

    std::vector<Path> onGoingPaths{*path}; // TODO: Can we avoid copy?
    while (onGoingPaths.size() > 0) {
      std::vector<Path> pathsToContinue;

      for (const auto &path : onGoingPaths) {

        for (const auto &nextRoom : path.getCurrentRoom()->getNextRooms()) {

          auto availableRoomIterator = std::find(availableRooms.begin(), availableRooms.end(), nextRoom);
          if (availableRoomIterator == availableRooms.end()) {
            // Room already reached or cannot be entered.
            // We cannot have gotten here faster than the other steps getting here.
            continue;
          }

          const Room *avaiableRoom = *availableRoomIterator;
          availableRooms.erase(availableRoomIterator);

          auto enterRoomResult = canEnterRoom(&path, avaiableRoom);
          if (enterRoomResult == EnterRoomResult::CannotEnter) {
            continue;
          }

          Path newPath(path);
          newPath.enterRoom(avaiableRoom);

          if (!commonState->makesSenseToPerformActions(&newPath)) {
            continue;
          }

          int startStepCount = newPath.getStepCount();

          if (enterRoomResult == EnterRoomResult::CanEnterWithTaskObstacle) {
            newPath.completeTask(avaiableRoom->taskObstacle);
          } else if (enterRoomResult != EnterRoomResult::CanEnter) {
            throw std::exception("Unknown enter room result!");
          }

          performPossibleActions(&newPath);

          commonState->submitIfDone(&newPath);

          if (!commonState->makesSenseToMoveOn(&newPath)) {
            continue;
          }

          bool anythingDone = newPath.getStepCount() > startStepCount;
          if (anythingDone) {
            newPath.depth++;
            newPathCallback(std::move(newPath));
          } else {
            pathsToContinue.push_back(std::move(newPath));
          }
        }
      }

      onGoingPaths = pathsToContinue;
    }
  }

  EnterRoomResult canEnterRoom(const Path *path, const Room *room);

  void performPossibleActions(Path *path);

  bool canCompleteTask(const Path *path, const Task *task);

  bool canPickUpItem(const Path *path, const Item *item);

  CommonState *commonState;
  const Data *data;
};
#pragma once

#include "data.h"
#include "path.h"

#include <chrono>
#include <unordered_map>

class PathController {
public:
  PathController(const Data *data);

  void start();

  void printResult() const;

private:
  enum class EnterRoomResult {
    CannotEnter,
    CanEnter,
    CanEnterWithTaskObstacle,
  };

  void moveOnRecursive(const Path *path);

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

          if (!makesSenseToMoveOn(&newPath)) {
            continue;
          }

          int startStepCount = newPath.getStepCount();

          if (enterRoomResult == EnterRoomResult::CanEnterWithTaskObstacle) {
            newPath.completeTask(avaiableRoom->taskObstacle);
          } else if (enterRoomResult != EnterRoomResult::CanEnter) {
            throw std::exception("Unknown enter room result!");
          }

          performPossibleActions(&newPath);

          if (!makesSenseToMoveOn(&newPath)) {
            continue;
          }

          bool anythingDone = newPath.getStepCount() > startStepCount;
          if (anythingDone) {
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

  bool makesSenseToMoveOn(const Path *path);

  bool submitIfDone(const Path *path);

  void maybePrint();

  int maxStepCount = 5000;
  std::unordered_map<const Room *, std::unordered_map<std::vector<bool>, int>> stepsForStage;
  std::vector<Path> goodOnes;
  const Data *data;

  int tooManyGeneralStepsCount = 0;
  int tooManyGeneralStepsDepthTotal = 0;
  int tooManyStateStepsCount = 0;
  int tooManyStateStepsDepthTotal = 0;

  std::chrono::time_point<std::chrono::system_clock> lastPrintTime = std::chrono::system_clock::now();
};
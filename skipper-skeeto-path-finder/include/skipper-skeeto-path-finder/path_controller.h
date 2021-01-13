#pragma once

#include "skipper-skeeto-path-finder/common_state.h"
#include "skipper-skeeto-path-finder/path.h"
#include "skipper-skeeto-path-finder/raw_data.h"

#include <functional>
#include <queue>
#include <unordered_map>

class PathController {
public:
  PathController(const RawData *data);
  ~PathController();

  void start();

  void printResult() const;

private:
  static const char *MEMORY_DUMP_DIR;

  enum class EnterRoomResult {
    CannotEnter,
    CanEnter,
    CanEnterWithTaskObstacle,
  };

  void moveOnRecursive(Path *path);

  bool moveOnDistributed(Path *path);

  void distributeToThreads(const std::vector<Path *> paths, std::vector<Path *> parentPaths, const std::function<void(Path *)> &threadFunction);

  bool findNewPath(Path *originPath);

  EnterRoomResult canEnterRoom(const Path *path, const Room *room) const;

  void performPossibleActions(Path *path);

  void performPossibleActions(Path *path, std::vector<const Task *> possibleTasks);

  std::vector<const Task *> getPossibleTasks(const Path *path, const Room *room) const;

  std::vector<const Item *> getPossibleItems(const Path *path, const Room *room) const;

  bool canCompleteTask(const Path *path, const Task *task) const;

  bool canPickUpItem(const Path *path, const Item *item) const;

  bool submitIfDone(const Path *path);

  std::vector<std::vector<const Action *>> findFinalSteps(const Path *finalPath) const;

  std::vector<std::vector<const Action *>> moveToFinalStepsRoom(const std::vector<std::vector<const Action *>> &currentStepsOfSteps, const Path *currentPath, const Room *currentRoom, const Room *targetRoom) const;

  std::pair<std::vector<std::vector<const Action *>>, const Room *> performFinalStepsActions(const std::vector<std::vector<const Action *>> &currentStepsOfSteps, const Path *currentPath, const Room *currentRoom) const;

  CommonState *commonState;
  const RawData *data;

  std::string resultDirName;
};
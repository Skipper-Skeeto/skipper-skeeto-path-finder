#pragma once

#include "skipper-skeeto-path-finder/common_state.h"
#include "skipper-skeeto-path-finder/data.h"
#include "skipper-skeeto-path-finder/path.h"

#include <queue>
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

  void moveOnRecursive(Path *path);

  bool moveOnDistributed(Path *path);

  void moveOnDistributeRecursive(const std::vector<Path *> paths);

  bool findNewPath(Path *originPath);

  EnterRoomResult canEnterRoom(const Path *path, const Room *room);

  void performPossibleActions(Path *path);

  std::vector<const Task *> getPossibleTasks(const Path *path, const Room *room) const;

  std::vector<const Item *> getPossibleItems(const Path *path, const Room *room) const;

  bool canCompleteTask(const Path *path, const Task *task) const;

  bool canPickUpItem(const Path *path, const Item *item) const;

  CommonState *commonState;
  const Data *data;

  std::string resultDirName;
};
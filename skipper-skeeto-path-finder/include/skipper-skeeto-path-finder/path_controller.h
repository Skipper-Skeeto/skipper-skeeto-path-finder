#pragma once

#include "skipper-skeeto-path-finder/path.h"
#include "skipper-skeeto-path-finder/raw_data.h"

class PathController {
public:
  PathController(const RawData *rawData, const std::vector<std::array<const Room *, VERTICES_COUNT>> &graphPaths, const std::string &resultDir);

  void start();

private:
  static const char *MEMORY_DUMP_DIR;

  enum class EnterRoomResult {
    CannotEnter,
    CanEnter,
    CanEnterWithTaskObstacle,
  };

  std::vector<std::shared_ptr<const Path>> moveOnRecursive(std::shared_ptr<const Path> originPath, const std::array<const Room *, VERTICES_COUNT> &graphPath, int reachedIndex);

  std::vector<std::shared_ptr<const Path>> findPaths(std::shared_ptr<const Path> originPath, const Room *targetRoom);

  EnterRoomResult canEnterRoom(std::shared_ptr<const Path> path, const Room *room) const;

  void performPossibleActions(std::shared_ptr<Path> path);

  std::vector<const Task *> getPossibleTasks(std::shared_ptr<const Path> path, const Room *room) const;

  std::vector<const Item *> getPossibleItems(std::shared_ptr<const Path> path, const Room *room) const;

  bool canCompleteTask(std::shared_ptr<const Path> path, const Task *task) const;

  bool canPickUpItem(std::shared_ptr<const Path> path, const Item *item) const;

  void dumpResult(std::vector<std::shared_ptr<const Path>> paths) const;

  const RawData *rawData;
  const std::vector<std::array<const Room *, VERTICES_COUNT>> graphPaths;
  const std::string resultDir;
};
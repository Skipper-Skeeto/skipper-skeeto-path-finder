#pragma once

#include "skipper-skeeto-path-finder/path.h"
#include "skipper-skeeto-path-finder/raw_data.h"

class Vertex;

class PathController {
public:
  PathController(const RawData *rawData, const std::string &resultDir);

  void resolveAndDumpResults(const std::vector<std::array<const Vertex *, VERTICES_COUNT>> &graphPaths, int expectedLength) const;

private:
  static const char *MEMORY_DUMP_DIR;

  enum class EnterRoomResult {
    CannotEnter,
    CanEnter,
    CanEnterWithTaskObstacle,
  };

  std::vector<std::shared_ptr<const Path>> moveOnRecursive(std::shared_ptr<Path> originPath, const std::array<const Vertex *, VERTICES_COUNT> &graphPath, int reachedIndex) const;

  std::vector<std::shared_ptr<Path>> findPaths(std::shared_ptr<Path> originPath, const Vertex *targetVertex) const;

  EnterRoomResult canEnterRoom(std::shared_ptr<const Path> path, const Room *room) const;

  void performPossibleActions(std::shared_ptr<Path> path, const Task *possiblePostRoomTask) const;

  std::vector<const Task *> getPossibleTasks(std::shared_ptr<const Path> path, const Room *room) const;

  std::vector<const Item *> getPossibleItems(std::shared_ptr<const Path> path, const Room *room) const;

  bool canCompleteTask(std::shared_ptr<const Path> path, const Task *task) const;

  bool canPickUpItem(std::shared_ptr<const Path> path, const Item *item) const;

  void dumpResult(std::vector<std::shared_ptr<const Path>> paths, const std::string &fileName) const;

  const RawData *rawData;
  const std::string resultDir;
};

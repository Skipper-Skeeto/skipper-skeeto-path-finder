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

  enum class EnterSceneResult {
    CannotEnter,
    CanEnter,
    CanEnterWithTaskObstacle,
  };

  std::vector<std::shared_ptr<const Path>> moveOnRecursive(std::shared_ptr<Path> originPath, const std::array<const Vertex *, VERTICES_COUNT> &graphPath, int reachedIndex) const;

  std::vector<std::shared_ptr<Path>> findPaths(std::shared_ptr<Path> originPath, const Vertex *targetVertex) const;

  EnterSceneResult canEnterScene(std::shared_ptr<const Path> path, const Scene *scene) const;

  void performPossibleActions(std::shared_ptr<Path> path, const Task *possiblePostSceneTask) const;

  std::vector<const Task *> getPossibleTasks(std::shared_ptr<const Path> path, const Scene *scene) const;

  std::vector<const Item *> getPossibleItems(std::shared_ptr<const Path> path, const Scene *scene) const;

  bool canCompleteTask(std::shared_ptr<const Path> path, const Task *task) const;

  bool canPickUpItem(std::shared_ptr<const Path> path, const Item *item) const;

  void dumpResult(const std::vector<std::pair<std::shared_ptr<const Path>, std::string>> &pathInfos, const std::string &fileName) const;

  const RawData *rawData;
  const std::string resultDir;
};

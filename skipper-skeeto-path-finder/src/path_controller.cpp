#include "skipper-skeeto-path-finder/path_controller.h"

#include "skipper-skeeto-path-finder/vertex.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <mutex>
#include <sstream>
#include <thread>

const char *PathController::MEMORY_DUMP_DIR = "temp_memory_dump";

PathController::PathController(const RawData *rawData, const std::string &resultDir)
    : rawData(rawData), resultDir(resultDir) {
}

void PathController::resolveAndDumpResults(const std::vector<std::array<const Vertex *, VERTICES_COUNT>> &graphPaths, int expectedLength) const {
  auto startPath = std::make_shared<Path>(rawData->getStartScene());

  performPossibleActions(startPath, nullptr);

  std::vector<std::pair<std::shared_ptr<const Path>, std::string>> allPaths;
  for (auto graphPath : graphPaths) {
    auto paths = moveOnRecursive(startPath, graphPath, 0);
    for (const auto &path : paths) {
      std::string warning;
      if (expectedLength != path->getVisitedScenesCount()) {
        std::stringstream warningStream;
        warningStream << "WARNING: Found path length (" << std::to_string(path->getVisitedScenesCount()) << ") "
                      << "varies from the expected (" << expectedLength << ")!";

        warning = warningStream.str();

        std::cout << warning << " Found path will be dumped to the expected length." << std::endl;
      }

      allPaths.push_back(std::make_pair(path, warning));
    }
  }

  dumpResult(allPaths, std::to_string(expectedLength) + ".txt");
}

std::vector<std::shared_ptr<const Path>> PathController::moveOnRecursive(std::shared_ptr<Path> originPath, const std::array<const Vertex *, VERTICES_COUNT> &graphPath, int reachedIndex) const {
  if (reachedIndex > graphPath.size() - 2) {
    return std::vector<std::shared_ptr<const Path>>{originPath};
  }

  auto nextIndex = reachedIndex + 1;
  auto targetVertex = graphPath[nextIndex];

  std::vector<std::shared_ptr<const Path>> paths;

  auto subPaths = findPaths(originPath, targetVertex);
  for (auto subPath : subPaths) {
    auto finalPaths = moveOnRecursive(subPath, graphPath, nextIndex);
    for (const auto &finalPath : finalPaths) {
      paths.push_back(finalPath);
    }
  }

  return paths;
}

std::vector<std::shared_ptr<Path>> PathController::findPaths(std::shared_ptr<Path> originPath, const Vertex *targetVertex) const {
  auto targetScene = targetVertex->furthestScene;
  const Task *postSceneTask = nullptr;
  for (auto task : targetVertex->tasks) {
    if (task->getPostScene() != nullptr) {
      if (postSceneTask != nullptr) {
        throw std::runtime_error("There was more than one post scene task associated with a single vertex!");
      }

      postSceneTask = task;
    }
  }

  if (originPath->getCurrentSceneIndex() == targetScene->getUniqueIndex()) {

    // This is needed as the last iteration might not have dealt with
    // the post scene task if the vertex didn't include it (see reason
    // in performPossibleActions()
    performPossibleActions(originPath, postSceneTask);

    return std::vector<std::shared_ptr<Path>>{originPath};
  }

  std::list<std::shared_ptr<const Path>> remainingPaths;
  std::vector<std::shared_ptr<Path>> completedPaths;

  if (originPath->hasPostScene()) {
    auto tasks = rawData->getTasksForScene(rawData->getScene(originPath->getCurrentSceneIndex()));
    for (const auto &task : tasks) {
      if (task->getPostScene() != nullptr) {
        auto newPath = originPath->createFromNewScene(task->getPostScene());
        if (newPath->getCurrentSceneIndex() == targetScene->getUniqueIndex()) {
          completedPaths.push_back(newPath);
        } else {
          remainingPaths.push_back(newPath);
        }
      }
    }
  } else {
    remainingPaths.push_back(originPath);
  }

  while (!remainingPaths.empty()) {
    auto path = remainingPaths.front();
    remainingPaths.pop_front();

    if (!completedPaths.empty() && completedPaths.back()->getVisitedScenesCount() >= path->getVisitedScenesCount()) {
      continue;
    }

    const Scene *currentScene = rawData->getScene(path->getCurrentSceneIndex());

    auto nextSceneIndexes = currentScene->getNextSceneIndexes();
    for (auto nextSceneIndex : nextSceneIndexes) {
      const Scene *nextScene = rawData->getScene(nextSceneIndex);

      auto enterSceneResult = canEnterScene(path, nextScene);
      if (enterSceneResult == EnterSceneResult::CannotEnter) {
        continue;
      }

      auto newPath = path->createFromNewScene(nextScene);

      if (enterSceneResult == EnterSceneResult::CanEnterWithTaskObstacle) {
        newPath->completeTask(nextScene->getTaskObstacle());
      } else if (enterSceneResult != EnterSceneResult::CanEnter) {
        throw std::runtime_error("Unknown enter scene result!");
      }

      performPossibleActions(newPath, postSceneTask);

      if (nextSceneIndex == targetScene->getUniqueIndex()) {
        completedPaths.push_back(newPath);
      } else {
        remainingPaths.push_back(newPath);
      }
    }
  }

  return completedPaths;
}

PathController::EnterSceneResult PathController::canEnterScene(std::shared_ptr<const Path> path, const Scene *scene) const {
  auto taskObstacle = scene->getTaskObstacle();
  if (taskObstacle == nullptr) {
    return EnterSceneResult::CanEnter;
  }

  if (path->hasCompletedTask(taskObstacle)) {
    return EnterSceneResult::CanEnter;
  }

  if (taskObstacle->getScene() == scene && canCompleteTask(path, taskObstacle)) {
    return EnterSceneResult::CanEnterWithTaskObstacle;
  } else {
    return EnterSceneResult::CannotEnter;
  }
}

void PathController::performPossibleActions(std::shared_ptr<Path> path, const Task *possiblePostSceneTask) const {
  auto currentScene = rawData->getScene(path->getCurrentSceneIndex());
  auto possibleTasks = getPossibleTasks(path, currentScene);
  auto possibleItems = getPossibleItems(path, currentScene);

  int ignoredPostScenes = 0;
  std::vector<const Task *> postSceneTasks;
  while (!possibleItems.empty() || (possibleTasks.size() - postSceneTasks.size() - ignoredPostScenes) > 0) {
    for (const auto &task : possibleTasks) {
      if (task->getPostScene() != nullptr) {

        // We need to check this as there might be more vertices for one scene
        // and if the vertex we're dealing with currently isn't the one that
        // has a post scene task it could be fatal for the route as we might be
        // lead on a wrong path after solving this task
        if (task == possiblePostSceneTask) {
          postSceneTasks.push_back(task);
        } else {
          ++ignoredPostScenes;
        }
      } else {
        path->completeTask(task);
      }
    }

    path->pickUpItems(possibleItems);

    possibleTasks = getPossibleTasks(path, currentScene);
    possibleItems = getPossibleItems(path, currentScene);
  }

  for (const auto &task : postSceneTasks) {
    path->completeTask(task);
    path->setPostSceneState(true);

    // We moved scene, if there was more than one in postSceneTasks it would be for the wrong scene!
    // TODO: To make this more generic, if there could be more than one post scene, split into seperate paths
    return;
  }
}

std::vector<const Task *> PathController::getPossibleTasks(std::shared_ptr<const Path> path, const Scene *scene) const {
  std::vector<const Task *> possibleTasks;

  for (const auto &task : rawData->getTasksForScene(scene)) {
    if (!path->hasCompletedTask(task) && canCompleteTask(path, task)) {
      possibleTasks.push_back(task);
    }
  }

  return possibleTasks;
}

std::vector<const Item *> PathController::getPossibleItems(std::shared_ptr<const Path> path, const Scene *scene) const {
  std::vector<const Item *> possibleItems;

  for (const auto &item : rawData->getItemsForScene(scene)) {
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

void PathController::dumpResult(const std::vector<std::pair<std::shared_ptr<const Path>, std::string>> &pathInfos, const std::string &fileName) const {
  std::string filePath = resultDir + "/" + fileName;

  std::ofstream dumpFile(filePath);
  for (int index = 0; index < pathInfos.size(); ++index) {
    auto pathInfo = pathInfos[index];

    dumpFile << "PATH #" << index + 1 << "(of " << pathInfos.size() << ")";

    if (!pathInfo.second.empty()) {
      dumpFile << " - (" << pathInfo.second << ")";
    }

    dumpFile << ":" << std::endl;

    auto actions = pathInfo.first->getAllActions();
    for (auto action : actions) {
      dumpFile << action->getStepDescription() << std::endl;
    }

    dumpFile << std::endl
             << std::endl;
  }

  std::cout << "Dumped " << pathInfos.size() << " paths to " << filePath << std::endl;
}

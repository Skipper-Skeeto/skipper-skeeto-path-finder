#include "skipper-skeeto-path-finder/path.h"

#include "skipper-skeeto-path-finder/info.h"
#include "skipper-skeeto-path-finder/item.h"
#include "skipper-skeeto-path-finder/scene.h"
#include "skipper-skeeto-path-finder/task.h"

Path::Path(const Scene *startScene) {
  state = getStateWithScene(state, startScene->getUniqueIndex());
}

std::shared_ptr<Path> Path::createFromNewScene(const Scene *scene) const {
  auto newPath = std::shared_ptr<Path>(new Path());

  newPath->previousPath = shared_from_this();
  newPath->depth++;

  newPath->actions.push_back(scene);
  newPath->enteredScenesCount = enteredScenesCount + 1;
  newPath->state = getStateWithScene(state, scene->getUniqueIndex());
  newPath->foundItems = foundItems;
  newPath->completedTasks = completedTasks;

  return newPath;
}

void Path::pickUpItem(const Item *item) {
  if (hasFoundItem(item)) {
    throw std::runtime_error("Item was already picked up");
  }

  state |= (1ULL << (item->getStateIndex() + STATE_TASK_ITEM_START));
  foundItems |= (1ULL << item->getUniqueIndex());
  actions.push_back(item);
}

void Path::pickUpItems(const std::vector<const Item *> &items) {
  for (const auto &item : items) {
    pickUpItem(item);
  }
}

void Path::completeTask(const Task *task) {
  if (hasCompletedTask(task)) {
    throw std::runtime_error("Task was already completed");
  }

  state |= (1ULL << (task->getStateIndex() + STATE_TASK_ITEM_START));
  completedTasks |= (1ULL << task->getUniqueIndex());
  actions.push_back(task);
}

void Path::completeTasks(const std::vector<const Task *> &tasks) {
  for (const auto &task : tasks) {
    completeTask(task);
  }
}

int Path::getCurrentSceneIndex() const {
  return ((1ULL << STATE_SCENE_INDEX_SIZE) - 1) & state;
}

unsigned char Path::getVisitedScenesCount() const {
  return enteredScenesCount;
}

bool Path::isDone() const {
  return (1ULL << STATE_TASK_ITEM_SIZE) - 1 == (state >> STATE_TASK_ITEM_START);
};

const RawState &Path::getState() const {
  return state;
}

bool Path::hasFoundItem(const Item *item) const {
  return foundItems & (1ULL << item->getUniqueIndex());
}

bool Path::hasCompletedTask(const Task *task) const {
  return completedTasks & (1ULL << task->getUniqueIndex());
}

void Path::setPostSceneState(bool hasPostScene) {
  postScene = hasPostScene;
}

bool Path::hasPostScene() const {
  return postScene;
}

std::vector<const Action *> Path::getAllActions() const {
  if (previousPath != nullptr) {
    auto allActions = previousPath->getAllActions();
    for (auto action : actions) {
      allActions.push_back(action);
    }
    return allActions;
  } else {
    return actions;
  }
}

RawState Path::getStateWithScene(const RawState &state, int sceneIndex) {
  return ((~((1ULL << STATE_SCENE_INDEX_SIZE) - 1) & state) | sceneIndex);
}

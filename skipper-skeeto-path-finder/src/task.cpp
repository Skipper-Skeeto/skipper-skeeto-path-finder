#include "skipper-skeeto-path-finder/task.h"

Task::Task(const std::string &key, int uniqueIndex, const Scene *scene, const std::vector<const Item *> &itemsNeeded, const Scene *postScene)
    : key(key), uniqueIndex(uniqueIndex), scene(scene), itemsNeeded(itemsNeeded), postScene(postScene) {
}

std::string Task::getStepDescription() const {
  return std::string("  - Complete task: ") + key;
}

const std::string &Task::getKey() const {
  return key;
}

int Task::getUniqueIndex() const {
  return uniqueIndex;
}

const Scene *Task::getScene() const {
  return scene;
}

const std::vector<const Item *> &Task::getItemsNeeded() const {
  return itemsNeeded;
}

const Scene *Task::getPostScene() const {
  return postScene;
}

void Task::setStateIndex(int stateIndex) {
  this->stateIndex = stateIndex;
}

int Task::getStateIndex() const {
  return stateIndex;
}

void Task::setTaskObstacle(const Task *taskObstacle) {
  this->taskObstacle = taskObstacle;
}

const Task *Task::getTaskObstacle() const {
  return taskObstacle;
}

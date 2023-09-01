#include "skipper-skeeto-path-finder/item.h"

Item::Item(const std::string &key, int uniqueIndex, const Scene *scene)
    : key(key), uniqueIndex(uniqueIndex), scene(scene) {
}

std::string Item::getStepDescription() const {
  return std::string("  - Pick up: ") + key;
}

const std::string &Item::getKey() const {
  return key;
}

int Item::getUniqueIndex() const {
  return uniqueIndex;
}

const Scene *Item::getScene() const {
  return scene;
}

void Item::setStateIndex(int stateIndex) {
  this->stateIndex = stateIndex;
}

int Item::getStateIndex() const {
  return stateIndex;
}

void Item::setTaskObstacle(const Task *taskObstacle) {
  this->taskObstacle = taskObstacle;
}

const Task *Item::getTaskObstacle() const {
  return taskObstacle;
}

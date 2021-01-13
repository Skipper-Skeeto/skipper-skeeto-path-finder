#include "skipper-skeeto-path-finder/item.h"

Item::Item(const std::string &key, int uniqueIndex, const Room *room)
    : key(key), uniqueIndex(uniqueIndex), room(room) {
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

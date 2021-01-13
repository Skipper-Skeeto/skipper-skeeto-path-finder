#include "skipper-skeeto-path-finder/task.h"

Task::Task(const std::string &key, int uniqueIndex, const Room *room, const std::vector<const Item *> &itemsNeeded, const Room *postRoom)
    : key(key), uniqueIndex(uniqueIndex), room(room), itemsNeeded(itemsNeeded), postRoom(postRoom) {
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

const Room *Task::getRoom() const {
  return room;
}

const std::vector<const Item *> &Task::getItemsNeeded() const {
  return itemsNeeded;
}

const Room *Task::getPostRoom() const {
  return postRoom;
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

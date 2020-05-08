#include "skipper-skeeto-path-finder/path.h"

#include "skipper-skeeto-path-finder/item.h"
#include "skipper-skeeto-path-finder/room.h"
#include "skipper-skeeto-path-finder/task.h"

#include <algorithm>

Path::Path(const std::vector<const Item *> &allItems, const std::vector<const Task *> &allTasks, const Room *startRoom) {
  this->currentRoom = startRoom;

}

Path::Path(const Path &path) {
  currentRoom = path.currentRoom;
  steps = path.steps;
  enteredRoomsCount = path.enteredRoomsCount;
  state = path.state;
  depth = path.depth;

  // Note that we don't copy subPathInfo, since that should be clean
}

Path Path::createFromSubPath(std::vector<const Room *> subPath) const {
  Path path(*this);

  path.depth++;

  path.enterRooms(subPath);

  return std::move(path);
}

void Path::pickUpItem(const Item *item) {
  if (hasFoundItem(item)) {
    throw std::exception("Item was already picked up");
  }

  state[item->uniqueIndex] = true;
  steps.push_back(item);
}

void Path::pickUpItems(const std::vector<const Item *> &items) {
  for (const auto &item : items) {
    pickUpItem(item);
  }
}

void Path::completeTask(const Task *task) {
  if (hasCompletedTask(task)) {
    throw std::exception("Task was already completed");
  }

  state[task->uniqueIndex] = true;
  steps.push_back(task);
}

void Path::completeTasks(const std::vector<const Task *> &tasks) {
  for (const auto &task : tasks) {
    completeTask(task);
  }
}

void Path::enterRoom(const Room *room) {
  steps.push_back(room);
  ++enteredRoomsCount;
  currentRoom = room;
}

void Path::enterRooms(const std::vector<const Room *> rooms) {
  for (const auto &room : rooms) {
    enterRoom(room);
  }
}

const Room *Path::getCurrentRoom() const {
  return currentRoom;
}

int Path::getStepCount() const {
  return int(steps.size());
}

unsigned char Path::getVisitedRoomsCount() const {
  return enteredRoomsCount;
}

bool Path::isDone() const {
  return std::all_of(
      state.begin(),
      state.end(),
      [](bool value) {
        return value;
      });
};

const State &Path::getState() const {
  return state;
}

std::vector<const Action *> Path::getSteps() const {
  return steps;
}

bool Path::hasFoundItem(const Item *item) const {
  return state[item->uniqueIndex];
}

bool Path::hasCompletedTask(const Task *task) const {
  return state[task->uniqueIndex];
}

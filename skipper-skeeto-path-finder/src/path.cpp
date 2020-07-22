#include "skipper-skeeto-path-finder/path.h"

#include "skipper-skeeto-path-finder/info.h"
#include "skipper-skeeto-path-finder/item.h"
#include "skipper-skeeto-path-finder/room.h"
#include "skipper-skeeto-path-finder/sub_path.h"
#include "skipper-skeeto-path-finder/task.h"

Path::Path(const Room *startRoom) {
  state = getStateWithRoom(state, startRoom->roomIndex);
}

Path::Path(const Path &path) {
  previousPath = path.previousPath;
  enteredRoomsCount = path.enteredRoomsCount;
  state = path.state;
  foundItems = path.foundItems;
  completedTasks = path.completedTasks;
  depth = path.depth;

  // Note that we don't copy subPathInfo, since that should be clean
}

Path Path::createFromSubPath(const SubPath *subPath) const {
  Path path(*this);

  path.previousPath = this;
  path.depth++;

  path.enteredRoomsCount += subPath->visitedRoomsCount();
  path.state = getStateWithRoom(state, subPath->getLastRoomIndex());

  return std::move(path);
}

void Path::pickUpItem(const Item *item) {
  if (hasFoundItem(item)) {
    throw std::exception("Item was already picked up");
  }

  state |= (1ULL << item->stateIndex);
  foundItems |= (1ULL << item->uniqueIndex);
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

  state |= (1ULL << task->stateIndex);
  completedTasks |= (1ULL << task->uniqueIndex);
}

void Path::completeTasks(const std::vector<const Task *> &tasks) {
  for (const auto &task : tasks) {
    completeTask(task);
  }
}

int Path::getCurrentRoomIndex() const {
  return ((1ULL << STATE_ROOM_INDEX_SIZE) - 1) & state;
}

unsigned char Path::getVisitedRoomsCount() const {
  return enteredRoomsCount;
}

bool Path::isDone() const {
  return (1ULL << STATE_TASK_ITEM_SIZE) - 1 == (state >> STATE_TASK_ITEM_START);
};

const State &Path::getState() const {
  return state;
}

bool Path::hasFoundItem(const Item *item) const {
  return foundItems & (1ULL << item->uniqueIndex);
}

bool Path::hasCompletedTask(const Task *task) const {
  return completedTasks & (1ULL << task->uniqueIndex);
}

void Path::setPostRoomState(bool hasPostRoom) {
  postRoom = hasPostRoom;
}

bool Path::hasPostRoom() const {
  return postRoom;
}

std::vector<const Path *> Path::getRoute() const {
  if (previousPath != nullptr) {
    auto route = previousPath->getRoute();
    route.push_back(this);
    return route;
  } else {
    return std::vector<const Path *>{this};
  }
}

State Path::getStateWithRoom(const State &state, int roomIndex) {
  return ((~((1ULL << STATE_ROOM_INDEX_SIZE) - 1) & state) | roomIndex);
}

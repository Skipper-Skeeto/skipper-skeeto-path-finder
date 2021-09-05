#include "skipper-skeeto-path-finder/path.h"

#include "skipper-skeeto-path-finder/info.h"
#include "skipper-skeeto-path-finder/item.h"
#include "skipper-skeeto-path-finder/room.h"
#include "skipper-skeeto-path-finder/task.h"

Path::Path(const Room *startRoom) {
  state = getStateWithRoom(state, startRoom->getUniqueIndex());
}

std::shared_ptr<Path> Path::createFromNewRoom(const Room *room) const {
  auto newPath = std::shared_ptr<Path>(new Path());

  newPath->previousPath = shared_from_this();
  newPath->depth++;

  newPath->actions.push_back(room);
  newPath->enteredRoomsCount = enteredRoomsCount + 1;
  newPath->state = getStateWithRoom(state, room->getUniqueIndex());
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

int Path::getCurrentRoomIndex() const {
  return ((1ULL << STATE_ROOM_INDEX_SIZE) - 1) & state;
}

unsigned char Path::getVisitedRoomsCount() const {
  return enteredRoomsCount;
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

void Path::setPostRoomState(bool hasPostRoom) {
  postRoom = hasPostRoom;
}

bool Path::hasPostRoom() const {
  return postRoom;
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

RawState Path::getStateWithRoom(const RawState &state, int roomIndex) {
  return ((~((1ULL << STATE_ROOM_INDEX_SIZE) - 1) & state) | roomIndex);
}

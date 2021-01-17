#include "skipper-skeeto-path-finder/path.h"

#include "skipper-skeeto-path-finder/info.h"
#include "skipper-skeeto-path-finder/item.h"
#include "skipper-skeeto-path-finder/room.h"
#include "skipper-skeeto-path-finder/sub_path.h"
#include "skipper-skeeto-path-finder/task.h"

Path::Path(const Room *startRoom) {
  state = getStateWithRoom(state, startRoom->getUniqueIndex());
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

Path::Path(std::istream &instream, const Path *previousPath) {
  this->previousPath = previousPath;

  deserialize(instream);
}

void Path::deserialize(std::istream &instream) {
  instream.read(reinterpret_cast<char *>(&depth), sizeof(depth));
  instream.read(reinterpret_cast<char *>(&enteredRoomsCount), sizeof(enteredRoomsCount));
  instream.read(reinterpret_cast<char *>(&postRoom), sizeof(postRoom));
  instream.read(reinterpret_cast<char *>(&state), sizeof(state));
  instream.read(reinterpret_cast<char *>(&foundItems), sizeof(foundItems));
  instream.read(reinterpret_cast<char *>(&completedTasks), sizeof(completedTasks));

  subPathInfo.deserialize(instream, this);
}

void Path::serialize(std::ostream &outstream) const {
  outstream.write(reinterpret_cast<const char *>(&depth), sizeof(depth));
  outstream.write(reinterpret_cast<const char *>(&enteredRoomsCount), sizeof(enteredRoomsCount));
  outstream.write(reinterpret_cast<const char *>(&postRoom), sizeof(postRoom));
  outstream.write(reinterpret_cast<const char *>(&state), sizeof(state));
  outstream.write(reinterpret_cast<const char *>(&foundItems), sizeof(foundItems));
  outstream.write(reinterpret_cast<const char *>(&completedTasks), sizeof(completedTasks));

  subPathInfo.serialize(outstream);
}

void Path::cleanUp() {
  subPathInfo.cleanUp();
}

void Path::pickUpItem(const Item *item) {
  if (hasFoundItem(item)) {
    throw std::runtime_error("Item was already picked up");
  }

  state |= (1ULL << (item->getStateIndex() + STATE_TASK_ITEM_START));
  foundItems |= (1ULL << item->getUniqueIndex());
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

std::vector<const Path *> Path::getRoute() const {
  if (previousPath != nullptr) {
    auto route = previousPath->getRoute();
    route.push_back(this);
    return route;
  } else {
    return std::vector<const Path *>{this};
  }
}

RawState Path::getStateWithRoom(const RawState &state, int roomIndex) {
  return ((~((1ULL << STATE_ROOM_INDEX_SIZE) - 1) & state) | roomIndex);
}

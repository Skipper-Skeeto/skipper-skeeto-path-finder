#include "skipper-skeeto-path-finder/data.h"

#include "skipper-skeeto-path-finder/info.h"

#include <sstream>

Data::Data(const nlohmann::json &jsonData) {
  int nextItemIndex = 0;
  int nextTaskIndex = 0;
  int nextRoomIndex = 0;

  for (auto const &jsonRoomMapping : jsonData["rooms"].items()) {
    auto room = Room();
    room.key = jsonRoomMapping.key();
    room.roomIndex = nextRoomIndex++;
    roomMapping[jsonRoomMapping.key()] = room;

    roomToItemsMapping[&roomMapping[jsonRoomMapping.key()]];
    roomToTasksMapping[&roomMapping[jsonRoomMapping.key()]];
  }

  for (auto const &jsonItemMapping : jsonData["items"].items()) {
    auto item = Item();
    item.key = jsonItemMapping.key();
    item.uniqueIndex = nextItemIndex++;

    itemMapping[jsonItemMapping.key()] = item;
  }

  for (auto const &jsonTaskMapping : jsonData["tasks"].items()) {
    auto task = Task();
    task.key = jsonTaskMapping.key();
    task.uniqueIndex = nextTaskIndex++;

    task.room = &roomMapping[jsonTaskMapping.value()["room"]];

    for (auto const &itemKey : jsonTaskMapping.value()["items_needed"]) {
      task.itemsNeeded.push_back(&itemMapping[itemKey]);
    }

    if (jsonTaskMapping.value()["post_room"] != nullptr) {
      task.postRoom = &roomMapping[jsonTaskMapping.value()["post_room"]];
    }

    taskMapping[jsonTaskMapping.key()] = task;
  }

  for (auto const &jsonTaskMapping : jsonData["tasks"].items()) {
    auto &task = taskMapping[jsonTaskMapping.key()];
    roomToTasksMapping[task.room].push_back(&task);

    if (jsonTaskMapping.value()["task_obstacle"] != nullptr) {
      task.taskObstacle = &taskMapping[jsonTaskMapping.value()["task_obstacle"]];
    }
  }

  for (auto const &jsonRoomMapping : jsonData["rooms"].items()) {
    auto &room = roomMapping[jsonRoomMapping.key()];

    if (jsonRoomMapping.value()["task_obstacle"] != nullptr) {
      room.taskObstacle = &taskMapping[jsonRoomMapping.value()["task_obstacle"]];
    }

    Room *left = nullptr;
    Room *right = nullptr;
    Room *up = nullptr;
    Room *down = nullptr;

    if (jsonRoomMapping.value()["left"] != nullptr) {
      left = &roomMapping[jsonRoomMapping.value()["left"]];
    }

    if (jsonRoomMapping.value()["right"] != nullptr) {
      right = &roomMapping[jsonRoomMapping.value()["right"]];
    }

    if (jsonRoomMapping.value()["up"] != nullptr) {
      up = &roomMapping[jsonRoomMapping.value()["up"]];
    }

    if (jsonRoomMapping.value()["down"] != nullptr) {
      down = &roomMapping[jsonRoomMapping.value()["down"]];
    }

    room.setupNextRooms(left, right, up, down);
  }

  for (auto const &jsonItemMapping : jsonData["items"].items()) {
    auto &item = itemMapping[jsonItemMapping.key()];

    item.room = &roomMapping[jsonItemMapping.value()["room"]];
    roomToItemsMapping[item.room].push_back(&item);

    if (jsonItemMapping.value()["task_obstacle"] != nullptr) {
      item.taskObstacle = &taskMapping[jsonItemMapping.value()["task_obstacle"]];
    }
  }

  int nextStateIndex = STATE_TASK_ITEM_START;
  for (const auto &roomTasks : roomToTasksMapping) {
    int noObstacleIndex = -1;
    for (const auto &task : roomTasks.second) {
      if (task->taskObstacle == nullptr && task->itemsNeeded.empty()) {
        if (noObstacleIndex == -1) {
          noObstacleIndex = nextStateIndex++;
        }
        taskMapping[task->key].stateIndex = noObstacleIndex;
      } else {
        taskMapping[task->key].stateIndex = nextStateIndex++;
      }
    }
  }

  for (const auto &roomItems : roomToItemsMapping) {
    int noObstacleIndex = -1;
    if (roomItems.first->taskObstacle != nullptr && roomItems.first->taskObstacle->room == roomItems.first) {
      noObstacleIndex = roomItems.first->taskObstacle->stateIndex;
    }

    for (const auto &item : roomItems.second) {
      if (item->taskObstacle == nullptr) {
        if (noObstacleIndex == -1) {
          noObstacleIndex = nextStateIndex++;
        }
        itemMapping[item->key].stateIndex = noObstacleIndex;
      } else if (item->taskObstacle->room == roomItems.first) {
        itemMapping[item->key].stateIndex = item->taskObstacle->stateIndex;
      } else {
        itemMapping[item->key].stateIndex = nextStateIndex++;
      }
    }
  }

  if (ROOM_COUNT != roomMapping.size()) {
    std::stringstream stringStream;
    stringStream << "Predefined room count (" << ROOM_COUNT << ") did not match the actual (" << roomMapping.size() << ")";
    throw std::exception(stringStream.str().c_str());
  } else {
    for (const auto &roomInfo : roomMapping) {
      rooms[roomInfo.second.roomIndex] = &roomInfo.second;
    }
  }

  if (ITEM_COUNT != itemMapping.size()) {
    std::stringstream stringStream;
    stringStream << "Predefined item count (" << ITEM_COUNT << ") did not match the actual (" << itemMapping.size() << ")";
    throw std::exception(stringStream.str().c_str());
  }

  if (TASK_COUNT != taskMapping.size()) {
    std::stringstream stringStream;
    stringStream << "Predefined task count (" << TASK_COUNT << ") did not match the actual (" << taskMapping.size() << ")";
    throw std::exception(stringStream.str().c_str());
  }

  auto actualStateTaskItemSize = nextStateIndex - STATE_TASK_ITEM_START;
  if (STATE_TASK_ITEM_SIZE != actualStateTaskItemSize) {
    std::stringstream stringStream;
    stringStream << "Predefined state count (" << STATE_TASK_ITEM_SIZE << ") did not match the actual (" << actualStateTaskItemSize << ")";
    throw std::exception(stringStream.str().c_str());
  }

  if (nextStateIndex > 64) {
    std::stringstream stringStream;
    stringStream << "Too many different states to be able to optimize state key. "
                 << "Optimize the state-grouping or roll-back that use list of bools as state key. "
                 << "Found " << nextStateIndex << " unique states";
    throw std::exception(stringStream.str().c_str());
  }

  for (const auto &room : rooms) {
    bool foundPostRoom = false;
    for (const auto &task : getTasksForRoom(room)) {
      if (task->postRoom != nullptr) {
        if (foundPostRoom) {
          std::stringstream stringStream;
          stringStream << "Logic cannot handle more than one task with post-room in a room. "
                       << "This was the case for the room \"" << room->key << "\"! "
                       << "Update PathController if it is needed.";
          throw std::exception(stringStream.str().c_str());
        }

        foundPostRoom = true;
      }
    }
  }
}

const std::array<const Room*, ROOM_COUNT> &Data::getRooms() const {
  return rooms;
}

std::vector<const Item *> Data::getItems() const {
  std::vector<const Item *> items;
  for (const auto &item : itemMapping) {
    items.push_back(&item.second);
  }
  return items;
}

std::vector<const Task *> Data::getTasks() const {
  std::vector<const Task *> tasks;
  for (const auto &task : taskMapping) {
    tasks.push_back(&task.second);
  }
  return tasks;
}

const std::vector<const Item *> &Data::getItemsForRoom(const Room *room) const {
  return roomToItemsMapping.find(room)->second;
}

const std::vector<const Task *> &Data::getTasksForRoom(const Room *room) const {
  return roomToTasksMapping.find(room)->second;
}

const Room *Data::getRoom(int index) const {
  return rooms[index];
}

const Room *Data::getStartRoom() const {
  auto roomIterator = roomMapping.find(std::string("Home"));
  if (roomIterator == roomMapping.end()) {
    throw std::exception("Could not find start room");
  }
  return &roomIterator->second;
}

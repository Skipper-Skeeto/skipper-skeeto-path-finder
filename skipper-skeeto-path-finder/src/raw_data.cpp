#include "skipper-skeeto-path-finder/raw_data.h"

#include "skipper-skeeto-path-finder/info.h"

#include <sstream>

RawData::RawData(const nlohmann::json &jsonData) {
  int nextItemIndex = 0;
  int nextTaskIndex = 0;
  int nextRoomIndex = 0;

  for (auto const &jsonRoomMapping : jsonData["rooms"].items()) {
    auto key = jsonRoomMapping.key();
    auto room = &roomMapping.emplace(key, Room{key, nextRoomIndex++}).first->second;

    roomToItemsMapping[room];
    roomToTasksMapping[room];
  }

  for (auto const &jsonItemMapping : jsonData["items"].items()) {
    auto room = &roomMapping.find(jsonItemMapping.value()["room"])->second;

    auto key = jsonItemMapping.key();
    auto item = &itemMapping.emplace(key, Item{key, nextItemIndex++, room}).first->second;

    roomToItemsMapping[room].push_back(item);
  }

  for (auto const &jsonTaskMapping : jsonData["tasks"].items()) {
    auto room = &roomMapping.find(jsonTaskMapping.value()["room"])->second;

    std::vector<const Item *> itemsNeeded;
    for (auto const &itemKey : jsonTaskMapping.value()["items_needed"]) {
      itemsNeeded.push_back(&itemMapping.find(itemKey)->second);
    }

    Room *postRoom = nullptr;
    if (jsonTaskMapping.value()["post_room"] != nullptr) {
      postRoom = &roomMapping.find(jsonTaskMapping.value()["post_room"])->second;
    }

    auto key = jsonTaskMapping.key();
    auto task = &taskMapping.emplace(key, Task{key, nextTaskIndex++, room, itemsNeeded, postRoom}).first->second;

    roomToTasksMapping[room].push_back(task);
  }

  for (auto const &jsonTaskMapping : jsonData["tasks"].items()) {
    auto &task = taskMapping.find(jsonTaskMapping.key())->second;

    if (jsonTaskMapping.value()["task_obstacle"] != nullptr) {
      task.setTaskObstacle(&taskMapping.find(jsonTaskMapping.value()["task_obstacle"])->second);
    }
  }

  for (auto const &jsonRoomMapping : jsonData["rooms"].items()) {
    auto &room = roomMapping.find(jsonRoomMapping.key())->second;

    if (jsonRoomMapping.value()["task_obstacle"] != nullptr) {
      room.setTaskObstacle(&taskMapping.find(jsonRoomMapping.value()["task_obstacle"])->second);
    }

    std::vector<const Room *> rooms;
    for (auto const &room : jsonRoomMapping.value()["connected_rooms"]) {
      rooms.push_back(&roomMapping.find(room)->second);
    }

    room.setupNextRooms(rooms);
  }

  for (auto const &jsonItemMapping : jsonData["items"].items()) {
    auto &item = itemMapping.find(jsonItemMapping.key())->second;

    if (jsonItemMapping.value()["task_obstacle"] != nullptr) {
      item.setTaskObstacle(&taskMapping.find(jsonItemMapping.value()["task_obstacle"])->second);
    }
  }

  int nextStateIndex = STATE_TASK_ITEM_START;
  for (const auto &roomTasks : roomToTasksMapping) {
    int noObstacleIndex = -1;
    for (const auto &constTask : roomTasks.second) {
      auto &task = taskMapping.find(constTask->getKey())->second;
      if (constTask->getTaskObstacle() == nullptr && constTask->getItemsNeeded().empty()) {
        if (noObstacleIndex == -1) {
          noObstacleIndex = nextStateIndex++;
        }
        task.setStateIndex(noObstacleIndex);
      } else {
        task.setStateIndex(nextStateIndex++);
      }
    }
  }

  for (const auto &roomItems : roomToItemsMapping) {
    int noObstacleIndex = -1;
    auto taskObstacle = roomItems.first->getTaskObstacle();
    if (taskObstacle != nullptr && taskObstacle->getRoom() == roomItems.first) {
      noObstacleIndex = taskObstacle->getStateIndex();
    }

    for (const auto &constItem : roomItems.second) {
      auto taskObstacle = constItem->getTaskObstacle();
      auto &item = itemMapping.find(constItem->getKey())->second;
      if (taskObstacle == nullptr) {
        if (noObstacleIndex == -1) {
          noObstacleIndex = nextStateIndex++;
        }
        item.setStateIndex(noObstacleIndex);
      } else if (taskObstacle->getRoom() == roomItems.first) {
        item.setStateIndex(taskObstacle->getStateIndex());
      } else {
        item.setStateIndex(nextStateIndex++);
      }
    }
  }

  if (ROOM_COUNT != roomMapping.size()) {
    std::stringstream stringStream;
    stringStream << "Predefined room count (" << ROOM_COUNT << ") did not match the actual (" << roomMapping.size() << ")";
    throw std::runtime_error(stringStream.str());
  } else {
    for (const auto &roomInfo : roomMapping) {
      rooms[roomInfo.second.getUniqueIndex()] = &roomInfo.second;
    }
  }

  if (ITEM_COUNT != itemMapping.size()) {
    std::stringstream stringStream;
    stringStream << "Predefined item count (" << ITEM_COUNT << ") did not match the actual (" << itemMapping.size() << ")";
    throw std::runtime_error(stringStream.str());
  }

  if (TASK_COUNT != taskMapping.size()) {
    std::stringstream stringStream;
    stringStream << "Predefined task count (" << TASK_COUNT << ") did not match the actual (" << taskMapping.size() << ")";
    throw std::runtime_error(stringStream.str());
  }

  auto actualStateTaskItemSize = nextStateIndex - STATE_TASK_ITEM_START;
  if (STATE_TASK_ITEM_SIZE != actualStateTaskItemSize) {
    std::stringstream stringStream;
    stringStream << "Predefined state count (" << STATE_TASK_ITEM_SIZE << ") did not match the actual (" << actualStateTaskItemSize << ")";
    throw std::runtime_error(stringStream.str());
  }

  if (nextStateIndex > 64) {
    std::stringstream stringStream;
    stringStream << "Too many different states to be able to optimize state key. "
                 << "Optimize the state-grouping or roll-back that use list of bools as state key. "
                 << "Found " << nextStateIndex << " unique states";
    throw std::runtime_error(stringStream.str());
  }

  for (const auto &room : rooms) {
    bool foundPostRoom = false;
    for (const auto &task : getTasksForRoom(room)) {
      if (task->getPostRoom() != nullptr) {
        if (foundPostRoom) {
          std::stringstream stringStream;
          stringStream << "Logic cannot handle more than one task with post-room in a room. "
                       << "This was the case for the room \"" << room->getKey() << "\"! "
                       << "Update PathController if it is needed.";
          throw std::runtime_error(stringStream.str());
        }

        foundPostRoom = true;
      }
    }
  }
}

const std::array<const Room *, ROOM_COUNT> &RawData::getRooms() const {
  return rooms;
}

std::vector<const Item *> RawData::getItems() const {
  std::vector<const Item *> items;
  for (const auto &item : itemMapping) {
    items.push_back(&item.second);
  }
  return items;
}

std::vector<const Task *> RawData::getTasks() const {
  std::vector<const Task *> tasks;
  for (const auto &task : taskMapping) {
    tasks.push_back(&task.second);
  }
  return tasks;
}

const std::vector<const Item *> &RawData::getItemsForRoom(const Room *room) const {
  return roomToItemsMapping.find(room)->second;
}

const std::vector<const Task *> &RawData::getTasksForRoom(const Room *room) const {
  return roomToTasksMapping.find(room)->second;
}

const Room *RawData::getRoom(int index) const {
  return rooms[index];
}

const Room *RawData::getStartRoom() const {
  auto roomIterator = roomMapping.find(std::string(START_ROOM));
  if (roomIterator == roomMapping.end()) {
    throw std::runtime_error("Could not find start room");
  }
  return &roomIterator->second;
}

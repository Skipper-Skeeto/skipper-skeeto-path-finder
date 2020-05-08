#include "skipper-skeeto-path-finder/data.h"

#include <sstream>

Data::Data(const nlohmann::json &jsonData) {
  int nextActionIndex = 0;
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
    item.uniqueIndex = nextActionIndex++;

    itemMapping[jsonItemMapping.key()] = item;
  }

  for (auto const &jsonTaskMapping : jsonData["tasks"].items()) {
    auto task = Task();
    task.key = jsonTaskMapping.key();
    task.uniqueIndex = nextActionIndex++;

    task.room = &roomMapping[jsonTaskMapping.value()["room"]];

    for (auto const &itemKey : jsonTaskMapping.value()["items_needed"]) {
      task.itemsNeeded.push_back(&itemMapping[itemKey]);
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

  if (ROOM_COUNT != roomMapping.size()) {
    std::stringstream stringStream;
    stringStream << "Predefined room count (" << ROOM_COUNT << ") did not match the actual (" << roomMapping.size() << ")";
    throw std::exception(stringStream.str().c_str());
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
}

std::vector<const Room *> Data::getRooms() const {
  std::vector<const Room *> rooms;
  for (const auto &room : roomMapping) {
    rooms.push_back(&room.second);
  }
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

std::vector<const Item *> Data::getItemsForRoom(const Room *room) const {
  return roomToItemsMapping.find(room)->second;
}

std::vector<const Task *> Data::getTasksForRoom(const Room *room) const {
  return roomToTasksMapping.find(room)->second;
}

const Room *Data::getStartRoom() const {
  auto roomIterator = roomMapping.find(std::string("Home"));
  if (roomIterator == roomMapping.end()) {
    throw std::exception("Could not find start room");
  }
  return &roomIterator->second;
}

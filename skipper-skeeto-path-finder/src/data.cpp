#include "skipper-skeeto-path-finder/data.h"

Data::Data(const nlohmann::json &jsonData) {
  int nextUniqueIndex = 0;

  for (auto const &jsonRoomMapping : jsonData["rooms"].items()) {
    auto room = Room();
    room.key = jsonRoomMapping.key();
    roomMapping[jsonRoomMapping.key()] = room;
  }

  for (auto const &jsonItemMapping : jsonData["items"].items()) {
    auto item = Item();
    item.key = jsonItemMapping.key();
    item.uniqueIndex = nextUniqueIndex++;

    itemMapping[jsonItemMapping.key()] = item;
  }

  for (auto const &jsonTaskMapping : jsonData["tasks"].items()) {
    auto task = Task();
    task.key = jsonTaskMapping.key();
    task.uniqueIndex = nextUniqueIndex++;

    task.room = &roomMapping[jsonTaskMapping.value()["room"]];

    for (auto const &itemKey : jsonTaskMapping.value()["items_needed"]) {
      task.itemsNeeded.push_back(&itemMapping[itemKey]);
    }

    taskMapping[jsonTaskMapping.key()] = task;
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

    if (jsonItemMapping.value()["task_obstacle"] != nullptr) {
      item.taskObstacle = &taskMapping[jsonItemMapping.value()["task_obstacle"]];
    }
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

const Room *Data::getStartRoom() const {
  auto roomIterator = roomMapping.find(std::string("Home"));
  if (roomIterator == roomMapping.end()) {
    throw std::exception("Could not find start room");
  }
  return &roomIterator->second;
}

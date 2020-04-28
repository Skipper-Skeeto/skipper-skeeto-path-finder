#pragma once

#include "item.h"
#include "room.h"
#include "task.h"

#include <string>
#include <unordered_map>
#include <vector>

#include "json/json.hpp"

class Data {
public:
  Data(const nlohmann::json &jsonData);

  std::vector<const Room *> getRooms() const;

  std::vector<const Item *> getItems() const;

  std::vector<const Task *> getTasks() const;

  const Room *getStartRoom() const;

private:
  std::unordered_map<std::string, Room> roomMapping;
  std::unordered_map<std::string, Item> itemMapping;
  std::unordered_map<std::string, Task> taskMapping;
};
#pragma once

#include "skipper-skeeto-path-finder/info.h"
#include "skipper-skeeto-path-finder/item.h"
#include "skipper-skeeto-path-finder/room.h"
#include "skipper-skeeto-path-finder/state.h"
#include "skipper-skeeto-path-finder/task.h"

#include <map>
#include <string>
#include <vector>

#include "json/json.hpp"

class RawData {
public:
  RawData(const nlohmann::json &jsonData);

  const std::array<const Room *, ROOM_COUNT> &getRooms() const;

  std::vector<const Item *> getItems() const;

  std::vector<const Task *> getTasks() const;

  const Item *getItemByKey(const std::string &key) const;

  const Task *getTaskByKey(const std::string &key) const;

  const std::vector<const Item *> &getItemsForRoom(const Room *room) const;

  const std::vector<const Task *> &getTasksForRoom(const Room *room) const;

  const Room *getRoom(int index) const;

  const Room *getRoomByKey(const std::string &key) const;

  const Room *getStartRoom() const;

  std::vector<std::pair<unsigned char, State>> getStatesForRoom(const Room *room) const;

private:
  const Room *startRoom;

  std::map<std::string, Room> roomMapping;
  std::map<std::string, Item> itemMapping;
  std::map<std::string, Task> taskMapping;

  std::map<const Room *, std::vector<const Item *>> roomToItemsMapping;
  std::map<const Room *, std::vector<const Task *>> roomToTasksMapping;

  std::array<const Room *, ROOM_COUNT> rooms;
};
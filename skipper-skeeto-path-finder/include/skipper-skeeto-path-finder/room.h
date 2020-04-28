#pragma once

#include <string>
#include <vector>

class Task;

class Room {
public:
  std::string key;
  Task *taskObstacle = nullptr;

  void setupNextRooms(Room *left, Room *right, Room *up, Room *down);
  std::vector<const Room *> getNextRooms() const;

private:
  std::vector<const Room *> rooms;
};
#pragma once

#include <string>

class Room;
class Task;

class Item {
public:
  int uniqueIndex = -1;
  std::string key;
  Room *room = nullptr;
  Task *taskObstacle = nullptr;
};
#pragma once

#include "skipper-skeeto-path-finder/action.h"

#include <string>

class Room;
class Task;

class Item : public Action {
public:
  virtual std::string getStepDescription() const;

  int uniqueIndex = -1;
  std::string key;
  Room *room = nullptr;
  Task *taskObstacle = nullptr;
};
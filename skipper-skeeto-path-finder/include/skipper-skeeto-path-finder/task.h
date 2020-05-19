#pragma once

#include "skipper-skeeto-path-finder/action.h"

#include <string>
#include <vector>

class Room;
class Item;

class Task : public Action {
public:
  virtual std::string getStepDescription() const;

  int uniqueIndex = -1;
  int stateIndex = -1;
  std::string key;
  Room *room = nullptr;
  Task *taskObstacle = nullptr;
  std::vector<Item *> itemsNeeded;
  Room *postRoom = nullptr;
};
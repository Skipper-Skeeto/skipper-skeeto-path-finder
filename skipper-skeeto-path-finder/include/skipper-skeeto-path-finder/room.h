#pragma once

#include "skipper-skeeto-path-finder/action.h"

#include <string>
#include <vector>

class Task;

class Room : public Action {
public:
  virtual std::string getStepDescription() const;

  std::string key;
  Task *taskObstacle = nullptr;

  void setupNextRooms(Room *left, Room *right, Room *up, Room *down);
  std::vector<const Room *> getNextRooms() const;

private:
  std::vector<const Room *> rooms;
};
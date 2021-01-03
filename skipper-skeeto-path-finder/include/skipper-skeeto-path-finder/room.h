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
  int roomIndex;

  void setupNextRooms(std::vector<const Room *>  rooms);
  std::vector<const Room *> getNextRooms() const;
  std::vector<unsigned char> getNextRoomIndexes() const;

private:
  std::vector<const Room *> rooms;
  std::vector<unsigned char> roomIndexes;
};
#pragma once

#include "skipper-skeeto-path-finder/action.h"

#include <string>
#include <vector>

class Task;

class Room : public Action {
public:
  Room(const std::string &key, int uniqueIndex);

  virtual std::string getStepDescription() const;

  const std::string &getKey() const;

  int getUniqueIndex() const;

  void setTaskObstacle(const Task *taskObstacle);

  const Task *getTaskObstacle() const;

  void setupNextRooms(const std::vector<const Room *> &rooms);

  const std::vector<const Room *> &getNextRooms() const;

  const std::vector<unsigned char> &getNextRoomIndexes() const;

private:
  std::string key;
  int uniqueIndex;
  const Task *taskObstacle = nullptr;
  std::vector<const Room *> rooms;
  std::vector<unsigned char> roomIndexes;
};
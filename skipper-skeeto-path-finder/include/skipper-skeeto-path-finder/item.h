#pragma once

#include "skipper-skeeto-path-finder/action.h"

#include <string>

class Room;
class Task;

class Item : public Action {
public:
  Item(const std::string &key, int uniqueIndex, const Room *room);

  virtual std::string getStepDescription() const;

  const std::string &getKey() const;

  int getUniqueIndex() const;

  const Room *getRoom() const;

  void setStateIndex(int stateIndex);

  int getStateIndex() const;

  void setTaskObstacle(const Task *taskObstacle);

  const Task *getTaskObstacle() const;

private:
  std::string key;
  int uniqueIndex;
  int stateIndex = -1;
  const Room *room;
  const Task *taskObstacle = nullptr;
};
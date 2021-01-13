#pragma once

#include "skipper-skeeto-path-finder/action.h"

#include <string>
#include <vector>

class Room;
class Item;

class Task : public Action {
public:
  Task(const std::string &key, int uniqueIndex, const Room *room, const std::vector<const Item *> &itemsNeeded, const Room *postRoom);

  virtual std::string getStepDescription() const;

  const std::string &getKey() const;

  int getUniqueIndex() const;

  const Room *getRoom() const;

  const std::vector<const Item *> &getItemsNeeded() const;

  const Room *getPostRoom() const;

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
  std::vector<const Item *> itemsNeeded;
  const Room *postRoom;
};
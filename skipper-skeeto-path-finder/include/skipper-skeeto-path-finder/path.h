#pragma once

#include "skipper-skeeto-path-finder/sub_path_info.h"

#include <string>
#include <vector>

using State = std::vector<bool>;

class Item;
class Task;
class Room;

class Path {
public:
  Path(const std::vector<const Item *> &allItems, const std::vector<const Task *> &allTasks, const Room *startRoom);

  Path(const Path &path);

  Path createFromSubPath(std::vector<const Room *> subPath) const;

  void pickUpItem(const Item *item);

  void pickUpItems(const std::vector<const Item *> &items);

  void completeTask(const Task *task);

  void completeTasks(const std::vector<const Task *> &tasks);

  void enterRoom(const Room *room);

  void enterRooms(const std::vector<const Room *> rooms);

  const Room *getCurrentRoom() const;

  int getStepCount() const;

  unsigned char getVisitedRoomsCount() const;

  bool isDone() const;

  const State &getState() const;

  std::vector<const Item *> getRemainingItemsForRoom(const Room *room) const;

  std::vector<const Task *> getRemainingTasksForRoom(const Room *room) const;

  std::vector<std::string> getSteps() const;

  bool hasFoundItem(const Item *item) const;

  bool hasCompletedTask(const Task *task) const;

  unsigned char depth{0};

  SubPathInfo subPathInfo;

private:
  const Room *currentRoom;
  std::vector<const Item *> foundItems;
  std::vector<const Item *> remainingItems;
  std::vector<const Task *> completedTasks;
  std::vector<const Task *> remainingTasks;
  std::vector<std::string> steps;
  unsigned char enteredRoomsCount = 0;
  std::vector<bool> state;
};
#pragma once

#include "skipper-skeeto-path-finder/sub_path_info.h"

#include <array>
#include <vector>

using State = unsigned long long int;

class Action;
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

  int getCurrentRoomIndex() const;

  unsigned char getVisitedRoomsCount() const;

  bool isDone() const;

  const State &getState() const;

  std::vector<const Action *> getSteps() const;

  bool hasFoundItem(const Item *item) const;

  bool hasCompletedTask(const Task *task) const;

  unsigned char depth{0};

  SubPathInfo subPathInfo;

  static State getStateWithRoom(const State &state, const Room *room);

private:
  const Path *previousPath = nullptr;
  std::vector<const Action *> steps;
  unsigned char enteredRoomsCount = 0;
  State state{};
  unsigned long long int foundItems{};
  unsigned long long int completedTasks{};
};
#pragma once

#include "skipper-skeeto-path-finder/sub_path_info.h"

#include <array>
#include <vector>

using State = unsigned long long int;

class Action;
class Item;
class Task;
class Room;
class SubPath;

class Path {
public:
  Path(const Room *startRoom);

  Path(const Path &path);

  Path createFromSubPath(const SubPath *subPath) const;

  void pickUpItem(const Item *item);

  void pickUpItems(const std::vector<const Item *> &items);

  void completeTask(const Task *task);

  void completeTasks(const std::vector<const Task *> &tasks);

  int getCurrentRoomIndex() const;

  unsigned char getVisitedRoomsCount() const;

  bool isDone() const;

  const State &getState() const;

  bool hasFoundItem(const Item *item) const;

  bool hasCompletedTask(const Task *task) const;

  void setPostRoomState(bool hasPostRoom);

  bool hasPostRoom() const;

  std::vector<const Path*> getRoute() const;

  unsigned char depth{0};

  SubPathInfo subPathInfo;

  static State getStateWithRoom(const State &state, const Room *room);

private:
  const Path *previousPath = nullptr;
  unsigned char enteredRoomsCount = 0;
  bool postRoom = false;
  State state{};
  unsigned long long int foundItems{};
  unsigned long long int completedTasks{};
};
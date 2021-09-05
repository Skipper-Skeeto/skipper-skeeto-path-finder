#pragma once

#include <array>
#include <vector>

using RawState = unsigned long long int;

class Action;
class Item;
class Task;
class Room;

class Path : public std::enable_shared_from_this<Path> {
public:
  Path(const Room *startRoom);

  std::shared_ptr<Path> createFromNewRoom(const Room *room) const;

  void pickUpItem(const Item *item);

  void pickUpItems(const std::vector<const Item *> &items);

  void completeTask(const Task *task);

  void completeTasks(const std::vector<const Task *> &tasks);

  int getCurrentRoomIndex() const;

  unsigned char getVisitedRoomsCount() const;

  bool isDone() const;

  const RawState &getState() const;

  bool hasFoundItem(const Item *item) const;

  bool hasCompletedTask(const Task *task) const;

  void setPostRoomState(bool hasPostRoom);

  bool hasPostRoom() const;

  std::vector<const Action *> getAllActions() const;

  unsigned char depth{0};

  static RawState getStateWithRoom(const RawState &state, int roomIndex);

private:
  Path() = default;

  std::shared_ptr<const Path> previousPath;
  std::vector<const Action *> actions;
  unsigned char enteredRoomsCount = 0;
  bool postRoom = false;
  RawState state{};
  unsigned long long int foundItems{};
  unsigned long long int completedTasks{};
};
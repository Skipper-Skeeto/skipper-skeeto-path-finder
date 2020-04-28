#pragma once

#include <string>
#include <vector>

using State = std::vector<bool>;

class Item;
class Task;
class Room;

class Path {
public:
  Path(const std::vector<const Item *> &allItems, const std::vector<const Task *> &allTasks, const Room *startRoom);

  void pickUpItem(const Item *item);

  void completeTask(const Task *task);

  void enterRoom(const Room *room);

  const Room *getCurrentRoom() const;

  int getStepCount() const;

  bool isDone() const;

  const State &getState() const;

  std::vector<const Item *> getRemainingItemsForRoom(const Room *room) const;

  std::vector<const Task *> getRemainingTasksForRoom(const Room *room) const;

  std::vector<std::string> getSteps() const;

  bool hasFoundItem(const Item *item) const;

  bool hasCompletedTask(const Task *task) const;

  int depth{0};

private:
  const Room *currentRoom;
  std::vector<const Item *> foundItems;
  std::vector<const Item *> remainingItems;
  std::vector<const Task *> completedTasks;
  std::vector<const Task *> remainingTasks;
  std::vector<std::string> steps;
  std::vector<bool> state;
};
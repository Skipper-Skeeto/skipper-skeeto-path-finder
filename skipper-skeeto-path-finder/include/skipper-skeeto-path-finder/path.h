#pragma once

#include <array>
#include <memory>
#include <vector>

using RawState = unsigned long long int;

class Action;
class Item;
class Task;
class Scene;

class Path : public std::enable_shared_from_this<Path> {
public:
  Path(const Scene *startScene);

  std::shared_ptr<Path> createFromNewScene(const Scene *scene) const;

  void pickUpItem(const Item *item);

  void pickUpItems(const std::vector<const Item *> &items);

  void completeTask(const Task *task);

  void completeTasks(const std::vector<const Task *> &tasks);

  int getCurrentSceneIndex() const;

  unsigned char getVisitedScenesCount() const;

  bool isDone() const;

  const RawState &getState() const;

  bool hasFoundItem(const Item *item) const;

  bool hasCompletedTask(const Task *task) const;

  void setPostSceneState(bool hasPostScene);

  bool hasPostScene() const;

  std::vector<const Action *> getAllActions() const;

  unsigned char depth{0};

  static RawState getStateWithScene(const RawState &state, int sceneIndex);

private:
  Path() = default;

  std::shared_ptr<const Path> previousPath;
  std::vector<const Action *> actions;
  unsigned char enteredScenesCount = 0;
  bool postScene = false;
  RawState state{};
  unsigned long long int foundItems{};
  unsigned long long int completedTasks{};
};
#pragma once

#include "skipper-skeeto-path-finder/action.h"

#include <string>

class Scene;
class Task;

class Item : public Action {
public:
  Item(const std::string &key, int uniqueIndex, const Scene *scene);

  virtual std::string getStepDescription() const;

  const std::string &getKey() const;

  int getUniqueIndex() const;

  const Scene *getScene() const;

  void setStateIndex(int stateIndex);

  int getStateIndex() const;

  void setTaskObstacle(const Task *taskObstacle);

  const Task *getTaskObstacle() const;

private:
  std::string key;
  int uniqueIndex;
  int stateIndex = -1;
  const Scene *scene;
  const Task *taskObstacle = nullptr;
};
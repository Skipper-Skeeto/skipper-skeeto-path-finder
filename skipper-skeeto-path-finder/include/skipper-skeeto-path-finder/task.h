#pragma once

#include "skipper-skeeto-path-finder/action.h"

#include <string>
#include <vector>

class Scene;
class Item;

class Task : public Action {
public:
  Task(const std::string &key, int uniqueIndex, const Scene *scene, const std::vector<const Item *> &itemsNeeded, const Scene *postScene);

  virtual std::string getStepDescription() const;

  const std::string &getKey() const;

  int getUniqueIndex() const;

  const Scene *getScene() const;

  const std::vector<const Item *> &getItemsNeeded() const;

  const Scene *getPostScene() const;

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
  std::vector<const Item *> itemsNeeded;
  const Scene *postScene;
};
#pragma once

#include "skipper-skeeto-path-finder/action.h"

#include <string>
#include <vector>

class Task;

class Scene : public Action {
public:
  Scene(const std::string &key, int uniqueIndex);

  virtual std::string getStepDescription() const;

  const std::string &getKey() const;

  int getUniqueIndex() const;

  void setTaskObstacle(const Task *taskObstacle);

  const Task *getTaskObstacle() const;

  void setupNextScenes(const std::vector<const Scene *> &scenes);

  const std::vector<const Scene *> &getNextScenes() const;

  const std::vector<unsigned char> &getNextSceneIndexes() const;

private:
  std::string key;
  int uniqueIndex;
  const Task *taskObstacle = nullptr;
  std::vector<const Scene *> scenes;
  std::vector<unsigned char> sceneIndexes;
};
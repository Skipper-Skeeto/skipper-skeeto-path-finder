#include "skipper-skeeto-path-finder/scene.h"

Scene::Scene(const std::string &key, int uniqueIndex)
    : key(key), uniqueIndex(uniqueIndex) {
}

std::string Scene::getStepDescription() const {
  return std::string("- Move to: ") + key;
}

const std::string &Scene::getKey() const {
  return key;
}

int Scene::getUniqueIndex() const {
  return uniqueIndex;
}

void Scene::setTaskObstacle(const Task *taskObstacle) {
  this->taskObstacle = taskObstacle;
}

const Task *Scene::getTaskObstacle() const {
  return taskObstacle;
}

void Scene::setupNextScenes(const std::vector<const Scene *> &scenes) {
  this->scenes = scenes;

  sceneIndexes.clear();
  for (auto scene : scenes) {
    sceneIndexes.push_back(scene->getUniqueIndex());
  }
}

const std::vector<const Scene *> &Scene::getNextScenes() const {
  return scenes;
}

const std::vector<unsigned char> &Scene::getNextSceneIndexes() const {
  return sceneIndexes;
}
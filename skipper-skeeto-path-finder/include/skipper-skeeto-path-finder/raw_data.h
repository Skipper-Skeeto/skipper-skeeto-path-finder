#pragma once

#include "skipper-skeeto-path-finder/info.h"
#include "skipper-skeeto-path-finder/item.h"
#include "skipper-skeeto-path-finder/scene.h"
#include "skipper-skeeto-path-finder/state.h"
#include "skipper-skeeto-path-finder/task.h"

#include <map>
#include <string>
#include <vector>

#include "json/json.hpp"

class RawData {
public:
  RawData(const nlohmann::json &jsonData);

  const std::array<const Scene *, SCENE_COUNT> &getScenes() const;

  std::vector<const Item *> getItems() const;

  std::vector<const Task *> getTasks() const;

  const Item *getItemByKey(const std::string &key) const;

  const Task *getTaskByKey(const std::string &key) const;

  const std::vector<const Item *> &getItemsForScene(const Scene *scene) const;

  const std::vector<const Task *> &getTasksForScene(const Scene *scene) const;

  const Scene *getScene(int index) const;

  const Scene *getSceneByKey(const std::string &key) const;

  const Scene *getStartScene() const;

  std::vector<std::pair<unsigned char, State>> getStatesForScene(const Scene *scene) const;

private:
  const Scene *startScene;

  std::map<std::string, Scene> sceneMapping;
  std::map<std::string, Item> itemMapping;
  std::map<std::string, Task> taskMapping;

  std::map<const Scene *, std::vector<const Item *>> sceneToItemsMapping;
  std::map<const Scene *, std::vector<const Task *>> sceneToTasksMapping;

  std::array<const Scene *, SCENE_COUNT> scenes;
};
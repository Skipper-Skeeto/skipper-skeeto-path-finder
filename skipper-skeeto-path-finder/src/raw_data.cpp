#include "skipper-skeeto-path-finder/raw_data.h"

#include "skipper-skeeto-path-finder/info.h"

#include <sstream>

RawData::RawData(const nlohmann::json &jsonData) {
  int nextItemIndex = 0;
  int nextTaskIndex = 0;
  int nextSceneIndex = 0;

  for (auto const &jsonSceneMapping : jsonData["scenes"].items()) {
    auto key = jsonSceneMapping.key();
    auto scene = &sceneMapping.emplace(key, Scene{key, nextSceneIndex++}).first->second;

    sceneToItemsMapping[scene];
    sceneToTasksMapping[scene];
  }

  for (auto const &jsonItemMapping : jsonData["items"].items()) {
    auto scene = &sceneMapping.find(jsonItemMapping.value()["scene"])->second;

    auto key = jsonItemMapping.key();
    auto item = &itemMapping.emplace(key, Item{key, nextItemIndex++, scene}).first->second;

    sceneToItemsMapping[scene].push_back(item);
  }

  for (auto const &jsonTaskMapping : jsonData["tasks"].items()) {
    auto scene = &sceneMapping.find(jsonTaskMapping.value()["scene"])->second;

    std::vector<const Item *> itemsNeeded;
    for (auto const &itemKey : jsonTaskMapping.value()["items_needed"]) {
      itemsNeeded.push_back(&itemMapping.find(itemKey)->second);
    }

    Scene *postScene = nullptr;
    if (jsonTaskMapping.value()["post_scene"] != nullptr) {
      postScene = &sceneMapping.find(jsonTaskMapping.value()["post_scene"])->second;
    }

    auto key = jsonTaskMapping.key();
    auto task = &taskMapping.emplace(key, Task{key, nextTaskIndex++, scene, itemsNeeded, postScene}).first->second;

    sceneToTasksMapping[scene].push_back(task);
  }

  for (auto const &jsonTaskMapping : jsonData["tasks"].items()) {
    auto &task = taskMapping.find(jsonTaskMapping.key())->second;

    if (jsonTaskMapping.value()["task_obstacle"] != nullptr) {
      task.setTaskObstacle(&taskMapping.find(jsonTaskMapping.value()["task_obstacle"])->second);
    }
  }

  for (auto const &jsonSceneMapping : jsonData["scenes"].items()) {
    auto &scene = sceneMapping.find(jsonSceneMapping.key())->second;

    if (jsonSceneMapping.value()["task_obstacle"] != nullptr) {
      scene.setTaskObstacle(&taskMapping.find(jsonSceneMapping.value()["task_obstacle"])->second);
    }

    std::vector<const Scene *> scenes;
    for (auto const &connectedScene : jsonSceneMapping.value()["connected_scenes"]) {
      scenes.push_back(&sceneMapping.find(connectedScene)->second);
    }

    scene.setupNextScenes(scenes);
  }

  for (auto const &jsonItemMapping : jsonData["items"].items()) {
    auto &item = itemMapping.find(jsonItemMapping.key())->second;

    if (jsonItemMapping.value()["task_obstacle"] != nullptr) {
      item.setTaskObstacle(&taskMapping.find(jsonItemMapping.value()["task_obstacle"])->second);
    }
  }

  int nextStateIndex = 0;
  for (const auto &scenePair : sceneMapping) {
    int noObstacleIndex = -1;
    for (const auto &constTask : sceneToTasksMapping.find(&scenePair.second)->second) {
      auto &task = taskMapping.find(constTask->getKey())->second;
      if (constTask->getTaskObstacle() == nullptr && constTask->getItemsNeeded().empty()) {
        if (noObstacleIndex == -1) {
          noObstacleIndex = nextStateIndex++;
        }
        task.setStateIndex(noObstacleIndex);
      } else {
        task.setStateIndex(nextStateIndex++);
      }
    }
  }

  for (const auto &scenePair : sceneMapping) {
    int noObstacleIndex = -1;
    auto sceneTaskObstacle = scenePair.second.getTaskObstacle();
    if (sceneTaskObstacle != nullptr && sceneTaskObstacle->getScene() == &scenePair.second) {
      noObstacleIndex = sceneTaskObstacle->getStateIndex();
    }

    for (const auto &constItem : sceneToItemsMapping.find(&scenePair.second)->second) {
      auto itemTaskObstacle = constItem->getTaskObstacle();
      auto &item = itemMapping.find(constItem->getKey())->second;
      if (itemTaskObstacle == nullptr) {
        if (noObstacleIndex == -1) {
          for (auto task : sceneToTasksMapping[&scenePair.second]) {
            if (task->getTaskObstacle() == nullptr && task->getItemsNeeded().empty()) {
              noObstacleIndex = task->getStateIndex();
              break;
            }
          }
          if (noObstacleIndex == -1) {
            noObstacleIndex = nextStateIndex++;
          }
        }
        item.setStateIndex(noObstacleIndex);
      } else if (itemTaskObstacle->getScene() == &scenePair.second) {
        item.setStateIndex(itemTaskObstacle->getStateIndex());
      } else {
        item.setStateIndex(nextStateIndex++);
      }
    }
  }
    
  auto sceneIterator = sceneMapping.find(jsonData["start_scene"]);
  if (sceneIterator == sceneMapping.end()) {
    throw std::runtime_error("Could not find start scene");
  }
  startScene = &sceneIterator->second;

  if (SCENE_COUNT != sceneMapping.size()) {
    std::stringstream stringStream;
    stringStream << "Predefined scene count (" << SCENE_COUNT << ") did not match the actual (" << sceneMapping.size() << ")";
    throw std::runtime_error(stringStream.str());
  } else {
    for (const auto &sceneInfo : sceneMapping) {
      scenes[sceneInfo.second.getUniqueIndex()] = &sceneInfo.second;
    }
  }

  if (ITEM_COUNT != itemMapping.size()) {
    std::stringstream stringStream;
    stringStream << "Predefined item count (" << ITEM_COUNT << ") did not match the actual (" << itemMapping.size() << ")";
    throw std::runtime_error(stringStream.str());
  }

  if (TASK_COUNT != taskMapping.size()) {
    std::stringstream stringStream;
    stringStream << "Predefined task count (" << TASK_COUNT << ") did not match the actual (" << taskMapping.size() << ")";
    throw std::runtime_error(stringStream.str());
  }

  if (STATE_TASK_ITEM_SIZE != nextStateIndex) {
    std::stringstream stringStream;
    stringStream << "Predefined state count (" << STATE_TASK_ITEM_SIZE << ") did not match the actual (" << nextStateIndex << ")";
    throw std::runtime_error(stringStream.str());
  }

  if (nextStateIndex + STATE_TASK_ITEM_START > 64) {
    std::stringstream stringStream;
    stringStream << "Too many different states to be able to optimize state key. "
                 << "Optimize the state-grouping or roll-back that use list of bools as state key. "
                 << "Found " << nextStateIndex << " unique states";
    throw std::runtime_error(stringStream.str());
  }

  for (const auto &scene : scenes) {
    bool foundPostScene = false;
    for (const auto &task : getTasksForScene(scene)) {
      if (task->getPostScene() != nullptr) {
        if (foundPostScene) {
          std::stringstream stringStream;
          stringStream << "Logic cannot handle more than one task with post-scene in a scene. "
                       << "This was the case for the scene \"" << scene->getKey() << "\"! "
                       << "Update PathController if it is needed.";
          throw std::runtime_error(stringStream.str());
        }

        foundPostScene = true;
      }
    }
  }
}

const std::array<const Scene *, SCENE_COUNT> &RawData::getScenes() const {
  return scenes;
}

std::vector<const Item *> RawData::getItems() const {
  std::vector<const Item *> items;
  for (const auto &item : itemMapping) {
    items.push_back(&item.second);
  }
  return items;
}

std::vector<const Task *> RawData::getTasks() const {
  std::vector<const Task *> tasks;
  for (const auto &task : taskMapping) {
    tasks.push_back(&task.second);
  }
  return tasks;
}

const Item *RawData::getItemByKey(const std::string &key) const {
  auto itemIterator = itemMapping.find(key);

  if (itemIterator == itemMapping.end()) {
    throw std::runtime_error("Could not find item with specific key");
  }

  return &itemIterator->second;
}

const Task *RawData::getTaskByKey(const std::string &key) const {
  auto taskIterator = taskMapping.find(key);

  if (taskIterator == taskMapping.end()) {
    throw std::runtime_error("Could not find task with specific key");
  }

  return &taskIterator->second;
}

const std::vector<const Item *> &RawData::getItemsForScene(const Scene *scene) const {
  return sceneToItemsMapping.find(scene)->second;
}

const std::vector<const Task *> &RawData::getTasksForScene(const Scene *scene) const {
  return sceneToTasksMapping.find(scene)->second;
}

const Scene *RawData::getScene(int index) const {
  return scenes[index];
}

const Scene *RawData::getSceneByKey(const std::string &key) const {
  auto sceneIterator = sceneMapping.find(key);

  if (sceneIterator == sceneMapping.end()) {
    throw std::runtime_error("Could not find scene with specific key");
  }

  return &sceneIterator->second;
}

const Scene *RawData::getStartScene() const {
  return startScene;
}

std::vector<std::pair<unsigned char, State>> RawData::getStatesForScene(const Scene *scene) const {
  std::array<bool, STATE_TASK_ITEM_SIZE> stateFound{};
  std::array<unsigned long long int, STATE_TASK_ITEM_SIZE> stateConditions{};
  for (auto item : getItemsForScene(scene)) {
    stateFound[item->getStateIndex()] = true;

    if (item->getTaskObstacle() != nullptr && item->getTaskObstacle()->getStateIndex() != item->getStateIndex()) {
      stateConditions[item->getStateIndex()] |= (1ULL << item->getTaskObstacle()->getStateIndex());
    }
  }
  for (auto task : getTasksForScene(scene)) {
    stateFound[task->getStateIndex()] = true;

    for (auto item : task->getItemsNeeded()) {
      stateConditions[task->getStateIndex()] |= (1ULL << item->getStateIndex());
    }

    if (task->getTaskObstacle() != nullptr) {
      stateConditions[task->getStateIndex()] |= (1ULL << task->getTaskObstacle()->getStateIndex());
    }
  }

  std::vector<std::pair<unsigned char, State>> states;
  for (int stateIndex = 0; stateIndex < stateFound.size(); ++stateIndex) {
    if (stateFound[stateIndex]) {
      State condition;
      condition.setBits<0, STATE_TASK_ITEM_SIZE>(stateConditions[stateIndex]);

      states.emplace_back(stateIndex, condition);
    }
  }

  return states;
}

#include "skipper-skeeto-path-finder/graph_data.h"

#include "skipper-skeeto-path-finder/edge.h"
#include "skipper-skeeto-path-finder/raw_data.h"

#include <list>
#include <sstream>

GraphData::GraphData(const nlohmann::json &jsonData, const RawData &rawData)
    : startLength(jsonData["start_length"]) {
  auto rawVertices = jsonData["vertices"];
  if (rawVertices.size() != VERTICES_COUNT) {
    std::stringstream stringStream;
    stringStream << "Predefined vertices count (" << VERTICES_COUNT << ") does not match actual (" << rawVertices.size() << ")";
    throw std::runtime_error(stringStream.str());
  }

  auto rawEdges = jsonData["edges"];
  if (rawEdges.size() != EDGES_COUNT) {
    std::stringstream stringStream;
    stringStream << "Predefined edges count (" << EDGES_COUNT << ") does not match actual (" << rawEdges.size() << ")";
    throw std::runtime_error(stringStream.str());
  }

  int vertexIndex = 0;
  for (auto rawVertexIterator = rawVertices.begin(); rawVertexIterator != rawVertices.end(); ++rawVertexIterator, ++vertexIndex) {
    auto &vertexInfo = vertices[vertexIndex];

    vertexInfo.key = rawVertexIterator.key();
    vertexInfo.furthestScene = rawData.getSceneByKey(rawVertexIterator.value()["furthest_scene"]);
    vertexInfo.isOneTime = rawVertexIterator.value()["one_time"];

    for (auto itemKey : rawVertexIterator.value()["items"]) {
      vertexInfo.items.push_back(rawData.getItemByKey(itemKey));
    }

    for (auto taskKey : rawVertexIterator.value()["tasks"]) {
      vertexInfo.tasks.push_back(rawData.getTaskByKey(taskKey));
    }
  }

  startIndex = getIndexForVertex(jsonData["start_vertex"]);

  int edgeIndex = 0;
  for (auto const &edgeData : rawEdges) {
    auto edge = &edges[edgeIndex];

    edge->endVertexIndex = getIndexForVertex(edgeData["to"]);
    edge->length = edgeData["length"];
    for (auto const &vertexKey : edgeData["conditions"]) {
      edge->condition |= (1ULL << getIndexForVertex(vertexKey));
    }

    auto fromVertexIndex = getIndexForVertex(edgeData["from"]);

    addEdgeToVertex(edge, fromVertexIndex);

    ++edgeIndex;
  }

  if (edgeIndex != EDGES_COUNT) {
    std::stringstream stringStream;
    stringStream << "Predefined edges count (" << EDGES_COUNT << ") did not match the actual (" << edgeIndex << ")";
    throw std::runtime_error(stringStream.str());
  }

  setupMinimumEntryDistances();
}

GraphData::GraphData(const RawData &rawData) {
  auto startScene = rawData.getStartScene();
  const Item *startItem = nullptr;
  for (auto item : rawData.getItemsForScene(startScene)) {
    if (item->getTaskObstacle() == nullptr) {
      startItem = item;
      break;
    }
  }

  if (startItem == nullptr) {
    throw std::runtime_error("Don't know how to handle start scene without item without obstacle");
  }
  startIndex = startItem->getStateIndex();
  startLength = 0;

  int edgeIndex = 0;
  for (int currentStateIndex = 0; currentStateIndex < STATE_TASK_ITEM_SIZE; ++currentStateIndex) {
    const Scene *rootScene = nullptr;
    int startEdgeLengh = 0;
    for (auto scene : rawData.getScenes()) {
      for (auto sceneState : rawData.getStatesForScene(scene)) {
        if (sceneState.first == currentStateIndex) {
          for (auto task : rawData.getTasksForScene(scene)) {
            if (task->getStateIndex() == currentStateIndex && task->getPostScene() != nullptr) {
              rootScene = task->getPostScene();
              startEdgeLengh = 10; // TODO: Find better solution
              break;
            }
          }

          if (rootScene == nullptr) {
            rootScene = scene;
          }
          break;
        }
      }
      if (rootScene != nullptr) {
        break;
      }
    }

    std::array<std::vector<std::pair<int, unsigned long long int>>, STATE_TASK_ITEM_SIZE> foundEdges{};

    std::list<std::tuple<const Scene *, int, unsigned long long int, int>> unresolvedScenes;
    unresolvedScenes.emplace_back(rootScene, startEdgeLengh, 0, -1);

    while (!unresolvedScenes.empty()) {
      auto currentSceneTuple = unresolvedScenes.front();
      auto currentScene = std::get<0>(currentSceneTuple);
      auto edgeLengh = std::get<1>(currentSceneTuple);
      auto extraConditions = std::get<2>(currentSceneTuple);
      auto previousSceneIndex = std::get<3>(currentSceneTuple);
      unresolvedScenes.pop_front();

      bool keepLooking = true;
      if (currentScene->getTaskObstacle() != nullptr && currentScene != rootScene) {
        if (currentScene->getTaskObstacle()->getScene() == currentScene) {
          // We have to solve this task to enter the scene - and after it's solved, we can always travel via it
          keepLooking = false;

          auto conditions = extraConditions;
          for (auto item : currentScene->getTaskObstacle()->getItemsNeeded()) {
            conditions |= (1ULL << item->getStateIndex());
          }

          if (currentScene->getTaskObstacle()->getTaskObstacle() != nullptr) {
            conditions |= (1ULL << currentScene->getTaskObstacle()->getTaskObstacle()->getStateIndex());
          }

          maybeAddEdge(foundEdges[currentScene->getTaskObstacle()->getStateIndex()], edgeLengh, conditions);
        } else {
          int foundStateWithoutCondition = false;
          for (auto state : rawData.getStatesForScene(currentScene)) {
            auto conditionForState = state.second.getBits<0, STATE_TASK_ITEM_SIZE>();
            auto conditionForEdge = extraConditions | conditionForState | (1ULL << currentScene->getTaskObstacle()->getStateIndex());

            maybeAddEdge(foundEdges[state.first], edgeLengh, conditionForEdge);

            if (conditionForState == 0) {
              foundStateWithoutCondition = true;
            }
          }

          // We need a state without condition to finish this "path" since we want to be able to
          // go to a state continue from it without fulfilling any extra conditions (other than
          // the task obstacle of the scene(s))
          if (foundStateWithoutCondition) {
            keepLooking = false;
          } else {
            extraConditions |= (1ULL << currentScene->getTaskObstacle()->getStateIndex());
          }
        }
      } else {
        for (auto sceneState : rawData.getStatesForScene(currentScene)) {
          if (sceneState.first == currentStateIndex) {
            continue;
          }

          auto condition = sceneState.second.getBits<0, STATE_TASK_ITEM_SIZE>();

          maybeAddEdge(foundEdges[sceneState.first], edgeLengh, condition | extraConditions);

          if (condition == 0) {
            // This state can always be reached so we can just continue from it
            keepLooking = false;
          }
        }
      }

      if (keepLooking) {
        for (auto nextScene : currentScene->getNextScenes()) {
          if (nextScene->getUniqueIndex() == previousSceneIndex) {
            continue;
          }

          unresolvedScenes.emplace_back(nextScene, edgeLengh + 1, extraConditions, currentScene->getUniqueIndex());
        }
      }

      // Remember post scene
    }

    for (int endVertexIndex = 0; endVertexIndex < STATE_TASK_ITEM_SIZE; ++endVertexIndex) {
      for (auto edgeData : foundEdges[endVertexIndex]) {
        if (edgeIndex >= EDGES_COUNT) {
          std::stringstream stringStream;
          stringStream << "Predefined edges count (" << EDGES_COUNT << ") is too small, it should be increased";
          throw std::runtime_error(stringStream.str());
        }

        auto edge = &edges[edgeIndex];

        edge->endVertexIndex = endVertexIndex;
        edge->length = edgeData.first;
        edge->condition = edgeData.second;

        auto fromVertexIndex = currentStateIndex;

        addEdgeToVertex(edge, fromVertexIndex);

        ++edgeIndex;
      }
    }

    for (auto item : rawData.getItems()) {
      auto &vertex = vertices[item->getStateIndex()];

      vertex.items.push_back(item);

      auto &furthestSceneForVertex = vertex.furthestScene;
      if (furthestSceneForVertex != nullptr && furthestSceneForVertex != item->getScene()) {
        throw std::runtime_error("Did find more than one scene for state");
      }

      furthestSceneForVertex = item->getScene();
    }

    for (auto task : rawData.getTasks()) {
      auto &vertex = vertices[task->getStateIndex()];

      vertex.tasks.push_back(task);

      if (task->getPostScene() != nullptr) {
        // As a rule of thumb it shouldn't be visited more than once as it could be used as a "shortcut"
        // if post scene isn't a regular connected scene
        vertex.isOneTime = true;
      }

      auto &furthestSceneForVertex = vertex.furthestScene;
      if (furthestSceneForVertex != nullptr && furthestSceneForVertex != task->getScene()) {
        throw std::runtime_error("Did find more than one scene for state");
      }

      furthestSceneForVertex = task->getScene();
    }
  }

  if (edgeIndex != EDGES_COUNT) {
    std::stringstream stringStream;
    stringStream << "Predefined edges count (" << EDGES_COUNT << ") did not match the actual (" << edgeIndex << ")";
    throw std::runtime_error(stringStream.str());
  }

  setupMinimumEntryDistances();
}

unsigned char GraphData::getStartIndex() const {
  return startIndex;
}

unsigned char GraphData::getStartLength() const {
  return startLength;
}

const std::vector<const Edge *> &GraphData::getEdgesForVertex(char vertexIndex) const {
  return verticesToEdgesMap[vertexIndex];
}

const Vertex *GraphData::getVertex(char vertexIndex) const {
  return &vertices[vertexIndex];
}

unsigned char GraphData::getMinimumEntryDistance(char vertexIndex) const {
  return vertexMinimumEntryDistances[vertexIndex];
}

nlohmann::json GraphData::toJson(const RawData &rawData) const {
  nlohmann::json json;

  for (const auto &edge : edges) {
    int fromVertexIndex = -1;
    for (int vertexIndex = 0; vertexIndex < VERTICES_COUNT; ++vertexIndex) {
      for (auto edgePtr : verticesToEdgesMap[vertexIndex]) {
        if (edgePtr == &edge) {
          fromVertexIndex = vertexIndex;
          break;
        }
      }

      if (fromVertexIndex != -1) {
        break;
      }
    }

    if (fromVertexIndex < 0) {
      throw std::runtime_error("Could not find from vertex");
    }

    auto conditions = nlohmann::json::array();
    for (int conditionNumber = 0; conditionNumber < VERTICES_COUNT; ++conditionNumber) {
      auto mask = (1ULL << conditionNumber);
      if ((edge.condition & mask) == mask) {
        conditions.push_back(conditionNumber);
      }
    }

    json.push_back(
        {{"from", fromVertexIndex},
         {"to", edge.endVertexIndex},
         {"length", edge.length},
         {"conditions", conditions}});
  }

  return json;
}

void GraphData::maybeAddEdge(std::vector<std::pair<int, unsigned long long int>> &edges, int length, unsigned long long int condition) {
  bool shouldAdd = true;
  auto iterator = edges.begin();
  while (iterator != edges.end()) {
    if (condition == 0) {
      if (length <= iterator->first) {
        iterator = edges.erase(iterator);
      } else if (iterator->second == 0) {
        // The other edge with no conditions as well that's shorter
        shouldAdd = false;
        break;
      } else {
        // The other edge is shorter, but has conditions, so it's not enough to discard this
        ++iterator;
      }
    } else {
      if (iterator->second == 0) {
        if (iterator->first <= length) {
          shouldAdd = false;
          break;
        } else {
          // Even though there is an edge without condition it might be smarter to go this
          // shorter route with condition if it's fulfilled
          ++iterator;
        }
      } else if (iterator->second == condition) {
        if (iterator->first > length) {
          iterator = edges.erase(iterator);
        } else {
          shouldAdd = false;
          break;
        }
      } else {
        // There might be faster ways with other conditions but if this is the only fulfilled
        // it might be smarter to pick
        ++iterator;
      }
    }
  }

  if (shouldAdd) {
    edges.emplace_back(length, condition);
  }
}

int GraphData::getIndexForVertex(const std::string &key) const {
  for (int vertexIndex = 0; vertexIndex < VERTICES_COUNT; ++vertexIndex) {
    if (vertices[vertexIndex].key == key) {
      return vertexIndex;
    }
  }

  std::stringstream stream;
  stream << "Vertex with key " << key << " could not be found";
  throw std::runtime_error(stream.str());
}

void GraphData::addEdgeToVertex(const Edge *edge, int fromVertexIndex) {
  // Insert edge into vertices map - note that we sort the shortest first
  auto &edgesFromVertex = verticesToEdgesMap[fromVertexIndex];
  const auto &longerLengthIterator = std::upper_bound(
      edgesFromVertex.begin(),
      edgesFromVertex.end(),
      edge->length,
      [](const auto length, const auto &otherEdge) {
        return length < otherEdge->length;
      });
  edgesFromVertex.insert(longerLengthIterator, edge);
}

void GraphData::setupMinimumEntryDistances() {
  std::array<bool, VERTICES_COUNT> isDeadEndMap{};

  for (int ownIndex = 0; ownIndex < VERTICES_COUNT; ++ownIndex) {
    auto exitEdges = getEdgesForVertex(ownIndex);
    bool isDeadEnd = exitEdges.size() < 2;
    char distance = -1;

    for (int otherIndex = 0; otherIndex < VERTICES_COUNT; ++otherIndex) {
      if (!isDeadEnd || exitEdges.empty()) {
        break;
      }

      if (otherIndex == ownIndex) {
        continue;
      }

      for (auto edge : getEdgesForVertex(otherIndex)) {
        if (edge->endVertexIndex != ownIndex) {
          continue;
        }

        if (exitEdges.front()->endVertexIndex != otherIndex) {
          isDeadEnd = false;
          break;
        }
      }
    }

    if (isDeadEnd) {
      isDeadEndMap[ownIndex] = true;
    }
  }

  for (int ownIndex = 0; ownIndex < VERTICES_COUNT; ++ownIndex) {
    char distance = -1;

    for (int otherIndex = 0; otherIndex < VERTICES_COUNT; ++otherIndex) {
      if (otherIndex == ownIndex) {
        continue;
      }

      if (isDeadEndMap[otherIndex]) {
        // If it's dead end it could end up being the last vertex so we might never go from otherIndex to ownIndex.
        // Furthermore this also means we will always go from some other vertex in order to go to ownIndex (or else it would already have been visited if we go from otherIndex)
        continue;
      }

      for (auto edge : getEdgesForVertex(otherIndex)) {
        if (edge->endVertexIndex != ownIndex) {
          continue;
        }

        if (distance < 0 || distance > edge->length) {
          distance = edge->length;
        }
      }
    }

    if (distance < 0) {
      std::stringstream stream;
      stream << "Could not find an entry for vertex " << ownIndex << std::endl;

      throw std::runtime_error(stream.str().c_str());
    }

    vertexMinimumEntryDistances[ownIndex] = distance;
  }
}

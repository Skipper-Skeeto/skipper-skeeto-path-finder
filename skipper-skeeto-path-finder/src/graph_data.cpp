#include "skipper-skeeto-path-finder/graph_data.h"

#include "skipper-skeeto-path-finder/edge.h"
#include "skipper-skeeto-path-finder/raw_data.h"

#include <list>
#include <sstream>

GraphData::GraphData(const nlohmann::json &jsonData, const RawData &rawData) : startIndex(0) {
  auto rawVertices = jsonData["vertices"];
  if (rawVertices.size() > VERTICES_COUNT) {
    std::stringstream stringStream;
    stringStream << "Predefined vertices count (" << VERTICES_COUNT << ") does not match actual (" << rawVertices.size() << ")";
    throw std::runtime_error(stringStream.str());
  }

  auto rawEdges = jsonData["edges"];
  if (rawEdges.size() > EDGES_COUNT) {
    std::stringstream stringStream;
    stringStream << "Predefined edges count (" << EDGES_COUNT << ") does not match actual (" << rawEdges.size() << ")";
    throw std::runtime_error(stringStream.str());
  }

  for (auto rawVertexIterator = rawVertices.begin(); rawVertexIterator != rawVertices.end(); ++rawVertexIterator) {
    // Minus 1 because the vertices are 1-indexed
    auto vertexIndex = std::stoi(rawVertexIterator.key()) - 1;

    auto furthestRoom = rawData.getRoomByKey(rawVertexIterator.value()["furthest_room"]);
    auto &furthestRoomForVertex = verticesToFurthestRoomMap[vertexIndex];
    if (furthestRoomForVertex != nullptr) {
      throw std::runtime_error("Did find more than one room for vertex");
    }
    furthestRoomForVertex = furthestRoom;
  }

  int edgeIndex = 0;
  for (auto const &edgeData : rawEdges) {
    auto edge = &edges[edgeIndex];

    // Minus 1 because the vertices are 1-indexed
    edge->endVertexIndex = edgeData["to"].get<int>() - 1;
    edge->length = edgeData["length"];
    for (auto const &conditionNumber : edgeData["conditions"]) {
      // Minus 1 because the vertices are 1-indexed
      edge->condition |= (1ULL << (conditionNumber.get<int>() - 1));
    }

    // Minus 1 because the vertices are 1-indexed
    auto fromVertexIndex = edgeData["from"].get<int>() - 1;

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
  auto startRoom = rawData.getStartRoom();
  const Item *startItem = nullptr;
  for (auto item : rawData.getItemsForRoom(startRoom)) {
    if (item->getTaskObstacle() == nullptr) {
      startItem = item;
      break;
    }
  }

  if (startItem == nullptr) {
    throw std::runtime_error("Don't know how to handle start room without item without obstacle");
  }
  startIndex = startItem->getStateIndex();

  int edgeIndex = 0;
  for (int currentStateIndex = 0; currentStateIndex < STATE_TASK_ITEM_SIZE; ++currentStateIndex) {
    const Room *rootRoom = nullptr;
    int startEdgeLengh = 0;
    for (auto room : rawData.getRooms()) {
      for (auto roomState : rawData.getStatesForRoom(room)) {
        if (roomState.first == currentStateIndex) {
          for (auto task : rawData.getTasksForRoom(room)) {
            if (task->getStateIndex() == currentStateIndex && task->getPostRoom() != nullptr) {
              rootRoom = task->getPostRoom();
              startEdgeLengh = 10; // TODO: Find better solution
              break;
            }
          }

          if (rootRoom == nullptr) {
            rootRoom = room;
          }
          break;
        }
      }
      if (rootRoom != nullptr) {
        break;
      }
    }

    std::array<std::vector<std::pair<int, unsigned long long int>>, STATE_TASK_ITEM_SIZE> foundEdges{};

    std::list<std::tuple<const Room *, int, unsigned long long int, int>> unresolvedRooms;
    unresolvedRooms.emplace_back(rootRoom, startEdgeLengh, 0, -1);

    while (!unresolvedRooms.empty()) {
      auto currentRoomTuple = unresolvedRooms.front();
      auto currentRoom = std::get<0>(currentRoomTuple);
      auto edgeLengh = std::get<1>(currentRoomTuple);
      auto extraConditions = std::get<2>(currentRoomTuple);
      auto previousRoomIndex = std::get<3>(currentRoomTuple);
      unresolvedRooms.pop_front();

      bool keepLooking = true;
      if (currentRoom->getTaskObstacle() != nullptr && currentRoom != rootRoom) {
        if (currentRoom->getTaskObstacle()->getRoom() == currentRoom) {
          // We have to solve this task to enter the room - and after it's solved, we can always travel via it
          keepLooking = false;

          auto conditions = extraConditions;
          for (auto item : currentRoom->getTaskObstacle()->getItemsNeeded()) {
            conditions |= (1ULL << item->getStateIndex());
          }

          if (currentRoom->getTaskObstacle()->getTaskObstacle() != nullptr) {
            conditions |= (1ULL << currentRoom->getTaskObstacle()->getTaskObstacle()->getStateIndex());
          }

          maybeAddEdge(foundEdges[currentRoom->getTaskObstacle()->getStateIndex()], edgeLengh, conditions);
        } else {
          int foundStateWithoutCondition = false;
          for (auto state : rawData.getStatesForRoom(currentRoom)) {
            auto conditionForState = state.second.getBits<0, STATE_TASK_ITEM_SIZE>();
            auto conditionForEdge = extraConditions | conditionForState | (1ULL << currentRoom->getTaskObstacle()->getStateIndex());

            maybeAddEdge(foundEdges[state.first], edgeLengh, conditionForEdge);

            if (conditionForState == 0) {
              foundStateWithoutCondition = true;
            }
          }

          // We need a state without condition to finish this "path" since we want to be able to
          // go to a state continue from it without fulfilling any extra conditions (other than
          // the task obstacle of the room(s))
          if (foundStateWithoutCondition) {
            keepLooking = false;
          } else {
            extraConditions |= (1ULL << currentRoom->getTaskObstacle()->getStateIndex());
          }
        }
      } else {
        for (auto roomState : rawData.getStatesForRoom(currentRoom)) {
          if (roomState.first == currentStateIndex) {
            continue;
          }

          auto condition = roomState.second.getBits<0, STATE_TASK_ITEM_SIZE>();

          maybeAddEdge(foundEdges[roomState.first], edgeLengh, condition | extraConditions);

          if (condition == 0) {
            // This state can always be reached so we can just continue from it
            keepLooking = false;
          }
        }
      }

      if (keepLooking) {
        for (auto nextRoom : currentRoom->getNextRooms()) {
          if (nextRoom->getUniqueIndex() == previousRoomIndex) {
            continue;
          }

          unresolvedRooms.emplace_back(nextRoom, edgeLengh + 1, extraConditions, currentRoom->getUniqueIndex());
        }
      }

      // Remember post room
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
      auto &furthestRoomForVertex = verticesToFurthestRoomMap[item->getStateIndex()];
      if (furthestRoomForVertex != nullptr && furthestRoomForVertex != item->getRoom()) {
        throw std::runtime_error("Did find more than one room for state");
      }

      furthestRoomForVertex = item->getRoom();
    }

    for (auto task : rawData.getTasks()) {
      auto &furthestRoomForVertex = verticesToFurthestRoomMap[task->getStateIndex()];
      if (furthestRoomForVertex != nullptr && furthestRoomForVertex != task->getRoom()) {
        throw std::runtime_error("Did find more than one room for state");
      }

      furthestRoomForVertex = task->getRoom();
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

const std::vector<const Edge *> &GraphData::getEdgesForVertex(char vertexIndex) const {
  return verticesToEdgesMap[vertexIndex];
}

const Room *GraphData::getFurthestRoomForVertex(char vertexIndex) const {
  return verticesToFurthestRoomMap[vertexIndex];
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

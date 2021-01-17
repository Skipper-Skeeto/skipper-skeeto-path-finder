#include "skipper-skeeto-path-finder/graph_data.h"

#include "skipper-skeeto-path-finder/edge.h"

#include <sstream>

GraphData GraphData::fromJson(const nlohmann::json &jsonData) {
  GraphData graphData;
  graphData.startIndex = 0;

  int edgeIndex = 0;
  for (auto const &edgeData : jsonData) {
    Edge edge;

    // Minus 1 because the vertices are 1-indexed
    edge.endVertexIndex = edgeData["to"].get<int>() - 1;
    edge.length = edgeData["length"];
    for (auto const &conditionNumber : edgeData["conditions"]) {
      // Minus 1 because the vertices are 1-indexed
      edge.condition |= (1ULL << (conditionNumber.get<int>() - 1));
    }

    // Minus 1 because the vertices are 1-indexed
    auto fromVertexIndex = edgeData["from"].get<int>() - 1;

    graphData.addEdge(edgeIndex, edge, fromVertexIndex);

    ++edgeIndex;
  }

  graphData.setupMinimumEntryDistances();

  return graphData;
}

unsigned char GraphData::getStartIndex() const {
  return startIndex;
}

const std::vector<const Edge *> &GraphData::getEdgesForVertex(char vertexIndex) const {
  return verticesMap[vertexIndex];
}

unsigned char GraphData::getMinimumEntryDistance(char vertexIndex) const {
  return vertexMinimumEntryDistances[vertexIndex];
}

void GraphData::addEdge(int edgeIndex, const Edge &edge, int fromVertexIndex) {
  edges[edgeIndex] = edge;

  // Insert edge into vertices map - note that we sort the shortest first
  auto &edgesFromVertex = verticesMap[fromVertexIndex];
  const auto &longerLengthIterator = std::upper_bound(
      edgesFromVertex.begin(),
      edgesFromVertex.end(),
      edge.length,
      [](const auto length, const auto &otherEdge) {
        return length < otherEdge->length;
      });
  edgesFromVertex.insert(longerLengthIterator, &edges[edgeIndex]);
}

void GraphData::setupMinimumEntryDistances() {
  std::array<bool, VERTICES_COUNT> isDeadEndMap{};

  for (int ownIndex = 0; ownIndex < VERTICES_COUNT; ++ownIndex) {
    auto exitEdges = getEdgesForVertex(ownIndex);
    bool isDeadEnd = exitEdges.size() < 2;
    char distance = -1;

    for (int otherIndex = 0; otherIndex < VERTICES_COUNT; ++otherIndex) {
      if (!isDeadEnd) {
        break;
      }

      if (otherIndex == ownIndex) {
        continue;
      }

      for (auto edge : getEdgesForVertex(otherIndex)) {
        if (edge->endVertexIndex != ownIndex) {
          continue;
        }

        if (exitEdges.front()->endVertexIndex == otherIndex) {
          distance = edge->length;
        } else {
          isDeadEnd = false;
          break;
        }
      }
    }

    if (isDeadEnd) {
      isDeadEndMap[ownIndex] = true;
      vertexMinimumEntryDistances[ownIndex] = distance;
    }
  }

  for (int ownIndex = 0; ownIndex < VERTICES_COUNT; ++ownIndex) {
    if (isDeadEndMap[ownIndex]) {
      continue; // Distance already set
    }

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

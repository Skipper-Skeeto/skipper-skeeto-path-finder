#include "skipper-skeeto-path-finder/graph_data.h"

#include "skipper-skeeto-path-finder/edge.h"

#include <sstream>

GraphData::GraphData(const nlohmann::json &jsonData) {
  int edgeIndex = 0;
  for (auto const &edgeData : jsonData) {
    auto &edge = edges[edgeIndex];

    edge.endVertexIndex = edgeData["to"].get<int>() - 1;
    edge.length = edgeData["length"];
    for (auto const &conditionNumber : edgeData["conditions"]) {
      // Minus 1 because the vertices are 1-indexed
      edge.condition |= (1ULL << (conditionNumber.get<int>() - 1));
    }

    // Insert edge into vertices map - note that we sort the shortest first
    auto &edgesFromVertex = verticesMap[edgeData["from"].get<int>() - 1];
    const auto &longerLengthIterator = std::upper_bound(
        edgesFromVertex.begin(),
        edgesFromVertex.end(),
        edge.length,
        [](const auto length, const auto &otherEdge) {
          return length < otherEdge->length;
        });
    edgesFromVertex.insert(longerLengthIterator, &edge);

    ++edgeIndex;
  }
}

const std::vector<const Edge *> &GraphData::getEdgesForVertex(char vertexIndex) const {
  return verticesMap[vertexIndex];
}

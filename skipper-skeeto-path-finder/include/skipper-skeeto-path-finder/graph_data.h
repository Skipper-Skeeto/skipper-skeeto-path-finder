#pragma once

#include "skipper-skeeto-path-finder/edge.h"
#include "skipper-skeeto-path-finder/info.h"

#include "json/json.hpp"

#include <array>
#include <vector>

class GraphData {
public:
  GraphData(const nlohmann::json &jsonData);

  unsigned char getStartIndex() const;

  const std::vector<const Edge *> &getEdgesForVertex(char vertexIndex) const;

  unsigned char getMinimumEntryDistance(char vertexIndex) const;

private:

  void addEdgeToVertex(const Edge *edge, int fromVertexIndex);

  void setupMinimumEntryDistances();

  unsigned char startIndex;
  std::array<std::vector<const Edge *>, VERTICES_COUNT> verticesMap{};
  std::array<Edge, EDGES_COUNT> edges{};
  std::array<unsigned char, VERTICES_COUNT> vertexMinimumEntryDistances{};
};
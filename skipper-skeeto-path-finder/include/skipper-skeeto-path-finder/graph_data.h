#pragma once

#include "skipper-skeeto-path-finder/edge.h"
#include "skipper-skeeto-path-finder/info.h"

#include "json/json.hpp"

#include <array>
#include <vector>

class RawData;

class GraphData {
public:
  GraphData(const nlohmann::json &jsonData);

  GraphData(const RawData &rawData);

  unsigned char getStartIndex() const;

  const std::vector<const Edge *> &getEdgesForVertex(char vertexIndex) const;

  unsigned char getMinimumEntryDistance(char vertexIndex) const;

private:
  static void maybeAddEdge(std::vector<std::pair<int, unsigned long long int>> &edges, int length, unsigned long long int condition);

  void addEdgeToVertex(const Edge *edge, int fromVertexIndex);

  void setupMinimumEntryDistances();

  unsigned char startIndex;
  std::array<std::vector<const Edge *>, VERTICES_COUNT> verticesMap{};
  std::array<Edge, EDGES_COUNT> edges{};
  std::array<unsigned char, VERTICES_COUNT> vertexMinimumEntryDistances{};
};
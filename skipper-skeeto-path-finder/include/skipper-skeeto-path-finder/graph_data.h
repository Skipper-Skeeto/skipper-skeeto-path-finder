#pragma once

#include "skipper-skeeto-path-finder/edge.h"
#include "skipper-skeeto-path-finder/info.h"

#include "json/json.hpp"

#include <array>
#include <vector>

class GraphData {
public:
  static GraphData fromJson(const nlohmann::json &jsonData);

  const std::vector<const Edge *> &getEdgesForVertex(char vertexIndex) const;

  unsigned char getMinimumEntryDistance(char vertexIndex) const;

private:
  GraphData() = default;

  void setupMinimumEntryDistances();

  std::array<std::vector<const Edge *>, VERTICES_COUNT> verticesMap{};
  std::array<Edge, EDGES_COUNT> edges{};
  std::array<unsigned char, VERTICES_COUNT> vertexMinimumEntryDistances{};
};
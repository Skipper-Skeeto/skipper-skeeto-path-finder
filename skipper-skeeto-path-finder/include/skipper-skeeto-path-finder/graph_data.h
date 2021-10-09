#pragma once

#include "skipper-skeeto-path-finder/edge.h"
#include "skipper-skeeto-path-finder/info.h"
#include "skipper-skeeto-path-finder/vertex.h"

#include "json/json.hpp"

#include <array>
#include <vector>

class RawData;
class Item;
class Task;
class Room;

class GraphData {
public:
  GraphData(const nlohmann::json &jsonData, const RawData &rawData);

  GraphData(const RawData &rawData);

  unsigned char getStartIndex() const;

  unsigned char getStartLength() const;

  const std::vector<const Edge *> &getEdgesForVertex(char vertexIndex) const;

  const Vertex *getVertex(char vertexIndex) const;

  unsigned char getMinimumEntryDistance(char vertexIndex) const;

  nlohmann::json toJson(const RawData &rawData) const;

private:
  static void maybeAddEdge(std::vector<std::pair<int, unsigned long long int>> &edges, int length, unsigned long long int condition);

  void addEdgeToVertex(const Edge *edge, int fromVertexIndex);

  void setupMinimumEntryDistances();

  unsigned char startIndex;
  unsigned char startLength;
  std::array<std::vector<const Edge *>, VERTICES_COUNT> verticesToEdgesMap{};
  std::array<Vertex, VERTICES_COUNT> vertices{};
  std::array<Edge, EDGES_COUNT> edges{};
  std::array<unsigned char, VERTICES_COUNT> vertexMinimumEntryDistances{};
};
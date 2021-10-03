#pragma once

#include "skipper-skeeto-path-finder/edge.h"
#include "skipper-skeeto-path-finder/info.h"

#include "json/json.hpp"

#include <array>
#include <vector>

class RawData;
class Room;

class GraphData {
public:
  GraphData(const nlohmann::json &jsonData, const RawData &rawData);

  GraphData(const RawData &rawData);

  unsigned char getStartIndex() const;

  unsigned char getStartLength() const;

  const std::vector<const Edge *> &getEdgesForVertex(char vertexIndex) const;

  const Room *getFurthestRoomForVertex(char vertexIndex) const;

  bool isOneTime(char vertexIndex) const;

  unsigned char getMinimumEntryDistance(char vertexIndex) const;

  nlohmann::json toJson(const RawData &rawData) const;

private:
  struct VertexInfo {
    const Room *furthestRoom = nullptr;
    bool oneTime = false;
  };

  static void maybeAddEdge(std::vector<std::pair<int, unsigned long long int>> &edges, int length, unsigned long long int condition);

  void addEdgeToVertex(const Edge *edge, int fromVertexIndex);

  void setupMinimumEntryDistances();

  unsigned char startIndex;
  unsigned char startLength;
  std::array<std::vector<const Edge *>, VERTICES_COUNT> verticesToEdgesMap{};
  std::array<VertexInfo, VERTICES_COUNT> vertices{};
  std::array<Edge, EDGES_COUNT> edges{};
  std::array<unsigned char, VERTICES_COUNT> vertexMinimumEntryDistances{};
};
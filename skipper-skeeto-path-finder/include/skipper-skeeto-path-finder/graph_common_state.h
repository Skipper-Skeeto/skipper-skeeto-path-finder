#pragma once

#include "skipper-skeeto-path-finder/graph_path.h"
#include "skipper-skeeto-path-finder/info.h"

#include <parallel_hashmap/phmap.h>

#include <array>
#include <string>
#include <vector>

class GraphCommonState {
public:
  bool makesSenseToInitialize(const GraphPath *path) const;

  bool makesSenseToKeep(const GraphPath *path);

  bool maybeAddNewGoodOne(const GraphPath *path);

  void dumpGoodOnes(const std::string &dirName);

private:
  static const char *DUMPED_GOOD_ONES_BASE_DIR;

  unsigned char getMaxDistance() const;

  int checkForDuplicateState(const State &state, unsigned char distance);

  mutable std::mutex finalStateMutex;
  mutable std::mutex distanceStateMutex;
  mutable std::mutex printMutex;

  unsigned char maxDistance = 255;
  phmap::parallel_flat_hash_map<State, unsigned char> distanceForState{};
  std::vector<std::array<char, VERTICES_COUNT>> goodOnes;
  int dumpedGoodOnes = 0;
};

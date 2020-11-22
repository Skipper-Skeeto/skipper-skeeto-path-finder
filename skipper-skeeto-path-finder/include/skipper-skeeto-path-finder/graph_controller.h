#pragma once

#include "skipper-skeeto-path-finder/graph_common_state.h"

class GraphData;

class GraphController {
public:
  GraphController(const GraphData *data);

  void start();

private:
  static const char *MEMORY_DUMP_DIR;

  bool moveOnDistributed(GraphPath *path);

  void initializePath(GraphPath *path);

  GraphCommonState commonState;
  const GraphData *data;

  std::string resultDirName;
};
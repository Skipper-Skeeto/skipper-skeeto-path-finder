#include "skipper-skeeto-path-finder/info.h"

#if USED_DATA_TYPE == DATA_TYPE_RAW
#include "skipper-skeeto-path-finder/data.h"
#include "skipper-skeeto-path-finder/path_controller.h"
#else if DATA_TYPE == DATA_TYPE_GRAPH
#include "skipper-skeeto-path-finder/graph_controller.h"
#include "skipper-skeeto-path-finder/graph_data.h"
#endif

#include "json/json.hpp"

#include <fstream>
#include <iostream>

int main() {
#if USED_DATA_TYPE == DATA_TYPE_RAW
  std::ifstream dataStream("../data/ss1.json");
#else if DATA_TYPE == DATA_TYPE_GRAPH
  std::ifstream dataStream("../data/ss1_graph.json");
#endif

  nlohmann::json jsonData;
  dataStream >> jsonData;

  try {
#if USED_DATA_TYPE == DATA_TYPE_RAW
    Data data(jsonData);

    PathController controller(&data);
    controller.start();

    controller.printResult();
#else if DATA_TYPE == DATA_TYPE_GRAPH
    GraphData data(jsonData);

    GraphController controller(&data);
    controller.start();
#endif

    std::cout << "Done" << std::endl;
  } catch (const std::exception &exception) {
    std::cout << "Caught exception: " << exception.what() << std::endl;
    return -1;
  }

  return 0;
}
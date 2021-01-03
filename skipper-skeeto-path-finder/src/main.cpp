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

#if USED_DATA_TYPE == DATA_TYPE_RAW
#define DATA_FILE_SUFFIX "raw"
#else if DATA_TYPE == DATA_TYPE_GRAPH
#define DATA_FILE_SUFFIX "graph"
#endif

int main() {
  std::ifstream dataStream(std::string("../data/ss") + std::to_string(GAME_ID) + "_" DATA_FILE_SUFFIX ".json");

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
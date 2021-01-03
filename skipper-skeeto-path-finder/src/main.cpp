#include "skipper-skeeto-path-finder/data.h"
#include "skipper-skeeto-path-finder/path.h"
#include "skipper-skeeto-path-finder/path_controller.h"

#include "json/json.hpp"

#include <fstream>
#include <iostream>

int main() {
  std::ifstream dataStream(std::string("../data/ss") + std::to_string(GAME_ID) + "_raw.json");
  nlohmann::json jsonData;
  dataStream >> jsonData;

  try {
    Data data(jsonData);

    PathController controller(&data);
    controller.start();

    controller.printResult();
  } catch (const std::exception &exception) {
    std::cout << "Caught exception: " << exception.what() << std::endl;
    return -1;
  }

  return 0;
}
#include "skipper-skeeto-path-finder/data.h"
#include "skipper-skeeto-path-finder/path.h"
#include "skipper-skeeto-path-finder/path_controller.h"

#include "json/json.hpp"

#include <fstream>

int main() {
  std::ifstream dataStream("../data/ss1.json");
  nlohmann::json jsonData;
  dataStream >> jsonData;

  Data data(jsonData);

  PathController controller(&data);
  controller.start();

  controller.printResult();
}
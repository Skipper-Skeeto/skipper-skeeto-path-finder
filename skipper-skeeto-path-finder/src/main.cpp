#include "skipper-skeeto-path-finder/info.h"

#include "skipper-skeeto-path-finder/file_helper.h"
#include "skipper-skeeto-path-finder/graph_controller.h"
#include "skipper-skeeto-path-finder/graph_data.h"
#include "skipper-skeeto-path-finder/path_controller.h"
#include "skipper-skeeto-path-finder/raw_data.h"

#include "json/json.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

std::string getDateTime() {
  // Do not use localtime(), see https://stackoverflow.com/a/38034148/2761541
  std::time_t currentTime = std::time(nullptr);
  std::tm localTime{};
#ifdef _WIN32
  localtime_s(&localTime, &currentTime);
#else
  localtime_r(&currentTime, &localTime);
#endif

  std::ostringstream stringStream;
  stringStream << std::put_time(&localTime, "%Y%m%d-%H%M%S");

  return stringStream.str();
}

std::pair<std::string, std::string> createResultDirs() {
  auto baseResultDir = "results";
  FileHelper::createDir(baseResultDir);

  auto executionResultDir = std::string(baseResultDir) + "/" + getDateTime();
  FileHelper::createDir(executionResultDir.c_str());

  auto graphResultDir = executionResultDir + "/" + "graph";
  FileHelper::createDir(graphResultDir.c_str());

  auto finalResultDir = executionResultDir + "/" + "final";
  FileHelper::createDir(finalResultDir.c_str());

  return std::make_pair(graphResultDir, finalResultDir);
}

nlohmann::json loadJson(const char *type) {
  std::ifstream dataStream(std::string("../data/ss") + FILE_IDENTIFIER + "_" + type + ".json");

  nlohmann::json jsonData;
  dataStream >> jsonData;

  return jsonData;
}

int main() {
  std::string graphResultDir, finalResultDir;
  std::tie(graphResultDir, finalResultDir) = createResultDirs();

  try {
    RawData rawData(loadJson("raw"));
#if USED_DATA_TYPE == DATA_TYPE_RAW
    GraphData graphData(rawData);
#elif USED_DATA_TYPE == DATA_TYPE_GRAPH
    GraphData graphData(loadJson("graph"), rawData);
#endif

    GraphController controller(&graphData, graphResultDir);
    controller.start();

    std::cout << "Done with graph" << std::endl;

    auto graphResult = controller.getResult();

    PathController pathController(&rawData, &graphData, graphResult, finalResultDir);

    pathController.start();

    std::cout << "Done" << std::endl;
  } catch (const std::exception &exception) {
    std::cout << "Caught exception: " << exception.what() << std::endl;
    return -1;
  }

  return 0;
}

#include "skipper-skeeto-path-finder/info.h"

#if USED_DATA_TYPE == DATA_TYPE_RAW
#include "skipper-skeeto-path-finder/path_controller.h"
#include "skipper-skeeto-path-finder/raw_data.h"
#endif
#include "skipper-skeeto-path-finder/file_helper.h"
#include "skipper-skeeto-path-finder/graph_controller.h"
#include "skipper-skeeto-path-finder/graph_data.h"

#include "json/json.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#if USED_DATA_TYPE == DATA_TYPE_RAW
#define DATA_FILE_SUFFIX "raw"
#elif USED_DATA_TYPE == DATA_TYPE_GRAPH
#define DATA_FILE_SUFFIX "graph"
#endif

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

int main() {
  std::ifstream dataStream(std::string("../data/ss") + std::to_string(GAME_ID) + "_" DATA_FILE_SUFFIX ".json");

  nlohmann::json jsonData;
  dataStream >> jsonData;

  std::string graphResultDir, finalResultDir;
  std::tie(graphResultDir, finalResultDir) = createResultDirs();

  try {
#if USED_DATA_TYPE == DATA_TYPE_RAW
    RawData rawData(jsonData);

    GraphData graphData(rawData);
#elif USED_DATA_TYPE == DATA_TYPE_GRAPH
    GraphData graphData(jsonData);
#endif

    GraphController controller(&graphData, graphResultDir);
    controller.start();

#if USED_DATA_TYPE == DATA_TYPE_RAW
    std::cout << "Done with graph" << std::endl;

    auto graphResult = controller.getResult();

    PathController pathController(&rawData, &graphData, graphResult, finalResultDir);

    pathController.start();
#endif

    std::cout << "Done" << std::endl;
  } catch (const std::exception &exception) {
    std::cout << "Caught exception: " << exception.what() << std::endl;
    return -1;
  }

  return 0;
}

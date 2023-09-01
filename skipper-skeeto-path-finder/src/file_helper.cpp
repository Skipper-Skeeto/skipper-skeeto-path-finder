#include "skipper-skeeto-path-finder/file_helper.h"

#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#endif

void FileHelper::createDir(const char *dirName) {
#ifdef _WIN32
  auto result = CreateDirectoryA(dirName, nullptr);
  if (result == ERROR_PATH_NOT_FOUND) {
    throw std::runtime_error(std::string("Could not create ") + dirName + ", might be since intermediate directories do not exist");
  }
#else
  system((std::string("mkdir ") + dirName).c_str());
#endif
}

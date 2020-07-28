#include "skipper-skeeto-path-finder/file_helper.h"

#include <string>

#ifdef _WIN32
#include <Windows.h>
#endif

void FileHelper::createDir(const char *dirName) {
#ifdef _WIN32
  CreateDirectoryA(dirName, nullptr);
#else
  system((std::string("mkdir ") + dirName).c_str());
#endif
}

#include "skipper-skeeto-path-finder/file_helper.h"

#ifdef _WIN32
#include <Windows.h>
#endif

void FileHelper::createDir(const char *dirName) {
#ifdef _WIN32
  CreateDirectoryA(dirName, nullptr);
#else
#error "Creating directory was not implemented for this platform"
#endif
}

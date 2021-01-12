#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

#include <boost/iostreams/device/mapped_file.hpp>

class MemoryMappedFilePool {
public:
  static void setAllocationDir(const std::string &dir);

  static void *addFile(size_t size);

  static void deleteFile(void *pointer, size_t expectedSize);

private:
  static std::string allocationDirPath;

  static std::mutex sharedMapMutex;
  static int nextAvailableIndex;
  static std::unordered_map<void *, std::pair<std::string, boost::iostreams::mapped_file>> memoryMappedfileMap;
};
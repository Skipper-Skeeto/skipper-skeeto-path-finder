#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

#include <boost/iostreams/device/mapped_file.hpp>

class MemoryMappedFilePool {
public:
  static MemoryMappedFilePool &getInstance();

  MemoryMappedFilePool(const MemoryMappedFilePool &) = delete;
  void operator=(const MemoryMappedFilePool &) = delete;

  void *addFile(size_t size);

  void deleteFile(void *pointer, size_t expectedSize);

private:
  MemoryMappedFilePool();

  mutable std::mutex sharedMapMutex;
  int nextAvailableIndex = 0;
  std::unordered_map<void *, std::pair<std::string, boost::iostreams::mapped_file>> memoryMappedfileMap;
};
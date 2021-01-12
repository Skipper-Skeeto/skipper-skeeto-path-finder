#include "skipper-skeeto-path-finder/memory_mapped_file_pool.h"

#include <cstdio>
#include <iostream>

std::string MemoryMappedFilePool::allocationDirPath;

std::mutex MemoryMappedFilePool::sharedMapMutex;

int MemoryMappedFilePool::nextAvailableIndex = 0;

std::unordered_map<void *, std::pair<std::string, boost::iostreams::mapped_file>> MemoryMappedFilePool::memoryMappedfileMap;

void MemoryMappedFilePool::setAllocationDir(const std::string &dir) {
  allocationDirPath = dir;
}

void *MemoryMappedFilePool::addFile(size_t size) {
  std::lock_guard<std::mutex> guardSharedMap(sharedMapMutex);
  boost::iostreams::mapped_file_params fileParams;

  fileParams.flags = boost::iostreams::mapped_file::readwrite;
  fileParams.new_file_size = size;

  auto fileName = allocationDirPath + "/mm-" + std::to_string(nextAvailableIndex++) + ".dat";
  fileParams.path = fileName;

  boost::iostreams::mapped_file file(fileParams);

  auto pointer = file.data();

  memoryMappedfileMap.emplace(pointer, std::make_pair(fileName, std::move(file)));

  return pointer;
}

void MemoryMappedFilePool::deleteFile(void *pointer, size_t expectedSize) {
  std::lock_guard<std::mutex> guardSharedMap(sharedMapMutex);
  auto iterator = memoryMappedfileMap.find(pointer);
  if (iterator == memoryMappedfileMap.end()) {
    std::cout << "Pointer not found! Expected size: " << expectedSize << std::endl;
    return;
  }

  auto &filePair = iterator->second;
  auto &file = filePair.second;
  if (expectedSize != file.size()) {
    std::cout << "Deallocation size (" << expectedSize << ") not matching file size (" << file.size() << ")" << std::endl;
  }

  file.close();

  const auto &fileName = filePair.first;
  std::remove(fileName.c_str());

  memoryMappedfileMap.erase(iterator);
}

#include "skipper-skeeto-path-finder/memory_mapped_file_pool.h"

#include "skipper-skeeto-path-finder/file_helper.h"
#include "skipper-skeeto-path-finder/info.h"

#include <cstdio>
#include <iostream>

MemoryMappedFilePool &MemoryMappedFilePool::getInstance() {
  static MemoryMappedFilePool instance;

  return instance;
}

void *MemoryMappedFilePool::addFile(size_t size) {
  std::lock_guard<std::mutex> guardSharedMap(sharedMapMutex);
  boost::iostreams::mapped_file_params fileParams;

  fileParams.flags = boost::iostreams::mapped_file::readwrite;
  fileParams.new_file_size = size;

  auto fileName = std::string(TEMP_STATES_DIR) + "/" + std::to_string(nextAvailableIndex++) + ".dat";
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

MemoryMappedFilePool::MemoryMappedFilePool() {
  FileHelper::createDir(TEMP_STATES_DIR);
}

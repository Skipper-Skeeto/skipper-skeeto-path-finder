#pragma once

#include "skipper-skeeto-path-finder/memory_mapped_file_pool.h"

template <typename ValueType>
class MemoryMappedFileAllocator {
public:
  typedef ValueType value_type;

  MemoryMappedFileAllocator() = default;

  template <typename OtherValueType>
  MemoryMappedFileAllocator(MemoryMappedFileAllocator<OtherValueType> &other) {}

  ValueType *allocate(size_t objectCount) {
    auto size = sizeof(ValueType) * objectCount;

    auto pointer = reinterpret_cast<ValueType *>(MemoryMappedFilePool::addFile(size));

    return pointer;
  }

  void deallocate(ValueType *pointer, size_t objectCount) {
    auto expectedSize = sizeof(ValueType) * objectCount;

    MemoryMappedFilePool::deleteFile(pointer, expectedSize);
  }

private:
  friend class MemoryMappedFileAllocator;
  template <class LeftValueType, class RightValueType>
  friend bool operator==(const MemoryMappedFileAllocator<LeftValueType> &, const MemoryMappedFileAllocator<RightValueType> &);
};

template <class LeftValueType, class RightValueType>
bool operator==(const MemoryMappedFileAllocator<LeftValueType> &lhs, const MemoryMappedFileAllocator<RightValueType> &rhs) {
  // Just like the std::allocator, this allocator is always stateless and thus they are all equal
  return true;
}

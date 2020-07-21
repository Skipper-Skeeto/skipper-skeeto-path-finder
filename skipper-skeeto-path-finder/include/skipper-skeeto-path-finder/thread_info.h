#pragma once

#include <mutex>
#include <thread>

class ThreadInfo {
public:
  ThreadInfo() = default;

  ThreadInfo(ThreadInfo &&) = default;

  void init(std::thread &&thread, unsigned char identifier);

  void setDone();

  bool joinIfDone();

  unsigned char getIdentifier() const;

private:
  std::thread thread;
  unsigned char identifier = '?';

  std::mutex doneMutex;
  bool isDone = false;
};
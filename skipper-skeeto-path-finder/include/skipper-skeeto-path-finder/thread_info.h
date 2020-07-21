#pragma once

#include <mutex>
#include <thread>

class ThreadInfo {
public:
  ThreadInfo() = default;

  ThreadInfo(ThreadInfo &&) = default;

  void setThread(std::thread &&thread);

  void setDone();

  bool joinIfDone();

private:
  std::thread thread;

  std::mutex doneMutex;
  bool isDone = false;
};
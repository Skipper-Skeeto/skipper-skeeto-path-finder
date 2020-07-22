#pragma once

#include <mutex>
#include <thread>

class ThreadInfo {
public:
  ThreadInfo(std::thread &&thread, unsigned char identifier);

  ThreadInfo(ThreadInfo &&) = default;

  void setDone();

  bool joinIfDone();

  unsigned char getIdentifier() const;

  std::thread::id getThreadIdentifier() const;

private:
  std::thread thread;
  unsigned char identifier = '?';

  std::mutex doneMutex;
  bool isDone = false;
};
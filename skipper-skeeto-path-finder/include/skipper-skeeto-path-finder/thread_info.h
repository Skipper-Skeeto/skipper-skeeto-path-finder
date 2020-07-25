#pragma once

#include <mutex>
#include <thread>

class ThreadInfo {
public:
  ThreadInfo(std::thread &&thread, unsigned char identifier);

  ThreadInfo(ThreadInfo &&) = default;

  void setDone();

  bool joinIfDone();

  void setVisitedRoomsCount(unsigned char count);

  unsigned char getVisitedRoomsCount() const;

  void setPaused(bool isPaused);

  bool isPaused() const;

  void waitForUnpaused() const;

  unsigned char getIdentifier() const;

  std::thread::id getThreadIdentifier() const;

private:
  std::thread thread;
  unsigned char identifier = '?';
  mutable std::mutex threadMutex;

  mutable std::mutex visitedRoomsCountMutex;
  unsigned char visitedRoomsCount = 255;

  mutable std::mutex isPausedMutex;
  bool paused = false;

  std::mutex doneMutex;
  bool isDone = false;
};
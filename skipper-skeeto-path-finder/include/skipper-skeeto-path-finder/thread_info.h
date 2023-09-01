#pragma once

#include <mutex>
#include <thread>

class ThreadInfo {
public:
  ThreadInfo(std::thread &&thread, unsigned char identifier);

  ThreadInfo(ThreadInfo &&) = default;

  void setDone();

  bool joinIfDone();

  void setHighScore(unsigned char score);

  unsigned char getHighScore() const;

  void setPaused(bool isPaused);

  bool isPaused() const;

  void waitForUnpaused();

  bool isWaiting() const;

  unsigned char getIdentifier() const;

  std::thread::id getThreadIdentifier() const;

private:
  void setIsWaiting(bool waiting);

  std::thread thread;
  unsigned char identifier = '?';
  mutable std::mutex threadMutex;

  mutable std::mutex highScoreMutex;
  unsigned char highScore = 255;

  mutable std::mutex isPausedMutex;
  bool paused = false;

  mutable std::mutex isWaitingMutex;
  bool waiting = false;

  std::mutex doneMutex;
  bool isDone = false;
};
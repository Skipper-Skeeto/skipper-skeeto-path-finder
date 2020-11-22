#include "skipper-skeeto-path-finder/thread_info.h"

ThreadInfo::ThreadInfo(std::thread &&thread, unsigned char identifier) {
  this->thread = std::move(thread);
  this->identifier = identifier;
}

void ThreadInfo::setDone() {
  std::lock_guard<std::mutex> doneGuard(doneMutex);

  isDone = true;
}

bool ThreadInfo::joinIfDone() {
  std::lock_guard<std::mutex> doneGuard(doneMutex);

  if (isDone) {
    thread.join();
  }

  return isDone;
}

void ThreadInfo::setHighScore(unsigned char score) {
  std::lock_guard<std::mutex> highScoreGuard(highScoreMutex);

  highScore = score;
}

unsigned char ThreadInfo::getHighScore() const {
  std::lock_guard<std::mutex> highScoreGuard(highScoreMutex);

  return highScore;
}

void ThreadInfo::setPaused(bool isPaused) {
  std::lock_guard<std::mutex> isPausedGuard(isPausedMutex);

  if (paused == isPaused) {
    return;
  }

  paused = isPaused;

  if (isPaused) {
    threadMutex.lock();
  } else {
    threadMutex.unlock();
  }
}

bool ThreadInfo::isPaused() const {
  std::lock_guard<std::mutex> isPausedGuard(isPausedMutex);

  return paused;
}

void ThreadInfo::waitForUnpaused() const {
  std::lock_guard<std::mutex> guard(threadMutex);
}

unsigned char ThreadInfo::getIdentifier() const {
  return identifier;
}

std::thread::id ThreadInfo::getThreadIdentifier() const {
  return thread.get_id();
}

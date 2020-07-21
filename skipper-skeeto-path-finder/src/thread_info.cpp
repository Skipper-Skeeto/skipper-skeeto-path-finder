#include "skipper-skeeto-path-finder/thread_info.h"

void ThreadInfo::setThread(std::thread &&thread) {
  this->thread = std::move(thread);
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

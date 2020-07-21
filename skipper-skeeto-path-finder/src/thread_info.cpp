#include "skipper-skeeto-path-finder/thread_info.h"

void ThreadInfo::init(std::thread &&thread, unsigned char identifier) {
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

unsigned char ThreadInfo::getIdentifier() const {
  return identifier;
}

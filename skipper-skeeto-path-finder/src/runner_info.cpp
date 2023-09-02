#include "skipper-skeeto-path-finder/runner_info.h"

std::mutex RunnerInfo::identifierMutex;
unsigned int RunnerInfo::nextIdentifier = 0;

RunnerInfo::RunnerInfo(const std::vector<char> &parentPath) : identifier(createIdentifier()),
                                                              parentPath(parentPath),
                                                              waitForResults(false) {
}

RunnerInfo::RunnerInfo(const RunnerInfo &other) : identifier(other.identifier),
                                                  parentPath(other.parentPath),
                                                  highscore(other.highscore),
                                                  localMaxDistance(other.localMaxDistance),
                                                  waitForResults(other.waitForResults) {
}

unsigned int RunnerInfo::getIdentifier() const {
  return identifier;
}

void RunnerInfo::setHighScore(unsigned char score) {
  std::lock_guard<std::mutex> highscoreGuard(highscoreMutex);

  highscore = score;
}

unsigned char RunnerInfo::getHighScore() const {
  std::lock_guard<std::mutex> highscoreGuard(highscoreMutex);

  return highscore;
}

void RunnerInfo::setLocalMaxDistance(unsigned char score) {
  localMaxDistance = score;
}

unsigned char RunnerInfo::getLocalMaxDistance() const {
  return localMaxDistance;
}

void RunnerInfo::setHandleWaiting(bool handle) {
  handleWaiting = handle;
}

bool RunnerInfo::shouldHandleWaiting() const {
  return handleWaiting;
}

int RunnerInfo::getVisitedVerticesCount() const {
  return parentPath.size();
}

std::vector<char> RunnerInfo::getRoute() const {
  return parentPath;
}

RunnerInfo RunnerInfo::makeSubRunner(char vertex) {
  auto newParentPath = parentPath;
  newParentPath.push_back(vertex);

  return {newParentPath};
}

void RunnerInfo::setWaitForResults(bool shouldWait) {
  std::lock_guard<std::mutex> waitForResultsGuard(waitForResultsMutex);

  waitForResults = shouldWait;
}

bool RunnerInfo::shouldWaitForResults() const {
  std::lock_guard<std::mutex> waitForResultsGuard(waitForResultsMutex);

  return waitForResults;
}

void RunnerInfo::setLatestPickReason(char reason) {
  lastestPickReason = reason;
}

char RunnerInfo::getLatestPickReason() const {
  return lastestPickReason;
}

unsigned int RunnerInfo::createIdentifier() {
  std::lock_guard<std::mutex> identifierGuard(identifierMutex);

  return nextIdentifier++;
}

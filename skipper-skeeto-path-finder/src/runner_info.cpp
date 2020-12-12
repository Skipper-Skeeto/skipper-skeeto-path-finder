#include "skipper-skeeto-path-finder/runner_info.h"

std::mutex RunnerInfo::identifierMutex;
unsigned int RunnerInfo::nextIdentifier = 0;

RunnerInfo::RunnerInfo(const std::vector<char> &parentPath) : identifier(createIdentifier()),
                                                              parentPath(parentPath) {
}

unsigned int RunnerInfo::getIdentifier() const {
  return identifier;
}

void RunnerInfo::setHighScore(unsigned char score) {
  highscore = score;
}

unsigned char RunnerInfo::getHighScore() const {
  return highscore;
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

  RunnerInfo subRunner(newParentPath);
  subRunner.setHighScore(highscore);

  return subRunner;
}

unsigned int RunnerInfo::createIdentifier() {
  std::lock_guard<std::mutex> identifierGuard(identifierMutex);

  return nextIdentifier++;
}

#pragma once

#include "skipper-skeeto-path-finder/info.h"

#include <mutex>
#include <vector>

class RunnerInfo {
public:
  RunnerInfo(const std::vector<char> &parentPath);

  RunnerInfo(const RunnerInfo &other);

  unsigned int getIdentifier() const;

  void setHighScore(unsigned char score);

  unsigned char getHighScore() const;

  void setLocalMaxDistance(unsigned char score);

  unsigned char getLocalMaxDistance() const;

  void setHandleWaiting(bool handle);

  bool shouldHandleWaiting() const;

  int getVisitedVerticesCount() const;

  std::vector<char> getRoute() const;

  RunnerInfo makeSubRunner(char vertices);
 
  void setWaitForResults(bool shouldWait);

  bool shouldWaitForResults() const;

  // Note: Should only be used from the common state for now as it's not threadsafe
  void setLatestPickReason(char reason);

  char getLatestPickReason() const;

private:
  static unsigned int createIdentifier();

  static std::mutex identifierMutex;
  static unsigned int nextIdentifier;

  mutable std::mutex highscoreMutex;
  unsigned char highscore = 255;

  mutable std::mutex waitForResultsMutex;
  bool waitForResults;

  std::vector<char> parentPath;
  unsigned int identifier;
  unsigned char localMaxDistance;
  bool handleWaiting = false;
  char lastestPickReason = '?';
};
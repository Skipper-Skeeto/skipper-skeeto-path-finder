#pragma once

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

  int getVisitedVerticesCount() const;

  std::vector<char> getRoute() const;

  RunnerInfo makeSubRunner(char vertices);

private:
  static unsigned int createIdentifier();

  static std::mutex identifierMutex;
  static unsigned int nextIdentifier;

  mutable std::mutex highscoreMutex;
  unsigned char highscore = 255;

  std::vector<char> parentPath;
  unsigned int identifier;
  unsigned char localMaxDistance;
};
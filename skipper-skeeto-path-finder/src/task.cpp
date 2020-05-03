#include "skipper-skeeto-path-finder/task.h"

std::string Task::getStepDescription() const {
  return std::string("  - Complete task: ") + key;
}

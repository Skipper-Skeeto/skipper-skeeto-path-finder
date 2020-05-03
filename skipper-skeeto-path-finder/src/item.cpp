#include "skipper-skeeto-path-finder/item.h"

std::string Item::getStepDescription() const {
  return std::string("  - Pick up: ") + key;
}

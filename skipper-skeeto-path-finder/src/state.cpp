#include "skipper-skeeto-path-finder/state.h"

void State::clear() {
  state = 0;
}

bool State::isClean() const {
  return state == 0;
}

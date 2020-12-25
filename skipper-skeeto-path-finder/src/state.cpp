#include "skipper-skeeto-path-finder/state.h"

void State::setBit(unsigned char index) {
  state |= (1ULL << index);
}

void State::clear() {
  state = 0;
}

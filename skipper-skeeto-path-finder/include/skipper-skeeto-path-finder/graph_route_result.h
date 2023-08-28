#pragma once

#include "skipper-skeeto-path-finder/info.h"

enum class GraphRouteResult {
  Continue,
#ifdef FOUND_BEST_DISTANCE
  WaitForResult,
#endif // FOUND_BEST_DISTANCE
  Stop,
  PoolFull
};
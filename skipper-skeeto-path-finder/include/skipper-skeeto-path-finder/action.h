#pragma once

#include <string>

class Action {
public:
  virtual std::string getStepDescription() const = 0;
};
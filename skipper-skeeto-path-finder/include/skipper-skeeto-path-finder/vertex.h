#pragma once

#include <string>
#include <vector>

class Scene;
class Item;
class Task;

class Vertex {
public:
  std::string key;

  const Scene *furthestScene = nullptr;

  bool isOneTime = false;

  std::vector<const Item *> items;

  std::vector<const Task *> tasks;
};
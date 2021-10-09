#pragma once

#include <vector>

class Room;
class Item;
class Task;

class Vertex {
public:
  const Room *furthestRoom = nullptr;

  bool isOneTime = false;

  std::vector<const Item *> items;

  std::vector<const Task *> tasks;
};
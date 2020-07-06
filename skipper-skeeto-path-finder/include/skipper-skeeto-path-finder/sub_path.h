#pragma once

#include <memory>
#include <vector>

class Room;

class SubPath {
public:
  SubPath() = default;

  SubPath(const Room *room, const std::shared_ptr<SubPath> &parent);

  bool isEmpty() const;

  const Room *getRoom() const;

  std::vector<const Room *> getRooms() const;

  int size() const;

private:
  const Room *room = nullptr;
  std::shared_ptr<SubPath> parent;
};
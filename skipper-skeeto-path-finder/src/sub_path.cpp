#include "skipper-skeeto-path-finder/sub_path.h"

SubPath::SubPath(const Room *room, const std::shared_ptr<SubPath> &parent) {
  this->room = room;
  if (!parent->isEmpty()) {
    this->parent = parent;
  }
}

bool SubPath::isEmpty() const {
  return room == nullptr;
}

const Room *SubPath::getRoom() const {
  return room;
}

std::vector<const Room *> SubPath::getRooms() const {
  if (parent != nullptr) {
    auto rooms = parent->getRooms();
    rooms.push_back(room);
    return rooms;
  } else {
    return std::vector<const Room *>{room};
  }
}

int SubPath::size() const {
  if (parent != nullptr) {
    return 1 + parent->size();
  } else {
    return 1;
  }
}

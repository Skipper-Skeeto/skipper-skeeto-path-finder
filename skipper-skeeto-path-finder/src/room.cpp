#include "skipper-skeeto-path-finder/room.h"

void Room::setupNextRooms(Room *left, Room *right, Room *up, Room *down) {
  rooms.clear();

  if (left != nullptr) {
    rooms.push_back(left);
  }

  if (right != nullptr) {
    rooms.push_back(right);
  }

  if (up != nullptr) {
    rooms.push_back(up);
  }

  if (down != nullptr) {
    rooms.push_back(down);
  }
}

std::vector<const Room *> Room::getNextRooms() const {
  return rooms;
}

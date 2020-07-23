#include "skipper-skeeto-path-finder/room.h"

std::string Room::getStepDescription() const {
  return std::string("- Move to: ") + key;
}

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

  roomIndexes.clear();
  for (auto room : rooms) {
    roomIndexes.push_back(room->roomIndex);
  }
}

std::vector<const Room *> Room::getNextRooms() const {
  return rooms;
}

std::vector<unsigned char> Room::getNextRoomIndexes() const {
  return roomIndexes;
}
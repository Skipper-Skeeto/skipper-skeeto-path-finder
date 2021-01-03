#include "skipper-skeeto-path-finder/room.h"

std::string Room::getStepDescription() const {
  return std::string("- Move to: ") + key;
}

void Room::setupNextRooms(std::vector<const Room *> rooms) {
  this->rooms = rooms;

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
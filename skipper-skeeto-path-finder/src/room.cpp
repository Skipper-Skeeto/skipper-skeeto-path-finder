#include "skipper-skeeto-path-finder/room.h"

Room::Room(const std::string &key, int uniqueIndex)
    : key(key), uniqueIndex(uniqueIndex) {
}

std::string Room::getStepDescription() const {
  return std::string("- Move to: ") + key;
}

const std::string &Room::getKey() const {
  return key;
}

int Room::getUniqueIndex() const {
  return uniqueIndex;
}

void Room::setTaskObstacle(const Task *taskObstacle) {
  this->taskObstacle = taskObstacle;
}

const Task *Room::getTaskObstacle() const {
  return taskObstacle;
}

void Room::setupNextRooms(const std::vector<const Room *> &rooms) {
  this->rooms = rooms;

  roomIndexes.clear();
  for (auto room : rooms) {
    roomIndexes.push_back(room->getUniqueIndex());
  }
}

const std::vector<const Room *> &Room::getNextRooms() const {
  return rooms;
}

const std::vector<unsigned char> &Room::getNextRoomIndexes() const {
  return roomIndexes;
}
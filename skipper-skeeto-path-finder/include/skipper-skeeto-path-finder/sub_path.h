#pragma once

#include <iostream>

class Room;

class SubPath {
public:
  SubPath() = default;

  SubPath(const Room *newRoom, const SubPath &parent);

  SubPath(std::istream &instream);

  void serialize(std::ostream &outstream) const;

  bool isEmpty() const;

  int getLastRoomIndex() const;

  int visitedRoomsCount() const;

private:
  unsigned char lastRoomIndex = 0;
  unsigned char size = 0;
};
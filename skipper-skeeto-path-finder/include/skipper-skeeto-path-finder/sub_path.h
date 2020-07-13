#pragma once

class Room;

class SubPath {
public:
  SubPath() = default;

  SubPath(const Room *newRoom, const SubPath &parent);

  bool isEmpty() const;

  const Room *getLastRoom() const;

  int visitedRoomsCount() const;

private:
  const Room *lastRoom = nullptr;
  unsigned char size = 0;
};
#pragma once

class State {
public:
  template <size_t StartIndex, size_t Count>
  void setBits(unsigned long long int newState) {
    state &= (~(((1ULL << Count) - 1) << StartIndex));
    state |= (newState << StartIndex);
  };

  template <size_t StartIndex, size_t Count>
  unsigned long long int getBits() const {
    return (state >> StartIndex) & ((1ULL << Count) - 1);
  };

  template <size_t StartIndex, size_t Count>
  bool meetsCondition(unsigned long long int condition) const {
    return (getBits<StartIndex, Count>() & condition) == condition;
  }

  void setBit(unsigned char index);

  void clear();

private:
  unsigned long long int state{};
};

//
// EventNetwork.cpp
//
#include "EventNetwork.h"


df::EventNetwork::EventNetwork() {
  setType(NETWORK_EVENT);
};


df::EventNetwork::EventNetwork(int initial_bytes) {
  bytes = initial_bytes;
  setType(NETWORK_EVENT);
};


void df::EventNetwork::setBytes(int new_bytes) {
  bytes = new_bytes;
};


int df::EventNetwork::getBytes() const {
  return bytes;
};

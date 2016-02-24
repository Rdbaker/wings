//
// Sentry.cpp
//


// Engine includes.
#include "Sentry.h"
#include "include/EventStep.h"
#include "EventNetwork.h"
#include "NetworkManager.h"

df::Sentry::Sentry () {
  // Need to update rate control each step.
  registerInterest(df::STEP_EVENT);

  // Set object type.
  setType("Sentry");
  setSolidness(df::SPECTRAL);
}


void df::Sentry::doStep() {
  df::NetworkManager &net_manager = df::NetworkManager::getInstance();
  int numbytes = net_manager.isData();
  // check if there is at least an "int" worth of data in the socket
  if(numbytes >= sizeof(int)) {
    // generate a network event
    net_manager.onEvent(new EventNetwork(numbytes));
  }
  // no-op for now
}


int df::Sentry::eventHandler(const Event *p_e) {
  if(p_e->getType() == NETWORK_EVENT) {
    doStep();
  }
  return 0;
}

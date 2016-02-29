//
// Explosion.cpp
//

// Engine includes.
#include "include/EventStep.h"
#include "include/GameManager.h"
#include "include/LogManager.h"
#include "include/ResourceManager.h"
#include "include/WorldManager.h"
#include "NetworkManager.h"

// Game includes.
#include "Explosion.h"
#include "Role.h"

Explosion::Explosion() {
  registerInterest(df::STEP_EVENT);

  // Link to "explosion" sprite.
  df::ResourceManager &resource_manager = df::ResourceManager::getInstance();
  df::Sprite *p_temp_sprite = resource_manager.getSprite("explosion");
  if (!p_temp_sprite) {
    df::LogManager &log_manager = df::LogManager::getInstance();
    log_manager.writeLog("Explosion::Explosion(): Warning! Sprite '%s' not found", 
		"explosion");
    return;
  }
  setSprite(p_temp_sprite);

  setType("Explosion");

  time_to_live =  getSprite()->getFrameCount();
  setSolidness(df::SPECTRAL);
  Role &role = Role::getInstance();
  if(role.isHost())
    role.registerSyncObj(this);
}

// Handle event.
// Return 0 if ignored, else 1.
int Explosion::eventHandler(const df::Event *p_e) {

  if (p_e->getType() == df::STEP_EVENT) {
    step();
    return 1;
  }

  // If get here, have ignored this event.
  return 0;
}

// Count down until explosion finished.
void Explosion::step() {
  time_to_live--;
  Role &role = Role::getInstance();
  if (time_to_live <= 0 && role.isHost()){
    df::WorldManager &world_manager = df::WorldManager::getInstance();
    world_manager.markForDelete(this);
    role.unregisterSyncObj(this);
    df::NetworkManager &net_manager = df::NetworkManager::getInstance();
    std::string buf1 = getId() > 99 ? "0112Xid:" + std::to_string(getId()) : "0102Xid:" + std::to_string(getId());
    net_manager.send((void *)buf1.c_str(), buf1.length());
  }
}

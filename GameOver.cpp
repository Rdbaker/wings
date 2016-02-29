//
// GameOver.cpp
//

// Engine includes.
#include "include/EventStep.h"
#include "include/LogManager.h"
#include "include/ResourceManager.h"
#include "include/GameManager.h"
#include "include/WorldManager.h"
#include "NetworkManager.h"
#include "Role.h"

// Game includes.
#include "GameOver.h"
#include "GameStart.h"

GameOver::GameOver() {
  df::NetworkManager &net_mgr = df::NetworkManager::getInstance();
  Role &role = Role::getInstance();
  if(role.isHost()) {
    printf("About to send the shutdown signal\n");
    std::string buf = "GAMEOVER";
    net_mgr.send((void*)buf.c_str(), 8);
  }
  net_mgr.shutDown();


  setType("GameOver");

  // Link to "message" sprite.
  df::ResourceManager &resource_manager = df::ResourceManager::getInstance();
  df::Sprite *p_temp_sprite = resource_manager.getSprite("gameover");
  if (!p_temp_sprite) {
    df::LogManager &log_manager = df::LogManager::getInstance();
    log_manager.writeLog("GameOver::GameOver(): Warning! Sprite 'gameover' not found");
  } else {
    setSprite(p_temp_sprite);
    setSpriteSlowdown(15);
    setTransparency('#');    // Transparent character.
    time_to_live = p_temp_sprite->getFrameCount() * 15;
  }

  // Put in center of window.
  setLocation(df::CENTER_CENTER);

  // Register for step event.
  registerInterest(df::STEP_EVENT);

  // Play "game over" sound.
  df::Sound *p_sound = df::ResourceManager::getInstance().getSound("game over");
  p_sound->play();

}

// When done, game over so reset things for GameStart.
GameOver::~GameOver() {
}

// Handle event.
// Return 0 if ignored, else 1.
int GameOver::eventHandler(const df::Event *p_e) {

  if (p_e->getType() == df::STEP_EVENT) {
    step();
    return 1;
  }

  // If get here, have ignored this event.
  return 0;
}

// Count down to end of message.
void GameOver::step() {
  time_to_live--;
  if (time_to_live <= 0) {
    // the game ends when one person dies
    exit(0);
  }
}

// Override default draw so as not to display "value".
void GameOver::draw() {
  df::Object::draw();
}

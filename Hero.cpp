//
// Hero.cpp
//

// Engine includes.
#include "include/EventMouse.h"
#include "include/EventStep.h"
#include "include/EventView.h"
#include "include/GameManager.h"
#include "include/LogManager.h"
#include "include/ResourceManager.h"
#include "include/WorldManager.h"
#include "NetworkManager.h"

// Game includes.
#include "Bullet.h"
#include "EventNuke.h"
#include "Explosion.h"
#include "GameOver.h"
#include "Hero.h"
#include "Role.h"

Hero::Hero(bool isClient) : isClient(isClient) {

  // Link to "ship" sprite.
  df::ResourceManager &resource_manager = df::ResourceManager::getInstance();
  df::LogManager &log_manager = df::LogManager::getInstance();
  df::Sprite *p_temp_sprite;
  if(isClient) {
    p_temp_sprite = resource_manager.getSprite("client");
  } else {
    p_temp_sprite = resource_manager.getSprite("ship");
  }
  if (!p_temp_sprite) {
    log_manager.writeLog("Hero::Hero(): Warning! Sprite '%s' not found",
			 "ship");
  } else {
    setSprite(p_temp_sprite);
    setSpriteSlowdown(3);  // 1/3 speed animation.
    setTransparency();	   // Transparent sprite.
  }

  // Player controls hero, so register for input events.
  registerInterest(df::KEYBOARD_EVENT);
  registerInterest(df::MOUSE_EVENT);

  // Need to update rate control each step.
  registerInterest(df::STEP_EVENT);

  // Set object type.
  setType("Hero");

  // Set starting location.
  df::WorldManager &world_manager = df::WorldManager::getInstance();
  if(isClient) {
    df::Position pos(7, world_manager.getBoundary().getVertical()/2+4);
    setPosition(pos);
  } else {
    df::Position pos(7, world_manager.getBoundary().getVertical()/2-4);
    setPosition(pos);
  }

  // Create reticle for firing bullets.
  p_reticle = new Reticle();
  p_reticle->draw();

  // Set attributes that control actions.
  move_slowdown = 2;
  move_countdown = move_slowdown;
  fire_slowdown = 30;
  fire_countdown = fire_slowdown;
  nuke_count = 1;
  Role &role = Role::getInstance();
  if(role.isHost()) {
    role.registerSyncObj(this);
    log_manager.writeLog("Hero::Hero(): INFO! registered with %s", serialize().c_str());
  }
}

Hero::~Hero() {
  df::LogManager &log_manager = df::LogManager::getInstance();
  log_manager.writeLog("Hero::~Hero(): INFO! destroying hero");
  // Create GameOver object.
  GameOver *p_go = new GameOver;

  // Make big explosion.
  for (int i=-8; i<=8; i+=5) {
    for (int j=-5; j<=5; j+=3) {
      df::Position temp_pos = this->getPosition();
      temp_pos.setX(this->getPosition().getX() + i);
      temp_pos.setY(this->getPosition().getY() + j);
      Explosion *p_explosion = new Explosion;
      p_explosion -> setPosition(temp_pos);
    }
  }

  // Mark Reticle for deletion.
  df::WorldManager::getInstance().markForDelete(p_reticle);

  Role &role = Role::getInstance();
  role.unregisterSyncObj(this);
}

// Handle event.
// Return 0 if ignored, else 1.
int Hero::eventHandler(const df::Event *p_e) {
  Role &role = Role::getInstance();

  if (p_e->getType() == df::KEYBOARD_EVENT) {
    // if the ship is the host and so is the server
    const df::EventKeyboard *p_keyboard_event = dynamic_cast <const df::EventKeyboard *> (p_e);
    if(role.isHost() && !isClient) {
      kbd(p_keyboard_event);
    } else if(!role.isHost() && isClient) {
      // the ship and server is the client
      // send the event to the host server
      df::NetworkManager &net_mgr = df::NetworkManager::getInstance();
      std::string msg = "KEY:" + std::to_string(p_keyboard_event->getKey());
      printf("about to send the event: %s\n", msg.c_str());
      net_mgr.send((void*)msg.c_str(), msg.length());
    }
    return 1;
  }

  if (p_e->getType() == df::MOUSE_EVENT) {
    if(role.isHost() && !isClient) {
      const df::EventMouse *p_mouse_event = dynamic_cast <const df::EventMouse *> (p_e);
      mouse(p_mouse_event);
    } else if(!role.isHost() && isClient) {
      // the ship and server is the client
      // send the event
    }
    return 1;
  }

  if (p_e->getType() == df::STEP_EVENT) {
    if(role.isHost()) {
      step();
    }
    return 1;
  }

  // If get here, have ignored this event.
  return 0;
}

// Take appropriate action according to mouse action.
void Hero::mouse(const df::EventMouse *p_mouse_event) {

  // Pressed button?
  if ((p_mouse_event->getMouseAction() == df::CLICKED) &&
      (p_mouse_event->getMouseButton() == df::Mouse::LEFT))
    fire(p_mouse_event->getMousePosition());
}

// Take appropriate action according to key pressed.
void Hero::kbd(const df::EventKeyboard *p_keyboard_event) {

  switch(p_keyboard_event->getKey()) {
  case df::Keyboard::W:       // up
    if (p_keyboard_event->getKeyboardAction() == df::KEY_DOWN)
      move(-1);
    break;
  case df::Keyboard::S:       // down
    if (p_keyboard_event->getKeyboardAction() == df::KEY_DOWN)
      move(+1);
    break;
  case df::Keyboard::SPACE:   // nuke!
    if (p_keyboard_event->getKeyboardAction() == df::KEY_PRESSED)
      nuke();
    break;
 case df::Keyboard::Q:        // quit
   if (p_keyboard_event->getKeyboardAction() == df::KEY_PRESSED) {
     df::WorldManager &world_manager = df::WorldManager::getInstance();
     df::NetworkManager &net_mgr = df::NetworkManager::getInstance();
     new GameOver;
     world_manager.markForDelete(this);
    }
    break;
 default:
    break;
  };

  return;
}

// Move up or down.
void Hero::move(int dy) {

  // See if time to move.
  if (move_countdown > 0)
    return;
  move_countdown = move_slowdown;

  // If stays on window, allow move.
  df::Position new_pos(getPosition().getX(), getPosition().getY() + dy);
  df::WorldManager &world_manager = df::WorldManager::getInstance();
  if ((new_pos.getY() > 3) &&
      (new_pos.getY() < world_manager.getBoundary().getVertical()-1))
    world_manager.moveObject(this, new_pos);
}

// Fire bullet towards target.
void Hero::fire(df::Position target) {

  // See if time to fire.
  if (fire_countdown > 0)
    return;
  fire_countdown = fire_slowdown;

  // Fire Bullet towards target.
  df::Position *pos = new df::Position(getPosition().getX(), getPosition().getY());
  Bullet *p = new Bullet(*pos);
  p->setYVelocity((float) (target.getY() - getPosition().getY()) /
		  (float) (target.getX() - getPosition().getX()));

  // Play "fire" sound.
  df::Sound *p_sound = df::ResourceManager::getInstance().getSound("fire");
  p_sound->play();
}

// Decrease rate restriction counters.
void Hero::step() {

  // Move countdown.
  move_countdown--;
  if (move_countdown < 0)
    move_countdown = 0;

  // Fire countdown.
  fire_countdown--;
  if (fire_countdown < 0)
    fire_countdown = 0;
}

// Send "nuke" event to all objects.
void Hero::nuke() {

  // Check if nukes left.
  if (!nuke_count)
    return;
  nuke_count--;

  // Create "nuke" event and send to interested Objects.
  df::WorldManager &world_manager = df::WorldManager::getInstance();
  EventNuke nuke;
  world_manager.onEvent(&nuke);

  // Send "view" event do decrease number of nukes to interested ViewObjects.
  df::EventView ev("Nukes", -1, true);
  world_manager.onEvent(&ev);

  // Play "nuke" sound.
  df::Sound *p_sound = df::ResourceManager::getInstance().getSound("nuke");
  p_sound->play();
}

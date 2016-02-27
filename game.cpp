//
// game.cpp
//

// Engine includes.
#include "include/GameManager.h"
#include "include/LogManager.h"
#include "NetworkManager.h"
#include "include/Pause.h"
#include "include/ResourceManager.h"

// Game includes.
#include "GameStart.h"
#include "Star.h"
#include "Role.h"

// Function prototypes.
void loadResources(void);
void populateWorld(void);

int main(int argc, char *argv[]) {
  df::LogManager &log_manager = df::LogManager::getInstance();
  Role &role = Role::getInstance();

  // Start up game manager.
  df::GameManager &game_manager = df::GameManager::getInstance();
  if (game_manager.startUp())  {
    log_manager.writeLog("Error starting game manager!");
    game_manager.shutDown();
    return 0;
  }

  df::NetworkManager &net_manager = df::NetworkManager::getInstance();

  // get the host
  char host[128];
  memset(host, 0, 128);
  if(argc > 2) {
    strncpy(host, argv[2], 127);
  }

  if(argc == 1) {
    role.setHost(true);
    printf("Waiting for client to connect before starting...\n");
  } else {
    role.setHost(false);
  }

  if (net_manager.startUp(argc == 1, host))  {
    log_manager.writeLog("Error starting network manager!");
    net_manager.shutDown();
    return 0;
  }


  // Set flush of logfile during development (when done, make false).
  log_manager.setFlush(true);

  // Show splash screen.
  df::splash();

  // Load game resources.
  loadResources();
  // Populate game world with some objects.
  populateWorld();

  // Enable player to pause game.
  new df::Pause(df::Keyboard::F10);

  // Run game (this blocks until game loop is over).
  game_manager.run();

  // Shut everything down.
  game_manager.shutDown();
}

// Load resources (sprites, sound effects, music).
void loadResources(void) {
  df::ResourceManager &resource_manager = df::ResourceManager::getInstance();
  resource_manager.loadSprite("sprites/saucer-spr.txt", "saucer");
  resource_manager.loadSprite("sprites/ship-spr.txt", "ship");
  resource_manager.loadSprite("sprites/client-spr.txt", "client");
  resource_manager.loadSprite("sprites/bullet-spr.txt", "bullet");
  resource_manager.loadSprite("sprites/explosion-spr.txt", "explosion");
  resource_manager.loadSprite("sprites/gamestart-spr.txt", "gamestart");
  resource_manager.loadSprite("sprites/gameover-spr.txt", "gameover");
  resource_manager.loadSound("sounds/fire.wav", "fire");
  resource_manager.loadSound("sounds/explode.wav", "explode");
  resource_manager.loadSound("sounds/nuke.wav", "nuke");
  resource_manager.loadSound("sounds/game-over.wav", "game over");
  resource_manager.loadMusic("sounds/start-music.wav", "start music");
}

// Populate world with some objects.
void populateWorld(void) {

  // Spawn some Stars.
  for (int i=0; i<16; i++)
    new Star;

  // Create GameStart object.
  new GameStart();
}

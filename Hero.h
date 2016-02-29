//
// Hero.h
//

#include "include/EventKeyboard.h"
#include "include/EventMouse.h"
#include "include/Object.h"
#include "Reticle.h"

class Hero : public df::Object {

 private:
  Reticle *p_reticle;
  int fire_slowdown;
  int fire_countdown;
  int move_slowdown;
  int move_countdown;
  int nuke_count;
  void mouse(const df::EventMouse *p_mouse_event);
  void kbd(const df::EventKeyboard *p_keyboard_event);
  void move(int dy);
  void fire(df::Position target);
  void step();
  void nuke();
  const bool isClient;

 public:
  Hero(bool isClient=false);
  ~Hero();
  int eventHandler(const df::Event *p_e);
};

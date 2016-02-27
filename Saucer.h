//
// Saucer.h
//

#include "include/Object.h"
#include "include/EventCollision.h"
 
class Saucer : public df::Object {
 private:
  void moveToStart();
  void out();
  void hit(const df::EventCollision *p_collision_event);
 
 public:
  Saucer();
  ~Saucer();
 int eventHandler(const df::Event *p_e);
};

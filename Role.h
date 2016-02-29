//
// Role class
//
// Indicate whether game is Host or Client.
//

#ifndef __ROLE_H__
#define __ROLE_H__

#include "include/Object.h"
#include "include/ObjectList.h"

class Role : public df::Object {

 private:
  Role();                       // Private since a singleton.
  Role (Role const&);           // Don't allow copy.
  void operator=(Role const&);  // Don't allow assignment.
  bool is_host;                 // True if hosting game.
  bool has_started;
  df::ObjectList obj_list;      // list of objects to synchronize
  bool is_game_over;

 public:
  // Get the one and only instance of the Role.
  static Role &getInstance() {
    static Role *instance = new Role();
    return *instance;
  };

  // Set host.
  void setHost(bool is_host = true);

  // Return true if host.
  bool isHost() const;

  // return true if the game has started
  bool hasStarted() const;

  // send a message to create an entity to the client
  void sendCreateEntity(Object *obj);

  // register an object to synchronize
  void registerSyncObj(Object *p_obj);
  void unregisterSyncObj(Object *p_o);
  int eventHandler(const df::Event *p_e);
};

std::string fixedLength(int value, int digits = 3);

#endif // __ROLE_H__

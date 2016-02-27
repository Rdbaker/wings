#include "include/Event.h"
#include "include/EventStep.h"
#include "EventNetwork.h"
#include "include/LogManager.h"
#include "NetworkManager.h"
#include "Bullet.h"
#include "Explosion.h"
#include "Role.h"
#include "Hero.h"
#include "Saucer.h"
#include "GameOver.h"
#include "GameStart.h"
#include "include/WorldManager.h"


Role::Role() {
  // Need to update rate control each step.
  df::NetworkManager &net_manager = df::NetworkManager::getInstance();
  net_manager.registerInterest(this, df::NETWORK_EVENT);
  registerInterest(df::STEP_EVENT);

  // Set object type.
  setType("Role");
  setSolidness(df::SPECTRAL);
  has_started = false;
}

bool Role::hasStarted() const {
  return has_started;
}

void Role::setHost(bool is_host) {
  this->is_host = is_host;
}

bool Role::isHost() const {
  return is_host;
}

void Role::registerSyncObj(Object *p_obj) {
  obj_list.insert(p_obj);
}


int Role::eventHandler(const df::Event *p_e) {
  df::ObjectListIterator* oli = new df::ObjectListIterator(&obj_list);
  df::NetworkManager &net_manager = df::NetworkManager::getInstance();

  if(p_e->getType() == df::NETWORK_EVENT) {
    char buf[40000];
    memset(buf, 0, 40000);
    net_manager.receive(buf, 39999);
    std::string bufstr(buf);

    if(bufstr.find("GAMEOVER") != std::string::npos) {
      new GameOver;
      return -1;
    }

    // check if it's empty
    if(strlen(buf) == 0) {
      return 0;
    }

    // get the index of the next message
    int msgstart = bufstr.find("MSGTYPE");

    // see if we're making a new object
    if(bufstr[msgstart+8] == '0') {
      // check the object type
      // "MSGTYPE:n,OBJTYPE:n," has len -> 20
      if(buf[msgstart+18] == '0') {
        Hero *h = new Hero;
        h->deserialize(buf+msgstart+20);
      } else if(buf[msgstart+18] == '1') {
        Saucer *s = new Saucer;
        s->deserialize(buf+20);
      } else if(buf[msgstart+18] == '2') {
        Explosion *e = new Explosion;
        e->deserialize(buf+20);
      } else if(buf[msgstart+18] == '3') {
        df::Position *p = new df::Position(0, 0);
        Bullet *b = new Bullet(*p);
        b->deserialize(buf+20);
      } else if(buf[msgstart+18] == '4') {
        GameOver *g = new GameOver;
        g->deserialize(buf+20);
      } else if(buf[msgstart+18] == '5') {
        GameStart *g = new GameStart;
        g->deserialize(buf+20);
      }
    } else if (buf[msgstart+8] == '1') {
      // check if we're updating an object
      df::WorldManager &w_man = df::WorldManager::getInstance();
      // get the id as an int from the buf
      std::string str(buf+msgstart);
      int start = str.find("id");
      int end = str.find(",", start);
      std::string sid = str.substr(start+4, end - start);

      try {
        int id = stoi(sid);
        Object* obj = w_man.objectWithId(id);
        if(obj != NULL) {
          obj->deserialize(buf+4);
        }
      } catch (...) {
        df::LogManager &log_manager = df::LogManager::getInstance();
        log_manager.writeLog("Role::eventHandler: Error! could not stoi: %s", str.substr(start+4, end - start).c_str());
      }
      has_started = true;
    } else {
      // else we're deleting

    }

  } else if(p_e->getType() == df::STEP_EVENT) {
    Object* obj;
    // for every synchronized objects
    while(!oli->isDone()) {
      // get the current object
      obj = oli->currentObject();

      // check if any object is modified
      Role &role = Role::getInstance();
      if(obj->isModified() && role.isHost()) {
        // make the net mgr send the string
        const char* serialized = obj->serialize(true).c_str();

        char* buff = (char *)malloc(sizeof(char) * (strlen(serialized) + 24));
        memset(buff, 0, strlen(serialized)+24);

        // check if it's new
        if(strstr("id", serialized) != NULL) {
          // attach the message type
          // 0 - NEW
          // 1 - UPDATE
          // 2 - DELETE
          strcat(buff, "MSGTYPE:0,");
        } else {
          // it's an update
          // attach the message type
          // 0 - NEW
          // 1 - UPDATE
          // 2 - DELETE
          strcat(buff, "MSGTYPE:1,");
        }

        // attach the object type
        // 0 - HERO
        // 1 - SAUCER
        // 2 - EXPLOSION
        // 3 - BULLET
        // 4 - GAMEOVER
        // 5 - GAMESTART
        std::string type = obj->getType();
        const char* objtype;
        if (type == "Hero") {
            objtype = "OBJTYPE:0,";
        } else if (type == "Saucer") {
            objtype = "OBJTYPE:1,";
        } else if (type == "Explosion") {
            objtype = "OBJTYPE:2,";
        } else if (type == "Bullet") {
            objtype = "OBJTYPE:3,";
        } else if (type == "GameOver") {
            objtype = "OBJTYPE:4,";
        } else if (type == "GameStart") {
            objtype = "OBJTYPE:5,";
        }

        strcat(buff, objtype);

        /*
        if(strstr("id", serialized) == NULL) {
          strcat(buff, "id:");
          char b[3];
          memset(b, 0, 3);
          strcat(buff, std::to_string(obj->getId()).c_str());
          strcat(buff, ",");
        }*/

        // attach the buffer
        strcat(buff, serialized);

        net_manager.send((void *)buff, strlen(buff));
      }

      // set the iterator to the next object
      oli->next();
    }
  }
  return 0;
}

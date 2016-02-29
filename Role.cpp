#include "include/Event.h"
#include "include/Object.h"
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
  is_game_over = false;
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

void Role::unregisterSyncObj(Object *p_o) {
  obj_list.remove(p_o);
}

void Role::sendCreateEntity(Object *obj) {
  // get the entire serialized object
  const char* serialized = obj->serialize(true).c_str();

  char* buff = (char *)malloc(sizeof(char) * (strlen(serialized) + 6));
  memset(buff, 0, strlen(serialized)+6);

  // the first part of the message is how many bytes to read
  strcat(buff, fixedLength(strlen(serialized)+5).c_str());

  strcat(buff, "0");

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
      objtype = "0";
  } else if (type == "Saucer") {
      objtype = "1";
  } else if (type == "Explosion") {
      objtype = "2";
  } else if (type == "Bullet") {
      objtype = "3";
  } else if (type == "GameOver") {
      objtype = "4";
  } else if (type == "GameStart") {
      objtype = "5";
  }

  // append the object type
  strcat(buff, objtype);

  // append the serialized entity
  strcat(buff, serialized);

  if(strlen(buff) > 6) {
    df::NetworkManager &net_manager = df::NetworkManager::getInstance();
    net_manager.send((void *)buff, strlen(buff));
  }
}


int Role::eventHandler(const df::Event *p_e) {
  if(is_game_over)
    return -1;
  /*  MSG STRUCTURE
   *  000 - first three bytes are the size of the entity being sent over the network
   *        (undefined behavior for entities whose serialization is larger than 999 bytes)
   *  0   - second byte is the type of event (0=CREATE; 1=UPDATE; 2=DELETE)
   *  0   - third byte is the object type
   *        0 - HERO
   *        1 - SAUCER
   *        2 - EXPLOSION
   *        3 - BULLET
   *        4 - GAMEOVER
   *        5 - GAMESTART
   *  ... - the remaining bytes are the serialized entity
   */
  df::ObjectListIterator* oli = new df::ObjectListIterator(&obj_list);
  df::NetworkManager &net_manager = df::NetworkManager::getInstance();
  Role &role = Role::getInstance();

  if(p_e->getType() == df::NETWORK_EVENT) {
    int bytesinsock = net_manager.isData();
    if(bytesinsock == 0)
      return 0;


    // if we're the client
    // handle creating things from a message
    if(!role.isHost()) {
      // check if GAMEOVER is in the socket message queue
      char gotmpbuf[4000];
      memset(gotmpbuf, 0, 3999);
      net_manager.receive(gotmpbuf, 3999, true);
      df::LogManager &log_manager = df::LogManager::getInstance();
      if(strstr(gotmpbuf, "GAMEOVER") != NULL) {
        log_manager.writeLog("Role::eventHandler: found GAMEOVER in the network");
        is_game_over = true;
        new GameOver;
        return -1;
      }


      char tmpbuf[3];
      // peek at the length as indicated by the host's "packet"
      int pktlen = net_manager.receive(tmpbuf, 3, true);
      // make sure the lenth from isData is AT LEAST the length indicated
      while(bytesinsock >= atoi(tmpbuf)) {
        // receive that length
        char buf[atoi(tmpbuf)+1];
        memset(buf, 0, atoi(tmpbuf)+1);
        int bytesinsocket = net_manager.receive(buf, atoi(tmpbuf));

        std::string bufstr(buf);

        // check if it's empty
        if(strlen(buf) < 6) {
          return 0;
        }

        // see how many bytes this message is
        int msgbytes = stoi(bufstr.substr(0, 3));

        // we might not have anything worthwhile
        if(msgbytes < 6)
          return -1;

        std::string entity = bufstr.substr(0, msgbytes);

        // see if we're making a new object
        if(entity[3] == '0') {
          // check the object type
          if(entity[4] == '0') {
            Hero *h = new Hero;
            h->deserialize(entity.c_str()+5);
          } else if(entity[4] == '1') {
            log_manager.writeLog("Role::eventHandler: making a new saucer from: %s", entity.c_str()+5);
            Saucer *s = new Saucer;
            s->deserialize(entity.c_str()+5);
          } else if(entity[4] == '2') {
            Explosion *e = new Explosion;
            e->deserialize(entity.c_str()+5);
          } else if(entity[4] == '3') {
            log_manager.writeLog("Role::eventHandler: making a new bullet from: %s", entity.c_str()+5);
            df::Position *p = new df::Position(0, 0);
            Bullet *b = new Bullet(*p);
            b->deserialize(entity.c_str()+5);
          } else if(entity[4] == '4') {
            GameOver *g = new GameOver;
            g->deserialize(entity.c_str()+5);
          } else if(entity[4] == '5') {
            GameStart *g = new GameStart;
            g->deserialize(entity.c_str()+5);
          }
        } else if(entity[3] == '1') {
          // check if we're updating an object
          df::WorldManager &w_man = df::WorldManager::getInstance();
          // get the id as an int from the buf
          int start = entity.find("id");
          int end = entity.find(",", start);
          std::string sid = entity.substr(start+3, end - start);

          try {
            int id = stoi(sid);
            Object* obj = w_man.objectWithId(id);
            if(obj != NULL) {
              log_manager.writeLog("Role::eventHandler: object updating type: %s", obj->getType().c_str());
              log_manager.writeLog("Role::eventHandler: updating an object from: %s", entity.c_str());
              obj->deserialize(entity.c_str()+5);
            } else {
              // we might not have the entity, so create a new one:
              // check the object type
              if(entity[4] == '1') {
                Saucer *s = new Saucer;
                s->deserialize(entity.c_str()+5);
              } else if(entity[4] == '2') {
                log_manager.writeLog("Role::eventHandler: making a new explosion from: %s", entity.c_str()+5);
                Explosion *e = new Explosion;
                e->deserialize(entity.c_str()+5);
              } else if(entity[4] == '3') {
                log_manager.writeLog("Role::eventHandler: making a new bullet from: %s", entity.c_str()+5);
                df::Position *p = new df::Position(0, 0);
                Bullet *b = new Bullet(*p);
                b->deserialize(entity.c_str()+5);
              } else if(entity[4] == '4') {
                log_manager.writeLog("Role::eventHandler: making a new gameover from: %s", entity.c_str()+5);
                GameOver *g = new GameOver;
                g->deserialize(entity.c_str()+5);
              } else if(entity[4] == '5') {
                log_manager.writeLog("Role::eventHandler: making a new gamestart from: %s", entity.c_str()+5);
                GameStart *g = new GameStart;
                g->deserialize(entity.c_str()+5);
              }
            }
          } catch (...) {
            df::LogManager &log_manager = df::LogManager::getInstance();
            log_manager.writeLog("Role::eventHandler: Error! could not stoi: %s", entity.substr(start+4, end - start).c_str());
          }
          has_started = true;
        } else {
          // else we're deleting
          int start = entity.find("id")+3;
          std::string sid = entity.substr(start, entity.length() - start);
          int id = stoi(sid);
          df::WorldManager &w_man = df::WorldManager::getInstance();
          Object* obj = w_man.objectWithId(id);
          if(obj != NULL) {
            log_manager.writeLog("Role::eventHandler: deleting object with id: %d", obj->getId());
            if(obj->getType() != "ViewObject")
              w_man.markForDelete(obj);
          }
        }

        bytesinsock = net_manager.isData();
        if(bytesinsock == 0)
          return 0;

        // peek at the length as indicated by the host's "packet"
        memset(tmpbuf, 0, sizeof(tmpbuf));
        pktlen = net_manager.receive(tmpbuf, 3, true);
        log_manager.writeLog("Role::eventHandler: next packet size: %s, bytes in the socket: %d", tmpbuf, bytesinsock);
      }
    } else {
      // handle receiving an event
      char* buff = (char*)malloc(sizeof(char) * 50);
      memset(buff, 0, 50);
      net_manager.receive(buff, 49);
      df::LogManager &log_manager = df::LogManager::getInstance();

      if(strstr(buff, "KEY") != NULL) {
        // make a keyboard event
        log_manager.writeLog("Role::eventHandler: making a new keyboard event");
        df::EventKeyboard *p_k = new df::EventKeyboard;
        printf("what exactly is this..? %s\n", buff);
        //p_k->setKey();
      } else {
        // make a mouse event
        log_manager.writeLog("Role::eventHandler: making a new mouse event");
      }
    }

  } else if(p_e->getType() == df::STEP_EVENT && role.isHost()) {
    Object* obj;
    // for every synchronized objects
    while(!oli->isDone()) {
      // get the current object
      obj = oli->currentObject();

      // check if any object is modified
      Role &role = Role::getInstance();
      if(obj->isModified() && role.isHost()) {
        /*  MSG STRUCTURE
         *  000 - first three bytes are the size of the entity being sent over the network
         *        (undefined behavior for entities whose serialization is larger than 999 bytes)
         *  0   - second byte is the type of event (0=CREATE; 1=UPDATE; 2=DELETE)
         *  0   - third byte is the object type
         *        0 - HERO
         *        1 - SAUCER
         *        2 - EXPLOSION
         *        3 - BULLET
         *        4 - GAMEOVER
         *        5 - GAMESTART
         *  ... - the remaining bytes are the serialized entity
         */

        // get the entire serialized object
        const char* serialized = obj->serialize(true).c_str();

        char* buff = (char *)malloc(sizeof(char) * (strlen(serialized) + 6));
        memset(buff, 0, strlen(serialized)+6);

        // the first part of the message is how many bytes to read
        strcat(buff, fixedLength(strlen(serialized)+5).c_str());

        // check if it's new
        if(obj->isModified(df::ID)) {
          // attach the message type
          // 0 - NEW
          // 1 - UPDATE
          // 2 - DELETE
          //
          // THIS IS DEPRECATED
          //return 0;
          strcat(buff, "0");
        } else {
          // it's an update
          // attach the message type
          // 0 - NEW
          // 1 - UPDATE
          // 2 - DELETE
          strcat(buff, "1");
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
            objtype = "0";
        } else if (type == "Saucer") {
            objtype = "1";
        } else if (type == "Explosion") {
            objtype = "2";
        } else if (type == "Bullet") {
            objtype = "3";
        } else if (type == "GameOver") {
            objtype = "4";
        } else if (type == "GameStart") {
            objtype = "5";
        }

        // append the object type
        strcat(buff, objtype);

        // append the serialized entity
        strcat(buff, serialized);

        if(strlen(buff) > 6) {

          net_manager.send((void *)buff, strlen(buff));
        }
      }

      // set the iterator to the next object
      oli->next();
    }
  }
  return 0;
}


/* credit where it's due:
 * http://stackoverflow.com/questions/11521183/return-fixed-length-stdstring-from-integer-value
 *
 * this is more advanced C++ than I know how to write
 */
std::string fixedLength(int value, int digits) {
    unsigned int uvalue = value;
    if (value < 0) {
        uvalue = -uvalue;
    }
    std::string result;
    while (digits-- > 0) {
        result += ('0' + uvalue % 10);
        uvalue /= 10;
    }
    if (value < 0) {
        result += '-';
    }
    std::reverse(result.begin(), result.end());
    return result;
}

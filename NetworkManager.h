//
// NetworkManager.h
//
// Manage network connections to/from engine.
//

#ifndef __NETWORK_MANAGER_H__
#define __NETWORK_MANAGER_H__

// System includes.
#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>


// Engine includes.
#include "include/Manager.h"
#include "EventNetwork.h"

#define DRAGONFLY_PORT "8732"   // Default port.

namespace df {

  class NetworkManager : public df::Manager {

   private:
    NetworkManager() {}                      // Private since a singleton.
    NetworkManager(NetworkManager const&);  // Don't allow copy.
    void operator=(NetworkManager const&);  // Don't allow assignment.
    int sock;                               // Connected network socket.

   public:

    // Get the one and only instance of the NetworkManager.
    static NetworkManager &getInstance() {
      static NetworkManager *instance = new NetworkManager();
      return *instance;
    }

    // Start up NetworkManager.
    int startUp(bool isHost=true);

    // Shut down NetworkManager.
    void shutDown();

    // Accept only network events.
    // Returns false for other engine events.
    bool isValid(std::string event_type) const;

    // Block, waiting to accept network connection.
    int accept(std::string port = DRAGONFLY_PORT);

    // Make network connection.
    // Return 0 if success, else -1.
    int connect(std::string host, std::string port = DRAGONFLY_PORT);

    // Close network connection.
    // Return 0 if success, else -1.
    int close();

    // Return true if network connected, else false.
    bool isConnected() const;

    // Return socket.
    int getSocket() const;

    // Send buffer to connected network.
    // Return 0 if success, else -1.
    int send(void *buffer, int bytes);

    // Receive from connected network (no more than nbytes).
    // If peek is true, leave data in socket else remove.
    // Return number of bytes received, else -1 if error.
    int receive(void *buffer, int nbytes, bool peek = false);

    // Check if network data.
    // Return amount of data (0 if no data), -1 if not connected or error.
    int isData() const;

    // override the parent
    int onEvent(const Event *p_event) const;
  };

} // end of namespace df

void sigchld_handler(int s);
void *get_in_addr(struct sockaddr *sa);

#endif // __NETWORK_MANAGER_H__

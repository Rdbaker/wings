//
// NetworkManager.cpp
//


// Engine includes
#include "include/LogManager.h"
#include "NetworkManager.h"
#include "Role.h"


int df::NetworkManager::startUp(bool isHost, std::string host) {
  this->sock = -1;
  if(isHost) {
    this->accept();
  } else {
    if(host.empty()) {
      host = "127.0.0.1";
    }

    this->connect(host);
  }
  return 0;
}


void df::NetworkManager::shutDown() {
  if(this->sock < 0) {
    close();
  }
}


bool df::NetworkManager::isValid(std::string ename) const {
  return ename == NETWORK_EVENT;
}


bool df::NetworkManager::isConnected() const {
  return this->sock > 0;
}


int df::NetworkManager::getSocket() const {
  return this->sock;
}


// wait for a client to connect
int df::NetworkManager::accept(std::string port) {
  df::LogManager &log_manager = df::LogManager::getInstance();
  int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; // connector's address information
  socklen_t sin_size;
  struct sigaction sa;
  int yes=1;
  char s[INET6_ADDRSTRLEN];
  int rv;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // use my IP

  if ((rv = ::getaddrinfo(NULL, port.c_str(), &hints, &servinfo)) != 0) {
    log_manager.writeLog("NetworkManager::accept(): Error! Could not getaddrinfo(), error: %s", gai_strerror(errno));
    return -1;
  }

  // loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    // Create an unconnected socket to which a remote client can connect. A relevant system call is socket().
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
        p->ai_protocol)) == -1) {
      log_manager.writeLog("NetworkManager::accept(): Warning! Could not socket(), error: %s", gai_strerror(errno));
      continue;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
        sizeof(int)) == -1) {
      log_manager.writeLog("NetworkManager::accept(): Error! Could not setsockopt(), error: %s", gai_strerror(errno));
      return -1;
    }

    // Bind to the server's local address and port so a client can connect. A relevant system call is bind().
    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      ::close(sockfd);
      log_manager.writeLog("NetworkManager::accept(): Warning! Could not bind(), error: %s", gai_strerror(errno));
      continue;
    }

    break;
  }

  freeaddrinfo(servinfo); // all done with this structure

  if (p == NULL)  {
    log_manager.writeLog("NetworkManager::accept(): Error! Failed to bind(), error: %s", gai_strerror(errno));
    return -1;
  }

  // Put the socket in a state to accept the other "half" of an Internet connection. A relevant system call is listen().
  if (listen(sockfd, 100) == -1) {
    log_manager.writeLog("NetworkManager::accept(): Error! Failed to listen(), error: %s", gai_strerror(errno));
    return -1;
  }

  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    log_manager.writeLog("NetworkManager::accept(): Error! Failed to sigaction(), error: %s", gai_strerror(errno));
    return -1;
  }


  log_manager.writeLog("NetworkManager::accept(): host waiting for connection");
  // Wait (block) until the socket connected. A relevant system call is accept().
  while(1) {  // main accept() loop
    sin_size = sizeof their_addr;
    new_fd = ::accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
      log_manager.writeLog("NetworkManager::accept(): Warning! Failed to accept(), error: %s", gai_strerror(errno));
      continue;
    }

    inet_ntop(their_addr.ss_family,
      get_in_addr((struct sockaddr *)&their_addr),
      s, sizeof s);
    log_manager.writeLog("NetworkManager::accept(): host got connection from %s", s);

    ::close(sockfd); // don't need the old listener

    // update the "sock" variable
    this->sock = new_fd;

    return new_fd;
  }
}


// Return 0 if success, else -1.
int df::NetworkManager::connect(std::string host, std::string port) {
  df::LogManager &log_manager = df::LogManager::getInstance();
  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  log_manager.writeLog("NetworkManager::connect(): client attempting to connect");
  if ((rv = ::getaddrinfo(host.c_str(), port.c_str(), &hints, &servinfo)) != 0) {
    log_manager.writeLog("NetworkManager::connect(): Error! Failed to getaddrinfo(), error: %s", gai_strerror(errno));
    return -1;
  }

  // loop through all the results and connect to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
        p->ai_protocol)) == -1) {
      log_manager.writeLog("NetworkManager::connect(): Error! Failed to socket(), error: %s", gai_strerror(errno));
      return -1;
    }

    if (::connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      log_manager.writeLog("NetworkManager::connect(): Error! Failed to connect(), error: %s", gai_strerror(errno));
      ::close(sockfd);
      return -1;
    }

    break;
  }

  if (p == NULL) {
    log_manager.writeLog("NetworkManager::connect(): Error! Failed to connect to host, error: %s", gai_strerror(errno));
    return -1;
  }

  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
      s, sizeof s);
  log_manager.writeLog("NetworkManager::connect(): connecting to host: %s", s);

  freeaddrinfo(servinfo); // all done with this structure

  // set the socket variable
  this->sock = sockfd;

  return 0;
}


int df::NetworkManager::close() {
  return ::close(this->sock);
}


// Return 0 if success, else -1.
int df::NetworkManager::send(void *buffer, int bytes) {
  int sentsofar = 0;
  int sentthistime = 0;
  df::LogManager &log_manager = df::LogManager::getInstance();
  Role &role = Role::getInstance();
  log_manager.writeLog("NetworkManager::send(): about to send: %s", buffer);
  log_manager.writeLog("NetworkManager::send(): about to send this many: %d", bytes);

  if(role.isHost()) {
    while(sentsofar < bytes) {

      sentthistime = ::send(this->sock, buffer, bytes, 0);

    if(sentthistime == -1) {
      log_manager.writeLog("NetworkManager::send(): Error! send() failed: %s", gai_strerror(errno));
      return -1;
    } else if(sentthistime == 0) {
      break;
    }

    sentsofar += sentthistime;
    }
  }
  log_manager.writeLog("NetworkManager::send(): finished sending");
  return 0;
}


int df::NetworkManager::isData() const {
  int count = 0;
  if(isConnected() && ioctl(this->sock, FIONREAD, &count) < 0) {
    df::LogManager &log_manager = df::LogManager::getInstance();
    log_manager.writeLog("NetworkManager::isData(): Error! ioctl() failed: %s", gai_strerror(errno));
    return -1;
  }
  return count;
}


// Receive from connected network (no more than nbytes).
// If peek is true, leave data in socket else remove.
// Return number of bytes received, else -1 if error.
int df::NetworkManager::receive(void *buffer, int nbytes, bool peek) {
  if(!isConnected())
    return -1;
  int totalread = 0, resbytes = 0;
  int flags = (peek) ? MSG_PEEK | MSG_DONTWAIT : MSG_DONTWAIT;
  memset(buffer, 0, nbytes);
  df::LogManager &log_manager = df::LogManager::getInstance();

  while (totalread < nbytes) {
    // receive the response
    if((resbytes = recv(this->sock, buffer, nbytes, flags) ) < 0) {
      // there's an error
      log_manager.writeLog("NetworkManager::receive(): Error! recv() failed: %s", strerror(errno));
      return -1;
    } else if (resbytes == 0) {
      // we didn't receive any more bytes
      break;
    } else {
      // normal case
      totalread += resbytes;
    }
  }
  log_manager.writeLog("NetworkManager::receive(): just read: %s", buffer);
  log_manager.writeLog("NetworkManager::receive(): just read this many: %d", totalread);
  return totalread;
}


/*
 * handle a child signal
 */
void sigchld_handler(int s) {
  // waitpid() might overwrite errno, so we save and restore it:
  int saved_errno = errno;

  while(waitpid(-1, NULL, WNOHANG) > 0);

  errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

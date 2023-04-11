#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

#include "hot_potato.hpp"

using namespace std;

#define SIZE_OF_BUFFER 512

int get_random_num(int max_threshold, int id) {
  srand((unsigned int)time(NULL) + id);
  int random_num_in_range =
      rand() %
      max_threshold;  // Generate a random number within the range [0, max_threshold)
  return random_num_in_range;
}

int try_send(int socket_fd, const void * ptr, size_t size) {
  int bytes = 0;
  int numbyte;
  numbyte = send(socket_fd, ptr, size, MSG_WAITALL);
  if (numbyte == -1) {
    cerr << "Sending failed" << endl;
    exit(1);
  }
  return 0;
}

void get_ip_from_sockaddr_storage(struct sockaddr_storage * ss, char * ip) {
  if (ss->ss_family == AF_INET) {  // IPv4
    struct sockaddr_in * sin = (struct sockaddr_in *)ss;
    inet_ntop(AF_INET, &sin->sin_addr, ip, INET_ADDRSTRLEN);
  }
  else if (ss->ss_family == AF_INET6) {  // IPv6
    struct sockaddr_in6 * sin6 = (struct sockaddr_in6 *)ss;
    inet_ntop(AF_INET6, &sin6->sin6_addr, ip, INET6_ADDRSTRLEN);
  }
  else {
    exit(-1);
  }
}

int main(int argc, char * argv[]) {
  if (argc != 3) {
    perror("Usage:./player <machine_name> <port_num>\n");
    exit(EXIT_FAILURE);
  }

  char * endptr = NULL;
  const int server_port_num = (int)strtol(argv[2], &endptr, 10);
  if (!(*argv[1] != '\0' && *endptr == '\0') || server_port_num < 1024) {
    fprintf(stderr, "port number :%s is invalid!", argv[1]);
    exit(EXIT_FAILURE);
  }

  int status;
  int socket_fd, player_fd, left_fd, right_fd;
  int yes = 1;
  struct addrinfo host_info;
  struct addrinfo * host_info_list;
  const char * hostname = argv[1];
  const char * port = argv[2];
  struct sockaddr_storage their_addr;
  int id_num;

  socklen_t sin_size = sizeof(their_addr);

  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;

  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }  //if

  socket_fd = socket(host_info_list->ai_family,
                     host_info_list->ai_socktype,
                     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }  //if

  //cout << "Connecting to " << hostname << " on port " << port << "..." << endl;

  status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot connect to socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }  //if
  freeaddrinfo(host_info_list);
  int connectedFlag = 1;
  assert(!try_send(socket_fd, &connectedFlag, sizeof(connectedFlag)));
  int num_msg[2];
  //recv(socket_server,num_msg,sizeof(num_msg),0);
  assert(recv(socket_fd, num_msg, sizeof(num_msg), MSG_WAITALL) == sizeof(num_msg));
  int id = num_msg[0];
  int num_players = num_msg[1];
  cout << "Connected as player " << id << " out of " << num_players << " total players"
       << endl;

  //connect to neighbors
  int self_port_num = id + server_port_num + 1;
  int neighbor_port_num = ((id + 1) % num_players) + server_port_num + 1;
  string self_port_str = to_string(self_port_num);
  const char * self_port = self_port_str.c_str();
  string neighbor_port_str = to_string(neighbor_port_num);
  const char * neighbor_port = neighbor_port_str.c_str();

  //cout << self_port << endl;
  //cout << neighbor_port << endl;

  /* prepare self socket for listening*/
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags = AI_PASSIVE;

  status = getaddrinfo(NULL, self_port, &host_info, &host_info_list);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "  (" << hostname << "," << self_port << ")" << endl;
    return -1;
  }

  player_fd = socket(host_info_list->ai_family,
                     host_info_list->ai_socktype,
                     host_info_list->ai_protocol);
  if (player_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << hostname << "," << self_port << ")" << endl;
    return -1;
  }

  if (setsockopt(player_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    exit(1);
  }
  if (bind(player_fd, host_info_list->ai_addr, host_info_list->ai_addrlen) == -1) {
    close(player_fd);
    cerr << "Error: cannot bind socket" << endl;
    cerr << "  (" << hostname << "," << self_port << ")" << endl;
    exit(1);
  }
  freeaddrinfo(host_info_list);

  status = listen(player_fd, 100);
  if (status == -1) {
    cerr << "Error: cannot listen on socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }  //if
  //cout << "Waiting for connection on neighbor , my port" << self_port << endl;
  //try to connect a neighbor
  assert(recv(socket_fd, &sin_size, sizeof(sin_size), MSG_WAITALL) == sizeof(sin_size));
  struct sockaddr_storage neighbor_socket_addr;
  assert(recv(socket_fd, &neighbor_socket_addr, sin_size, MSG_WAITALL) == sin_size);

  char neighbor_ip[INET6_ADDRSTRLEN];
  get_ip_from_sockaddr_storage(&neighbor_socket_addr, neighbor_ip);

  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;

  status = getaddrinfo(neighbor_ip, neighbor_port, &host_info, &host_info_list);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "(" << neighbor_ip << "," << neighbor_port << ")" << endl;
    return -1;
  }  //if

  right_fd = socket(host_info_list->ai_family,
                    host_info_list->ai_socktype,
                    host_info_list->ai_protocol);
  if (socket_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << neighbor_ip << "," << neighbor_port << ")" << endl;
    return -1;
  }  //if

  //cout << "Connecting to " << neighbor_ip << " on port " << neighbor_port << "..."<< endl;

  while (connect(right_fd, host_info_list->ai_addr, host_info_list->ai_addrlen))
    ;
  //cout << "Successfully connect to right neigbor in port" << neighbor_port << endl;
  freeaddrinfo(host_info_list);

  ////accept connect request from another neighbor
  left_fd = accept(player_fd, (struct sockaddr *)&their_addr, &sin_size);
  if (left_fd == -1) {
    perror("accept");
    exit(-1);
  }

  HotPotato hp(0);

  struct timeval timeout;  // Timeout value
  timeout.tv_sec = 5;      // Set timeout to 5 seconds
  timeout.tv_usec = 0;

  int max_fd = (left_fd > right_fd) ? left_fd : right_fd;
  max_fd = (max_fd > socket_fd) ? max_fd : socket_fd;

  int seed = id;
  while (true) {
    fd_set read_fds;               // fd_set object to contain file descriptors
    FD_ZERO(&read_fds);            // Initialize to an empty set
    FD_SET(socket_fd, &read_fds);  // Add the server socket descriptor to the set
    FD_SET(left_fd, &read_fds);    // Add the left socket descriptor to the set
    FD_SET(right_fd, &read_fds);   // Add the right socket descriptor to the set
    int num_ready = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);

    if (num_ready) {
      if (FD_ISSET(socket_fd, &read_fds)) {
        // server_socket is ready for I/O
        assert(recv(socket_fd, &hp, sizeof(hp), MSG_WAITALL) == sizeof(hp));
      }
      else {
        if (FD_ISSET(left_fd, &read_fds)) {
          //left neighbor Socket is ready for I/O
          assert(recv(left_fd, &hp, sizeof(hp), MSG_WAITALL) == sizeof(hp));
        }
        else {
          assert(recv(right_fd, &hp, sizeof(hp), MSG_WAITALL) == sizeof(hp));
        }
      }
      //handle potato
      if (hp.cnt == hp.hops) {
        break;
      }
      else {
        hp.path[hp.cnt] = id;
        hp.cnt++;
        seed += num_players;
        if (hp.cnt < hp.hops) {  //pass to neighbor
          int random_number = get_random_num(2, seed);
          //cout << random_number << endl;
          if (random_number == 0) {  //choose left
            int neighbor_id = (id - 1 + num_players) % num_players;
            cout << "Sending potato to " << neighbor_id << endl;
            assert(!try_send(left_fd, &hp, sizeof(hp)));
          }
          else {  //choose right
            int neighbor_id = (id + 1) % num_players;
            cout << "Sending potato to " << neighbor_id << endl;
            assert(!try_send(right_fd, &hp, sizeof(hp)));
          }
        }
        else {
          hp.path[hp.cnt] = id;
          cout << "I'm it" << endl;
          assert(!try_send(socket_fd, &hp, sizeof(hp)));
        }
      }
    }
  }
  close(left_fd);
  close(right_fd);
  close(socket_fd);

  return 0;
}

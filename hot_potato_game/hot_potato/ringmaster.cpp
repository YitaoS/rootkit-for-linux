#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <thread>

#include "hot_potato.hpp"

using namespace std;

#define SIZE_OF_BUFFER 512
#define LISTEN_SIGNAL 0
#define CONNECT_SIGNAL 1
#define CONNECT_COMPLETE_SIGNAL -1
void * get_in_addr(struct sockaddr * sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int try_send(int socket_fd, const void * ptr, size_t size, int flags) {
  int bytes = 0;
  int numbyte;
  while (bytes != size) {
    numbyte = send(socket_fd, ptr, size, flags);
    if (numbyte == -1) {
      cerr << "Sending failed" << endl;
      return 1;  // Returning 1 to indicate failure
    }
    bytes += numbyte;
  }
  return 0;  // Returning 0 to indicate success
}

int get_random_num(int max_threshold) {
  srand(time(0));
  int random_num_in_range =
      rand() %
      max_threshold;  // Generate a random number within the range [0, max_threshold)
  return random_num_in_range;
}

int main(int argc, char * argv[]) {
  if (argc != 4) {
    perror("Usage:./ringmaster <port_num> <num_players> <num_hops>");
    exit(EXIT_FAILURE);
  }

  char * endptr = NULL;
  const int port_num = (int)strtol(argv[1], &endptr, 10);
  if (!(*argv[1] != '\0' && *endptr == '\0') || port_num < 1024) {
    fprintf(stderr, "port number :%s is invalid!", argv[1]);
    exit(EXIT_FAILURE);
  }
  endptr = NULL;
  const int num_players = (int)strtol(argv[2], &endptr, 10);
  if (!(*argv[2] != '\0' && *endptr == '\0' || num_players <= 1)) {
    fprintf(stderr, "number of players :%s is invalid!", argv[2]);
    exit(EXIT_FAILURE);
  }
  endptr = NULL;
  int num_hops = (int)strtol(argv[3], &endptr, 10);
  if (!(*argv[2] != '\0' && *endptr == '\0' || num_hops < 0 || num_hops > 512)) {
    fprintf(stderr, "number of hops :%s is invalid!", argv[3]);
    exit(EXIT_FAILURE);
  }
  cout << "Potato Ringmaster" << endl;
  cout << "Players = " << num_players << endl;
  cout << "Hops = " << num_hops << endl;

  int status;
  int socket_fd, new_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list, *p;
  const char * hostname = NULL;
  const char * port = argv[1];
  // listen to the port
  memset(&host_info, 0, sizeof(host_info));

  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags = AI_PASSIVE;

  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }  //if
  assert(host_info_list->ai_next->ai_next == NULL);
  socket_fd = socket(host_info_list->ai_family,
                     host_info_list->ai_socktype,
                     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }  //if

  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot bind socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }  //if
  freeaddrinfo(host_info_list);
  status = listen(socket_fd, 100);
  if (status == -1) {
    cerr << "Error: cannot listen on socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }  //if

  //cout << "Waiting for connection on port " << port << endl;
  int client_sockets[num_players];
  struct sockaddr_storage client_socket_addr[num_players];
  socklen_t client_socket_addr_len[num_players];
  //build connection socket
  for (int i = 0; i < num_players; i++) {
    struct sockaddr_storage their_addr;
    socklen_t sin_size = sizeof(their_addr);
    client_sockets[i] = accept(socket_fd, (struct sockaddr *)&their_addr, &sin_size);
    struct sockaddr_in * sin = (struct sockaddr_in *)&their_addr;
    int port = ntohs(sin->sin_port);
    cout << port << endl;
    if (client_sockets[i] == -1) {
      perror("accept");
      exit(1);
    }
    int connected_flag = 0;
    assert(
        recv(client_sockets[i], &connected_flag, sizeof(connected_flag), MSG_WAITALL) ==
        sizeof(connected_flag));
    if (connected_flag == 1) {
      cout << "Player " << i << " is ready to play" << endl;
      int num_msg[2];
      num_msg[0] = i;
      num_msg[1] = num_players;
      assert(!try_send(client_sockets[i], &num_msg, sizeof(num_msg), 0));
    }
    else {
      exit(-1);
    }
    client_socket_addr[i] = their_addr;
    client_socket_addr_len[i] = sin_size;
  }

  //send message to ask them connect to their neighbors

  for (int i = 0; i < num_players; i++) {
    const int listen_signal = 0;
    const int connect_signal = 1;
    assert(!try_send(client_sockets[i], &listen_signal, sizeof(listen_signal), 0));
    assert(!try_send(client_sockets[(i + 1) % num_players],
                     &connect_signal,
                     sizeof(connect_signal),
                     0));
    assert(!try_send(client_sockets[(i + 1) % num_players],
                     &client_socket_addr_len[i],
                     sizeof(client_socket_addr_len[i]),
                     0));
    assert(!try_send(client_sockets[(i + 1) % num_players],
                     &client_socket_addr[i],
                     client_socket_addr_len[i],
                     0));
  }

  //inform every player connection has been built
  for (int i = 0; i < num_players; i++) {
    int instruct = CONNECT_COMPLETE_SIGNAL;
    if (send(client_sockets[i], &instruct, sizeof instruct, 0) == -1) {
      perror("send");
    }
  }

  HotPotato hp(num_hops);
  int random_number = get_random_num(num_players);
  cout << "Ready to start the game, sending potato to player " << random_number << endl;
  assert(!try_send(client_sockets[random_number], &hp, sizeof(hp), 0));

  fd_set read_fds;  // fd_set object to contain file descriptors

  int max_fd = client_sockets[0];
  FD_ZERO(&read_fds);  // Initialize to an empty set
  for (int i = 0; i < num_players; i++) {
    max_fd = (max_fd > client_sockets[i]) ? max_fd : client_sockets[i];
    FD_SET(client_sockets[i], &read_fds);  // Add sockets descriptor to the set
  }
  int num_ready = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
  if (num_ready == -1) {
    cerr << "select()" << endl;
    exit(1);
  }
  for (int i = 0; i < num_players; i++) {
    if (FD_ISSET(client_sockets[i], &read_fds)) {
      //recv(socket_array[i],&potato,sizeof(potato),0);
      assert(recv(client_sockets[i], &hp, sizeof(hp), MSG_WAITALL) == sizeof(hp));
      assert(hp.cnt == hp.hops);
      hp.print();
      for (int i = 0; i < num_players; i++) {
        assert(!try_send(client_sockets[i], &hp, sizeof(hp), 0));
      }
    }
  }
  FD_ZERO(&read_fds);
  for (int i = 0; i < num_players; i++) {
    close(client_sockets[i]);
  }
}

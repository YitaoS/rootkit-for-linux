#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

class HotPotato {
 public:
  int hops;
  int cnt;
  int path[512];
  HotPotato() : hops(0), cnt(0) { memset(path, 0, sizeof(path)); }
  HotPotato(int h) : hops(h), cnt(0) { memset(path, 0, sizeof(path)); }
  void print() {
    std::cout << "Trace of potato:" << std::endl;
    for (int i = 0; i < cnt - 1; i++) {
      std::cout << path[i] << ",";
    }
    std::cout << path[cnt] << std::endl;
  }
};
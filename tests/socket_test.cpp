// #include "../../source/socket.h"
#include "log.h"
#include "socket.h"
#include <string>

using namespace std;

int main() {
  Log log;
  int i;
  string str;
  cin >> i;
  Socket sock(i, &log);
  int msg_len = sock.getMaxLen();
  char* str_c = new char[msg_len];
  while (true){
    str = sock.getMessage();
    if (! str.compare("!!!")) {
      break;
    }
  }
  sock.turnOff();
  return 0;
}
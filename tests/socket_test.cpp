// #include "../../source/socket.h"
#include "../source/log.h"
#include "../source/socket.h"
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
  while (! sock.acceptConnection()){
    cout << 1 << endl;
    sleep(1);
  }
  cout << "Connected" << endl;
  while (true){
    cout << "waiting for message" << endl;
    while (! sock.getMessage(&str)){
      if (! sock.checkClient()){
        cout << "Client is disconnected" << endl;
        while (! sock.acceptConnection()){
          sleep(1);
        }
        cout << "Connected" << endl;
      }
      sleep(1);
    }
    cout << str << endl;
    if (! str.compare("!!!")) {
      break;
    }
    // sock.answer((char *)str.c_str());
    sock.answer("good");
    // sock.answer("\n");
  }
  sock.turnOff();
  return 0;
}
#include "socket.h"

using namespace std;

Socket::Socket(int port_number, Log* logFile) {
  log = logFile;
  log->print("Socket initialization...");
  listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listener < 0){
    log->print("Can't create socket");
    cerr << "Can't create socket";
    exit(1);
  }
  log->print("Socket is created.");
  
  portNo = port_number;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(portNo);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  if(bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0){
    log->print("Can't bind socket");
    exit(2);
  }
  log->print("Socket is bound on port %d.", portNo);

  listen(listener, 1);  // В очереди только ОДИН запрос!
  log->print("TCP server is activated");
  msg_sock = -1;
}

Socket::~Socket(){
  if (msg_sock >= 0) {
    close(msg_sock);
  }
  close(listener);
}

void Socket::turnOff(){
  log->print("TCP server on port %d is closed", portNo);
  if (msg_sock >= 0) {
    close(msg_sock);
  }
  close(listener);
}

std::string Socket::getMessage() {
  if (msg_sock >= 0 ) {
    close(msg_sock);
  }

  char* message = new char[MESSAGE_LEN];

  msg_sock = accept(listener, 0, 0);
  if(msg_sock < 0){
    perror("Can't connect to the client\n");
    exit(3);
  }
  read(msg_sock, message, MESSAGE_LEN);
  string msg = strtok(message, "\n\0"); // Удаляем символ конца строки
  if (message != NULL) {
    delete []message;
  }
  return msg;
}






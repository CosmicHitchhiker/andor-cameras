#include "socket.h"

using namespace std;

Socket::Socket(int port_number, Log* logFile, int mode) { // mode == 0 - сервер, mode == 1 - клиент
  signal(SIGSEGV, Socket::myError);   // Вывод ошибки в случае переполнения стека
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

  if (mode == 0) {
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0){
      log->print("Can't bind socket");
      exit(2);
    }
    log->print("Socket is bound on port %d.", portNo);

    listen(listener, 1);  // В очереди только ОДИН запрос!
    log->print("TCP server is activated");
    msg_sock = -1;
  } else if (mode == 1) {
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if(connect(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      log->print("Can't connect");
      exit(2);
    }
    log->print("TCP client is activated");
  }
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
  if (listener >= 0) {
    close(listener);
  }
}

std::string Socket::getMessage() {
  string msg;
  while (true) {
    int bytes_read = 0;
    if (msg_sock >= 0 ) {     // Закрываем предыдущее общение
      close(msg_sock);
    }
    char* message = new char[MESSAGE_LEN];
    msg_sock = accept(listener, 0, 0);
    try {
      if(msg_sock < 0){
        throw "Can't connect to the client\n";
      }
    } catch (const char* exc){
      log->print(exc);
      if (message != NULL) delete []message;
      continue;
    }
    bytes_read = recv(msg_sock, message, MESSAGE_LEN, 0);
    if (! *message or message[0]=='\n' or message[0]=='\0' or bytes_read <= 0) {
      if (message != NULL) delete []message;
      continue;
    }
    msg = strtok(message, "\n\0"); // Удаляем символ конца строки
    if (msg.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890!?/.,:;'[]{}\"\\%#@*-=()_ ") != std::string::npos) {
      if (message != NULL) delete []message;
      continue;
    }
    if (message != NULL) delete []message;
    break;    
  }
  log->print("Client message: %s\0", msg.c_str());
  return msg;
}

void Socket::answer(const char* message) {
  if (msg_sock >= 0) {
    send(msg_sock, message, strlen(message), 0);
    log->print("Answer: %s", message);
  } else {
    log->print("CAN'T answer: %s", message);
  }
}

void Socket::sendMessage(char* message) {
  if (!(! *message or message[0]=='\n' or message[0]=='\0')){
    send(listener, message, strlen(message), 0);
    log->print("Send: %s", message);
  }
}

void Socket::myError(int){
  cerr << "ERROR\n";
  exit(3);
}

int Socket::getMaxLen(){
  return MESSAGE_LEN;
}

int Socket::getDescriptor(){
  return listener;
}

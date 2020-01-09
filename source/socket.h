#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <unistd.h>
#include "log.h"
#include <string>
#include <cstring>
//#include <boost/algorithm/string.hpp>

#ifndef SOCKET_H
#define SOCKET_H

class Socket
{
  public:
	Socket(int port_number, Log* logFile);
	~Socket();
	std::string getMessage();
	void turnOff();

  private:
  	int portNo;	// Номер порта
  	int listener;  // Основной сокет для приёма данных
  	struct sockaddr_in addr;
  	int msg_sock;  // Создаваемый для приёма команды сокет
	Log* log;
	const int MESSAGE_LEN = 1024;
};

#endif // SOCKET_H